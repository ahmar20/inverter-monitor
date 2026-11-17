#pragma once
// ============================================================================
// web_server.h — the AsyncWebServer instance and every HTTP route. Page
// routes serve the pre-built HTML strings from web_pages/*.h; API routes
// delegate to api_handlers.h, inverter_protocol.h and schedules.h. This is
// the one file that "wires everything together" on the HTTP side.
// ============================================================================

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include "config.h"
#include "types.h"
#include "wifi_manager.h"
#include "inverter_protocol.h"
#include "schedules.h"
#include "cloud_queue.h"
#include "api_handlers.h"
#include "debug_report.h"
#include "inverter_log.h"
#include "web_pages/page_root.h"
#include "web_pages/page_home.h"
#include "web_pages/page_dashboard.h"
#include "web_pages/page_debug.h"
#include "web_pages/page_history.h"
#include "web_pages/page_settings.h"
#include "web_pages/assets.h"

AsyncWebServer server(80);

// Latest inverter data
unsigned long lastDataUpdate = 0;
bool haveInverterData = false;

// Pending reboot (set by POST /api/reboot, actioned from loop())
bool pendingReboot = false;
unsigned long pendingRebootAt = 0;

void setupWebServer();

// WEB SERVER HANDLING
void sendWebPage(AsyncWebServerRequest *request)
{
  AsyncWebServerResponse *response = request->beginResponse(200, "text/html", index_html);
  response->addHeader("Cache-Control", "no-cache");
  request->send(response);
}

// JSON API
// NOTE: this is now a nested {"GSX":{...},"MOD":{...}, ...} shape - one
// object per command - instead of the old flat structure. The dashboard's
// JS render() needs to be updated to read e.g. data.GSX.gridVoltage instead
// of data.gridVoltage.
void sendJSON(AsyncWebServerRequest* request)
{
    JsonDocument doc;
    doc["valid"] = haveInverterData;
    doc["wifiConnected"] = (WiFi.status() == WL_CONNECTED);
    doc["inverterConnected"] = !inverterCurrentlyDown;
    doc["inverterAge"] = (millis() - lastSuccessfulInverterResponse) / 1000;

    for (uint8_t i = 0; i < NUM_INVERTER_COMMANDS; i++)
    {
        InverterCommandDef &def = inverterCommands[i];
        JsonObject o = doc[def.name].to<JsonObject>();
        def.jsonWriter(o);
    }

    String json;
    serializeJson(doc, json);

    AsyncWebServerResponse* response =
        request->beginResponse(200, "application/json", json);
    response->addHeader("Cache-Control", "no-cache");
    request->send(response);
}

void setupWebServer()
{
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
          { request->send(200, "text/html", root_html); });
  server.on("/dashboard", HTTP_GET, [](AsyncWebServerRequest *request)
          { sendWebPage(request); });
  server.on("/api/data", HTTP_GET, [](AsyncWebServerRequest *request)
            { sendJSON(request); });
  server.on("/api/serial", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              String debugData = generateSerialDebug();
              AsyncResponseStream *response = request->beginResponseStream("text/plain; charset=utf-8");
              response->print(debugData);
              request->send(response);
            });
  server.on("/serial", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/html", debug_html); });
  server.on("/history", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/html", history_html); });
  server.on("/manifest.json", HTTP_GET, [](AsyncWebServerRequest *request) {
      AsyncWebServerResponse *r = request->beginResponse(200, "application/manifest+json", manifest_json);
      r->addHeader("Cache-Control", "public, max-age=86400");
      request->send(r);
  });
  server.on("/icon.svg", HTTP_GET, [](AsyncWebServerRequest *request) {
      AsyncWebServerResponse *r = request->beginResponse(200, "image/svg+xml", icon_svg);
      r->addHeader("Cache-Control", "public, max-age=86400");
      request->send(r);
  });
  server.on("/sw.js", HTTP_GET, [](AsyncWebServerRequest *request) {
      AsyncWebServerResponse *r = request->beginResponse(200, "application/javascript", sw_js);
      r->addHeader("Cache-Control", "no-cache");
      request->send(r);
  });
  server.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request) {
      if (!request->authenticate(SETTINGS_USER, SETTINGS_PASS))
          return request->requestAuthentication();
      request->send(200, "text/html", settings_html);
  });

  server.on("/api/command", HTTP_POST,
      [](AsyncWebServerRequest* request) {
          if (!request->authenticate(SETTINGS_USER, SETTINGS_PASS))
              return request->requestAuthentication();
      },
      NULL,
      [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
          if (!request->authenticate(SETTINGS_USER, SETTINGS_PASS))
              return request->requestAuthentication();
          handleInverterCommand(request, data, len, index, total);
      }
  );

  // POST /api/schedules/run — immediately fire one schedule by index (auth required)
  server.on("/api/schedules/run", HTTP_POST,
      [](AsyncWebServerRequest* request) {
          if (!request->authenticate(SETTINGS_USER, SETTINGS_PASS))
              return request->requestAuthentication();
      },
      NULL,
      [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
          if (!request->authenticate(SETTINGS_USER, SETTINGS_PASS))
              return request->requestAuthentication();
          handleScheduleRunNow(request, data, len, index, total);
      }
  );

  // GET /api/schedules — return current schedules JSON (auth required)
  server.on("/api/schedules", HTTP_GET,
      [](AsyncWebServerRequest* request) {
          if (!request->authenticate(SETTINGS_USER, SETTINGS_PASS))
              return request->requestAuthentication();
          String body = "{\"schedules\":" + schedulesJson + "}";
          AsyncWebServerResponse* resp = request->beginResponse(200, "application/json", body);
          resp->addHeader("Cache-Control", "no-cache");
          request->send(resp);
      }
  );

  // POST /api/schedules — save schedules array; relays to cloud in background (auth required)
  server.on("/api/schedules", HTTP_POST,
      [](AsyncWebServerRequest* request) {
          if (!request->authenticate(SETTINGS_USER, SETTINGS_PASS))
              return request->requestAuthentication();
      },
      NULL,
      [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
          if (!request->authenticate(SETTINGS_USER, SETTINGS_PASS))
              return request->requestAuthentication();
          handleSchedulePost(request, data, len, index, total);
      }
  );

  server.on("/home", HTTP_GET, [](AsyncWebServerRequest *request)
          { request->send(200, "text/html", home_html); });

  server.on("/api/system", HTTP_GET, [](AsyncWebServerRequest *request)
          { handleSystemInfo(request); });

  server.on("/api/inverter/log", HTTP_GET, [](AsyncWebServerRequest *request) {
      String json = buildInverterLogJson();
      AsyncWebServerResponse* r = request->beginResponse(200, "application/json", json);
      r->addHeader("Cache-Control", "no-cache");
      request->send(r);
  });

  server.on("/api/wifi/scan", HTTP_GET, [](AsyncWebServerRequest *request)
          { handleWifiScan(request); });

  // Profiles are only ever added via the scan flow, never edited.
  server.on("/api/wifi/profiles", HTTP_GET, [](AsyncWebServerRequest* request) {
      JsonDocument doc;
      JsonArray arr = doc["profiles"].to<JsonArray>();
      for (uint8_t i = 0; i < wifiProfileCount; i++) {
          JsonObject o = arr.add<JsonObject>();
          o["ssid"]       = wifiProfiles[i].ssid;
          o["active"]     = (i == currentWiFi);
          o["isFallback"] = wifiProfiles[i].isFallback;
      }
      String json;
      serializeJson(doc, json);
      AsyncWebServerResponse* r = request->beginResponse(200, "application/json", json);
      r->addHeader("Cache-Control", "no-cache");
      request->send(r);
  });

  // Add ONE new profile. Fails if the SSID already exists - delete it first
  // to change its password (Android-style: no in-place edits).
  server.on("/api/wifi/profiles/add", HTTP_POST,
      [](AsyncWebServerRequest* request) { /* nothing to do until onBody completes */ },
      NULL,
      [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
          handleWifiProfileAdd(request, data, len, index, total);
      }
  );

  // Delete ONE saved profile by SSID. Fallback profiles aren't in NVS, so
  // this can never touch them.
  server.on("/api/wifi/profiles/delete", HTTP_POST,
      [](AsyncWebServerRequest* request) { /* nothing to do until onBody completes */ },
      NULL,
      [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
          handleWifiProfileDelete(request, data, len, index, total);
      }
  );

  // Drop the current connection and rescan immediately - useful if it's
  // stuck on a weak/bad AP. No auth: consistent with the rest of /home.
  server.on("/api/wifi/disconnect", HTTP_POST, [](AsyncWebServerRequest* request) {
      disconnectWiFi();
      request->send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Disconnected\"}");
  });

  // Report whether cloud upload is configured - never the key itself.
  server.on("/api/cloud/config", HTTP_GET, [](AsyncWebServerRequest* request) {
      if (!request->authenticate(SETTINGS_USER, SETTINGS_PASS))
          return request->requestAuthentication();
      JsonDocument doc;
      doc["configured"] = cloudConfigured();
      doc["url"] = cloudUpdateUrl;
      String json;
      serializeJson(doc, json);
      AsyncWebServerResponse* r = request->beginResponse(200, "application/json", json);
      r->addHeader("Cache-Control", "no-cache");
      request->send(r);
  });

  // Set/replace cloud credentials at runtime (auth required). Body:
  // {"url":"...","key":"..."} - both required together, or both blank to
  // disable. Takes effect immediately, no reboot needed.
  server.on("/api/cloud/config", HTTP_POST,
      [](AsyncWebServerRequest* request) {
          if (!request->authenticate(SETTINGS_USER, SETTINGS_PASS))
              return request->requestAuthentication();
      },
      NULL,
      [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
          if (!request->authenticate(SETTINGS_USER, SETTINGS_PASS))
              return request->requestAuthentication();
          handleCloudConfigSet(request, data, len, index, total);
      }
  );

  server.on("/api/history/local", HTTP_GET, [](AsyncWebServerRequest* request) {
      handleLocalHistory(request);
  });

  server.on("/api/reboot", HTTP_POST, [](AsyncWebServerRequest* request) {
      if (!request->authenticate(SETTINGS_USER, SETTINGS_PASS))
          return request->requestAuthentication();
      pendingReboot = true;
      pendingRebootAt = millis() + 500;
      request->send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Rebooting\"}");
  });

  server.on("/api/queue/clear", HTTP_POST, [](AsyncWebServerRequest* request) {
      if (!request->authenticate(SETTINGS_USER, SETTINGS_PASS))
          return request->requestAuthentication();
      xSemaphoreTake(cqMutex, portMAX_DELAY);
      cqRamHead = 0; cqRamCount = 0;
      xSemaphoreGive(cqMutex);
      if (cqFlashOk) {
          File dir = LittleFS.open(CQ_DIR);
          for (File f = dir.openNextFile(); f; f = dir.openNextFile()) {
              String name = String(f.name());
              f.close();
              LittleFS.remove(name);
          }
          dir.close();
          cqFlashRecords = 0;
          cqFlashHeadSeq = cqFlashNextSeq;
          cqFlashHeadOff = 0;
      }
      request->send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Queue cleared\"}");
  });

  server.begin();
}

