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

                    case "/api/status":
                        HandleApiStatus(e);
                        break;

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
                        <style>
                            body {
                                font-family: sans-serif;
                                background: #0b1220;
                                color: #e8edf5;
                                display: flex;
                                align-items: center;
                                justify-content: center;
                                height: 100vh;
                                margin: 0;
                            }
                        </style>
                        </head>
                        <body>
                        <p id=""msg"">Loading your dashboard, please wait...</p>
                        <script>
                        (function () {
                            var dashboardUrl = ""https://raw.githubusercontent.com/ahmar20/inverter-monitor/refs/heads/master/page.html"";
                            var cacheKey = ""dashboardHtmlCache"";
                            var msg = document.getElementById(""msg"");

                            function render(html) {
                                document.open();
                                document.write(html);
                                document.close();
                            }

                            function load(attempt) {
                                var controller = new AbortController();
                                var timeoutId = setTimeout(function () {
                                    controller.abort();
                                }, 8000);

                                fetch(dashboardUrl, { signal: controller.signal, cache: ""no-store"" })
                                    .then(function (res) {
                                        clearTimeout(timeoutId);
                                        if (!res.ok) throw new Error(""HTTP "" + res.status);
                                        return res.text();
                                    })
                                    .then(function (html) {
                                        try { localStorage.setItem(cacheKey, html); } catch (e) {}
                                        render(html);
                                    })
                                    .catch(function (err) {
                                        clearTimeout(timeoutId);

                                        if (attempt < 2) {
                                            setTimeout(function () { load(attempt + 1); }, 1500);
                                            return;
                                        }

                                        var cached = null;
                                        try { cached = localStorage.getItem(cacheKey); } catch (e) {}

                                        if (cached) {
                                            msg.textContent = ""Using last saved dashboard (offline) ..."";
                                            setTimeout(function () { render(cached); }, 600);
                                        } else {
                                            msg.textContent =
                                                ""Could not load dashboard: "" + err.message +
                                                "". Check that this device has internet access."";
                                        }
                                    });
                            }

                            load(1);
                        })();
                        </script>
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
                "\"uptime\":" + Helpers.GetFormattedUptime() +
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
