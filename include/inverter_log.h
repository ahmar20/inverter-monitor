#pragma once
// ============================================================================
// inverter_log.h — shared ring-buffer connectivity log, covering both the
// inverter link (driven by lastSuccessfulInverterResponse) and the WiFi
// connection (driven by WiFi.status()). One timeline, tagged by source, so
// the home page can show what went down and for how long, for either one.
// ============================================================================

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include "config.h"
#include "types.h"
#include "utils.h"
#include "debug_log.h"

struct ConnLogEntry {
    String        timestamp;    // ISO time the event was recorded
    const char*   source;       // "inverter" or "wifi"
    bool          down;         // true = just went down, false = just recovered
    unsigned long downForMs;    // only meaningful on a "recovered" entry
};
static ConnLogEntry connLog[INV_LOG_MAX];
static uint8_t connLogHead  = 0;
static uint8_t connLogCount = 0;

static void connLogPush(const char* source, bool down, unsigned long downForMs) {
    connLog[connLogHead] = { getISOTime(), source, down, downForMs };
    connLogHead = (connLogHead + 1) % INV_LOG_MAX;
    if (connLogCount < INV_LOG_MAX) connLogCount++;
}

// ── Inverter link ────────────────────────────────────────────────────────
static bool inverterCurrentlyDown = false;
static unsigned long inverterDownSince = 0;

// Call every loop(). Cheap: just a millis() subtraction unless a transition fires.
void checkInverterConnectivity() {
    unsigned long sinceLast = (lastSuccessfulInverterResponse == 0) ? millis() : millis() - lastSuccessfulInverterResponse;

    if (!inverterCurrentlyDown && sinceLast >= INV_DOWN_THRESHOLD_MS) {
        inverterCurrentlyDown = true;
        inverterDownSince = millis() - sinceLast;   // approx. moment it actually went quiet
        connLogPush("inverter", true, 0);
        dbgLog("[INV] No response for >%lus - marked down", INV_DOWN_THRESHOLD_MS / 1000UL);
    } else if (inverterCurrentlyDown && sinceLast < INV_DOWN_THRESHOLD_MS) {
        unsigned long downFor = millis() - inverterDownSince;
        inverterCurrentlyDown = false;
        connLogPush("inverter", false, downFor);
        dbgLog("[INV] Recovered after %lu ms", downFor);
    }
}

// ── WiFi link ────────────────────────────────────────────────────────────
static bool wifiCurrentlyDown = false;   // mirrors "disconnected", for the home page banner
static bool wifiEverConnected = false;   // suppresses a bogus "recovered" entry for the first boot connect
static unsigned long wifiDownSince = 0;

// Call every loop(), alongside checkInverterConnectivity().
void checkWifiConnectivity() {
    bool connected = (WiFi.status() == WL_CONNECTED);

    if (!connected && !wifiCurrentlyDown) {
        wifiCurrentlyDown = true;
        wifiDownSince = millis();
        if (wifiEverConnected) {
            connLogPush("wifi", true, 0);
            dbgLog("[WIFI] Disconnected");
        }
    } else if (connected && wifiCurrentlyDown) {
        unsigned long downFor = millis() - wifiDownSince;
        wifiCurrentlyDown = false;
        if (wifiEverConnected) {
            connLogPush("wifi", false, downFor);
            dbgLog("[WIFI] Reconnected after %lu ms", downFor);
        }
        wifiEverConnected = true;
    } else if (connected) {
        wifiEverConnected = true;   // first-ever boot connect: note it, don't log it
    }
}

String buildInverterLogJson() {
    JsonDocument doc;
    JsonArray arr = doc["events"].to<JsonArray>();
    uint8_t start = (connLogCount < INV_LOG_MAX) ? 0 : connLogHead;
    for (uint8_t i = 0; i < connLogCount; i++) {
        ConnLogEntry &e = connLog[(start + i) % INV_LOG_MAX];
        JsonObject o = arr.add<JsonObject>();
        o["timestamp"] = e.timestamp;
        o["source"]    = e.source;
        o["event"]     = e.down ? "down" : "recovered";
        if (!e.down) o["downForMs"] = e.downForMs;
    }
    doc["inverterCurrentlyDown"] = inverterCurrentlyDown;
    doc["wifiCurrentlyDown"]     = wifiCurrentlyDown;
    String json;
    serializeJson(doc, json);
    return json;
}