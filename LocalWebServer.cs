using System;
using System.Collections;
using System.Net;
using System.Text;
using nanoFramework.Json;
using nanoFramework.WebServer;

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
            string html =
                "<!DOCTYPE html>" +
                "<html>" +
                "<head>" +
                "<meta charset='UTF-8'>" +
                "<meta name='viewport' content='width=device-width,initial-scale=1'>" +
                "<title>Inverter Monitor</title>" +
                "</head>" +
                "<body>" +
                "<h1>Inverter Monitor</h1>" +
                "<p>ESP32 Web Server is working.</p>" +
                "</body>" +
                "</html>";

            SendResponse(e, html, "text/html", HttpStatusCode.OK);
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
