#pragma once
// ============================================================================
// api_handlers.h — small JSON API endpoints for the /home system page:
// system stats (/api/system), WiFi scan (/api/wifi/scan), and saving WiFi
// profiles (/api/wifi/profiles POST). Reboot and queue-clear stay as small
// inline lambdas in web_server.h since they're one-liners.
// ============================================================================

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include <Preferences.h>
#include <LittleFS.h>
#include "config.h"
#include "types.h"
#include "debug_log.h"
#include "wifi_manager.h"
#include "cloud_queue.h"

void handleSystemInfo(AsyncWebServerRequest* request) {
    JsonDocument doc;
    doc["uptimeMs"]     = millis();
    doc["freeHeap"]     = ESP.getFreeHeap();
    doc["minFreeHeap"]  = ESP.getMinFreeHeap();
    doc["heapSize"]     = ESP.getHeapSize();
    doc["psramFound"]   = psramFound();
    if (psramFound()) {
        doc["freePsram"]  = ESP.getFreePsram();
        doc["totalPsram"] = ESP.getPsramSize();
        doc["usedPsram"]  = ESP.getPsramSize() - ESP.getFreePsram();
    }
    doc["flashTotal"]   = cqFlashOk ? LittleFS.totalBytes() : 0;
    doc["flashUsed"]    = cqFlashOk ? LittleFS.usedBytes()  : 0;
    doc["buildDate"]    = __DATE__ " " __TIME__;
    doc["chipModel"]    = ESP.getChipModel();
    doc["chipRevision"] = ESP.getChipRevision();
    doc["sdkVersion"]   = ESP.getSdkVersion();
    doc["chipTemp"]     = temperatureRead();
    doc["wifiConnected"]= (WiFi.status() == WL_CONNECTED);
    doc["wifiSSID"]     = WiFi.SSID();
    doc["wifiRSSI"]     = WiFi.RSSI();
    doc["localIP"]      = WiFi.localIP().toString();
    doc["macAddress"]   = WiFi.macAddress();
    doc["queueRam"]     = cqRamCount;
    doc["queueRamCap"]  = cqRamCap;
    doc["queueFlash"]   = cqFlashRecords;
    doc["queueDropped"] = cqDropped;
    doc["cloudUploadPending"]   = inverter.cloud.cloudUploadPending;
    doc["lastCloudUpdate"]      = inverter.cloud.lastSuccessfulCloudUpdate;
    doc["cloudConfigured"]      = cloudConfigured();


    String json;
    serializeJson(doc, json);
    AsyncWebServerResponse* r = request->beginResponse(200, "application/json", json);
    r->addHeader("Cache-Control", "no-cache");
    request->send(r);
}

void handleWifiScan(AsyncWebServerRequest* request) {
    if (request->hasParam("start") && !wifiScanning && !wifiConnecting) {
        wifiScanRequested = true;   // actually started from manageWiFi() on the main loop task
    }
    JsonDocument doc;
    doc["scanning"] = wifiScanning || wifiScanRequested;
    doc["ready"]    = homeScanReady;
    JsonArray arr = doc["networks"].to<JsonArray>();
    for (uint8_t i = 0; i < wifiScanResultCount; i++) {
        JsonObject o = arr.add<JsonObject>();
        o["ssid"]   = wifiScanResults[i].ssid;
        o["rssi"]   = wifiScanResults[i].rssi;
        o["secure"] = wifiScanResults[i].secure;
    }
    String json;
    serializeJson(doc, json);
    AsyncWebServerResponse* r = request->beginResponse(200, "application/json", json);
    r->addHeader("Cache-Control", "no-cache");
    request->send(r);
}

void handleWifiProfileAdd(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
    if (index == 0) {
        if (request->_tempObject) free(request->_tempObject);
        request->_tempObject = malloc(total + 1);
        if (!request->_tempObject) {
            request->send(500, "application/json", "{\"status\":\"error\",\"message\":\"Out of memory\"}");
            return;
        }
    }
    if (request->_tempObject) memcpy((uint8_t*)request->_tempObject + index, data, len);
    if (index + len < total) return;

    ((uint8_t*)request->_tempObject)[total] = 0;
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, (char*)request->_tempObject);
    free(request->_tempObject);
    request->_tempObject = nullptr;

    const char* ssid = doc["ssid"] | "";
    const char* pass = doc["password"] | "";
    if (err || strlen(ssid) == 0) {
        request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing SSID\"}");
        return;
    }
    for (uint8_t i = 0; i < wifiProfileCount; i++) {
        if (wifiProfiles[i].ssid == ssid) {
            request->send(409, "application/json",
                "{\"status\":\"error\",\"message\":\"A profile with that SSID already exists - delete it first to change it\"}");
            return;
        }
    }

    JsonDocument nvsDoc;
    DeserializationError nvsErr = deserializeJson(nvsDoc, loadWifiProfilesNVS());
    JsonArray arr = (!nvsErr && nvsDoc.is<JsonArray>()) ? nvsDoc.as<JsonArray>() : nvsDoc.to<JsonArray>();
    JsonObject o = arr.add<JsonObject>();
    o["ssid"]     = ssid;
    o["password"] = pass;
    String json;
    serializeJson(arr, json);
    saveWifiProfilesNVS(json);
    loadWifiProfilesIntoRAM();

    dbgLog("[WIFI] Added profile: %s", ssid);
    request->send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Profile added\"}");
}

void handleWifiProfileDelete(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
    if (index == 0) {
        if (request->_tempObject) free(request->_tempObject);
        request->_tempObject = malloc(total + 1);
        if (!request->_tempObject) {
            request->send(500, "application/json", "{\"status\":\"error\",\"message\":\"Out of memory\"}");
            return;
        }
    }
    if (request->_tempObject) memcpy((uint8_t*)request->_tempObject + index, data, len);
    if (index + len < total) return;

    ((uint8_t*)request->_tempObject)[total] = 0;
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, (char*)request->_tempObject);
    free(request->_tempObject);
    request->_tempObject = nullptr;

    const char* ssid = doc["ssid"] | "";
    if (err || strlen(ssid) == 0) {
        request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing SSID\"}");
        return;
    }

    JsonDocument nvsDoc;
    DeserializationError nvsErr = deserializeJson(nvsDoc, loadWifiProfilesNVS());
    if (nvsErr || !nvsDoc.is<JsonArray>()) {
        request->send(500, "application/json", "{\"status\":\"error\",\"message\":\"Cannot read saved profiles\"}");
        return;
    }
    JsonArray oldArr = nvsDoc.as<JsonArray>();
    JsonDocument newDoc;
    JsonArray newArr = newDoc.to<JsonArray>();
    bool found = false;
    for (JsonObject p : oldArr) {
        const char* s = p["ssid"] | "";
        if (String(s) == ssid) { found = true; continue; }   // drop this one
        JsonObject o = newArr.add<JsonObject>();
        o["ssid"]     = s;
        o["password"] = p["password"] | "";
    }
    if (!found) {
        request->send(404, "application/json", "{\"status\":\"error\",\"message\":\"Profile not found\"}");
        return;
    }
    String json;
    serializeJson(newArr, json);
    saveWifiProfilesNVS(json);
    loadWifiProfilesIntoRAM();

    dbgLog("[WIFI] Deleted profile: %s", ssid);
    request->send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Profile deleted\"}");
}