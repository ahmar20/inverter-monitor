using nanoFramework.Json;
using nanoFramework.WebServer;
using nanoFramework.System.IO.FileSystem;
using System;
using System.IO;
using System.Net;

namespace inverter_monitor
{
    internal class LocalWebServer
    {
        private static WebServer _server;

        public static void Start()
        {
            if (_server != null)
                return;

            Helpers.Log("[WEB] Starting web server...");
            _server = new WebServer(80, HttpProtocol.Http);
            _server.CommandReceived += OnCommandReceived;
            _server.Start();
            Helpers.Log("[WEB] Web server started on port 80.");
        }

        private static void OnCommandReceived(object sender, WebServerEventArgs e)
        {
            string url = e.Context.Request.RawUrl;
            Helpers.Log("[WEB] " + url);
            try
            {
                switch (url)
                {
                    case "/":
                        HandleRoot(e);
                        break;

                    case "/api/data":
                        HandleApiData(e);
                        break;

                    //case "/api/status":
                    //    HandleApiStatus(e);
                    //    break;

                    default:
                        SendNotFound(e);
                        break;
                }
            }
            catch (Exception ex)
            {
                Helpers.Log("[WEB] Error: " + ex.Message);
                SendError(e);
            }
        }

        private static void HandleRoot(WebServerEventArgs e)
        {
            const string DirectoryPath = "I:\\"; // Internal storage
            // Check if file exists and serve it
            string filePath = DirectoryPath + "page.html";
            if (File.Exists(filePath))
            {
                WebServer.SendFileOverHTTP(e.Context.Response, filePath);
            }
            else
            {
                // if file is not available then load a dynamically loaded webpage
                // given the link of raw html file on Github.
                string htmlContent = @"<!DOCTYPE html>
                        <html lang=""en"">
                        <head>
                            <meta charset=""UTF-8"">
                            <meta name=""viewport"" content=""width=device-width, initial-scale=1.0"">
                            <title>Loading Dashboard...</title>
                            <script>
                                // The code executes automatically when the browser reads this script
                                const dashboardUrl = 'https://github.com/ahmar20/inverter-monitor/raw/refs/heads/master/page.html';

                                fetch(dashboardUrl)
                                  .then(response => {
                                    if (!response.ok) throw new Error(`HTTP error! Status: ${response.status}`);
                                    return response.blob(); 
                                  })
                                  .then(blob => {
                                    const htmlBlob = new Blob([blob], { type: 'text/html' });
                                    const newPageUrl = URL.createObjectURL(htmlBlob);
                                    window.location.replace(newPageUrl);
                                  })
                                  .catch(error => {
                                    console.error('Failed to download or render the dashboard:', error);
                                    document.body.innerHTML = '<h1>Error loading dashboard. Please try again later.</h1>';
                                  });
                            </script>
                        </head>
                        <body>
                            <!-- This message shows briefly while the file downloads -->
                            <p>Loading your dashboard, please wait...</p>
                        </body>
                        </html>";
                SendResponse(e, htmlContent, "text/html", HttpStatusCode.OK);
            }
        }

        private static void HandleApiData(WebServerEventArgs e)
        {
            // Temporary test data.
            // Later this will come from your inverter data model.

            string json = JsonConvert.SerializeObject(Inverter.InverterData.InverterSerialDataObject);
            SendResponse(e, json, "application/json", HttpStatusCode.OK);
        }

        private static void HandleApiStatus(WebServerEventArgs e)
        {
            string json =
                "{" +
                "\"status\":\"ok\"," +
                "\"uptime\":" + Helpers.ElapsedTime +
                "}";

            SendResponse(e, json, "application/json", HttpStatusCode.OK);
        }

        private static void SendNotFound(WebServerEventArgs e)
        {
            SendResponse(e, "{\"error\":\"Not Found\"}", "application/json", HttpStatusCode.NotFound);
        }

        private static void SendError(WebServerEventArgs e)
        {
            SendResponse(e, "{\"error\":\"Internal Server Error\"}", "application/json", HttpStatusCode.InternalServerError);
        }

        private static void SendResponse(WebServerEventArgs e, string content, string contentType, HttpStatusCode statusCode)
        {
            e.Context.Response.StatusCode = (int)statusCode;
            e.Context.Response.ContentType = contentType;

            WebServer.OutputAsStream(e.Context.Response, content);
        }
    }
}
