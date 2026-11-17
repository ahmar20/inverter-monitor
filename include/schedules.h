#pragma once
// ============================================================================
// schedules.h — time-of-day automation: NVS-backed schedule list, cloud
// sync (push/fetch), firing logic (checkAndFireSchedules, once per minute),
// missed-schedule catch-up on boot, and the authenticated
// POST /api/schedules and POST /api/schedules/run handlers. Schedules fire
// by appending framed PI18 commands to schedCmdQueue[], which loop() drains
// one command at a time into the shared webCmd* dispatch state declared in
// inverter_protocol.h.
// ============================================================================

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <ESPAsyncWebServer.h>
#include "config.h"
#include "debug_log.h"
#include "inverter_protocol.h"

// ── Scheduled command queue ────────────────────────────────────────
// Populated by checkAndFireSchedules(); drained by loop() one cmd at a time.
String schedCmdQueue[SCHED_CMD_QUEUE_MAX];
int    schedCmdQueueLen       = 0;   // how many cmds loaded this fire
int    schedCmdQueueIdx       = 0;   // next cmd to send
String schedulesJson          = "[]";  // in-RAM copy of NVS schedules
bool   schedulesCloudFetched  = false; // prevent repeated boot-fetch attempts
bool   missedSchedulesChecked = false;

void   fetchSchedulesFromCloud();
void   pushSchedulesToCloud(const String& json);

// C++ equivalent of JS buildCmd() — converts (setting, stored-value) → "^SxxxCMDparam"
// Stored value formats:
//   POP/PCP/PBT/PGR/MCHGC/MUCHGC/PSDV  — raw string ("1", "30", "440")
//   MCHGV  — "bulk10x,float10x" e.g. "572,560"
//   BUCD   — "stopdisch10x,stopchg10x"  e.g. "440,520"
//   ACCT/ACLT — "HHmm,HHmm" e.g. "0700,1800"
String buildScheduledCmd(const String& cmd, const String& value) {
    // FLAG_A … FLAG_H: value is "E" (enable) or "D" (disable)
    // e.g. FLAG_A + "E" → body "PEA" → "^S006PEA"
    //      FLAG_H + "D" → body "PDH" → "^S006PDH"
    if (cmd.startsWith("FLAG_") && cmd.length() == 6) {
        String body = "P" + value + String(cmd.charAt(5));
        char hdr[8];
        snprintf(hdr, sizeof(hdr), "^S%03d", (int)(body.length() + 3));
        return String(hdr) + body;
    }
    String param;
    if (cmd == "PCP") {
        param = "0," + value;
    } else if (cmd == "MCHGC") {
        char buf[8];
        snprintf(buf, sizeof(buf), "0,%03d", value.toInt());
        param = String(buf);
    } else if (cmd == "MUCHGC") {
        char buf[8];
        snprintf(buf, sizeof(buf), "0,%03d", value.toInt());
        param = String(buf);
    } else if (cmd == "V") {
        // stored value is "220"/"230"/"240"; PI18 needs volts*10, 4 digits
        char buf[8];
        snprintf(buf, sizeof(buf), "%04d", value.toInt() * 10);
        param = String(buf);
    } else {
        // POP, PBT, PGR, PSDV, F: raw value is already the param
        // MCHGV, BUCD, ACCT, ACLT: already stored as "X,Y"
        param = value;
    }
    String body = cmd + param;
    char header[8];
    snprintf(header, sizeof(header), "^S%03d", (int)(body.length() + 3));
    return String(header) + body;
}

// ════════════════════════════════════════════════════════════════════
//  SCHEDULE AUTOMATION
// ════════════════════════════════════════════════════════════════════

// ── NVS helpers ──────────────────────────────────────────────────────
String loadSchedulesNVS() {
    Preferences prefs;
    prefs.begin("inverter", true);  // read-only
    String s = prefs.getString("schedules", "[]");
    prefs.end();
    return s;
}

void saveSchedulesNVS(const String& json) {
    Preferences prefs;
    prefs.begin("inverter", false);
    prefs.putString("schedules", json);
    prefs.end();
}

// ── Cloud push / fetch (blocking HTTPS; always call from a task) ──────
void pushSchedulesToCloud(const String& json) {
    if (WiFi.status() != WL_CONNECTED) return;
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient https;
    // Reuse the cloud URL base: replace /telemetry with /schedules/<device_id>
    String url = String(cloudUpdateUrl);
    url.replace("/telemetry", "/schedules/" + inverterIDForDB);
    if (!https.begin(client, url)) return;
    https.setTimeout(4000);
    https.addHeader("Content-Type", "application/json");
    https.addHeader("Authorization", String("Bearer ") + cloudApiKey);
    String body = "{\"schedules\":" + json + "}";
    int code = https.PUT(body);
    Serial.printf("[SCHED] Cloud push HTTP %d\n", code);
    https.end();
}

void fetchSchedulesFromCloud() {
    if (WiFi.status() != WL_CONNECTED) return;
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient https;
    String url = String(cloudUpdateUrl);
    url.replace("/telemetry", "/schedules/" + inverterIDForDB);
    if (!https.begin(client, url)) return;
    https.setTimeout(4000);
    https.addHeader("Authorization", String("Bearer ") + cloudApiKey);
    int code = https.GET();
    if (code == HTTP_CODE_OK) {
        String resp = https.getString();
        JsonDocument doc;
        if (deserializeJson(doc, resp) == DeserializationError::Ok &&
            doc["schedules"].is<JsonArray>()) {
            String arr;
            serializeJson(doc["schedules"], arr);
            schedulesJson = arr;
            saveSchedulesNVS(arr);
            Serial.println("[SCHED] Fetched from cloud and saved to NVS.");
        }
    } else {
        Serial.printf("[SCHED] Cloud fetch failed: HTTP %d\n", code);
    }
    https.end();
}

// Task wrappers so HTTPS calls never block loop()
static char schedPushBuf[4096];

void schedPushTask(void* param) {
    pushSchedulesToCloud(String(schedPushBuf));
    vTaskDelete(NULL);
}

void startSchedPushTask(const String& json) {
    json.toCharArray(schedPushBuf, sizeof(schedPushBuf));
    BaseType_t r = xTaskCreate(schedPushTask, "SchedPush", 8192, NULL, 1, NULL);
    if (r != pdPASS) Serial.println("[SCHED] Failed to create push task");
}

void schedFetchTask(void* param) {
    fetchSchedulesFromCloud();
    vTaskDelete(NULL);
}

// ── Boot-time cloud fetch (once, after WiFi up, only when NVS is empty) ──
void maybeAttemptScheduleFetch() {
    if (schedulesCloudFetched) return;
    if (WiFi.status() != WL_CONNECTED) return;
    schedulesCloudFetched = true;  // attempt exactly once
    if (schedulesJson != "[]" && schedulesJson.length() > 2) {
        Serial.println("[SCHED] NVS has data — skipping cloud fetch.");
        return;
    }
    Serial.println("[SCHED] NVS empty — fetching schedules from cloud.");
    BaseType_t r = xTaskCreate(schedFetchTask, "SchedFetch", 8192, NULL, 1, NULL);
    if (r != pdPASS) Serial.println("[SCHED] Failed to create fetch task");
}

// ── Boot-time missed-schedule catch-up ────────────────────────────────────
void checkMissedSchedules() {
    if (!schedulesCloudFetched) return;
    struct tm t;
    if (!getLocalTime(&t, 0)) return;   // NTP not ready yet — caller will retry

    struct tm tCopy = t;
    tCopy.tm_sec = 0;
    time_t nowEpoch = mktime(&tCopy);

    if (schedulesJson.length() <= 2) {
        missedSchedulesChecked = true;
        return;
    }

    JsonDocument doc;
    if (deserializeJson(doc, schedulesJson) != DeserializationError::Ok) {
        missedSchedulesChecked = true;
        return;
    }

    bool anyQueued  = false;
    bool anyUpdated = false;

    for (JsonObject sched : doc.as<JsonArray>()) {
        bool enabled = sched["enabled"] | true;
        if (!enabled) continue;

        const char* timeStr = sched["time"] | "";
        if (strlen(timeStr) < 5) continue;
        int sHour = (timeStr[0]-'0')*10 + (timeStr[1]-'0');
        int sMin  = (timeStr[3]-'0')*10 + (timeStr[4]-'0');

        // Find the most recent past occurrence of this schedule (search back up to 7 days)
        time_t expectedEpoch = 0;
        for (int daysBack = 0; daysBack <= 7; daysBack++) {
            time_t candidate = nowEpoch - (time_t)daysBack * 86400;
            struct tm ct;
            localtime_r(&candidate, &ct);

            bool dayOk = false;
            for (JsonVariant d : sched["days"].as<JsonArray>()) {
                if (d.as<int>() == ct.tm_wday) { dayOk = true; break; }
            }
            if (!dayOk) continue;

            ct.tm_hour = sHour;
            ct.tm_min  = sMin;
            ct.tm_sec  = 0;
            time_t fireTime = mktime(&ct);

            if (fireTime <= nowEpoch) {
                expectedEpoch = fireTime;
                break;
            }
        }

        if (expectedEpoch == 0) continue;

        // Skip if already fired on time
        time_t lastFired = (time_t)(sched["lastFiredEpoch"] | (long long)0);
        if (lastFired >= expectedEpoch) continue;

        dbgLog("[SCHED] Missed \"%s\" (expected %lld, last fired %lld) — catch-up queuing",
               (sched["label"] | ""), (long long)expectedEpoch, (long long)lastFired);

        for (JsonObject cmd : sched["commands"].as<JsonArray>()) {
            if (schedCmdQueueLen >= SCHED_CMD_QUEUE_MAX) break;
            const char* setting = cmd["setting"] | "";
            const char* value   = cmd["value"]   | "";
            if (strlen(setting) == 0) continue;
            schedCmdQueue[schedCmdQueueLen++] = buildScheduledCmd(String(setting), String(value));
        }

        sched["lastFiredEpoch"] = (long long)expectedEpoch;
        anyQueued  = true;
        anyUpdated = true;
    }

    if (anyUpdated) {
        String newJson;
        serializeJson(doc, newJson);
        schedulesJson = newJson;
        saveSchedulesNVS(newJson);
        startSchedPushTask(newJson);   // sync lastFiredEpoch to cloud
    }

    if (anyQueued)
        dbgLog("[SCHED] %d catch-up command(s) queued", schedCmdQueueLen);

    missedSchedulesChecked = true;
}

// ── Scheduler tick — called every loop(), fires once per minute at most ──
void checkAndFireSchedules() {
    static int lastFiredMinute = -1;
    struct tm t;
    if (!getLocalTime(&t, 0)) return;

    int nowMinute = t.tm_hour * 60 + t.tm_min;
    if (nowMinute == lastFiredMinute) return;   // already handled this minute

    // Don't clobber a queue still draining
    if (schedCmdQueueIdx < schedCmdQueueLen) return;

    lastFiredMinute = nowMinute;
    dbgLog("[SCHED] Checking schedules for %02d:%02d day=%d", t.tm_hour, t.tm_min, t.tm_wday);

    if (schedulesJson.length() <= 2) return;

    JsonDocument doc;
    if (deserializeJson(doc, schedulesJson) != DeserializationError::Ok) return;

    schedCmdQueueLen = 0;
    schedCmdQueueIdx = 0;
    bool anyFired = false;

    // Epoch of this minute's fire time (for lastFiredEpoch)
    struct tm tCopy = t;
    tCopy.tm_sec = 0;
    time_t nowEpoch = mktime(&tCopy);

    for (JsonObject sched : doc.as<JsonArray>()) {
        bool enabled = sched["enabled"] | true;
        if (!enabled) continue;

        const char* timeStr = sched["time"] | "";
        if (strlen(timeStr) < 5) continue;
        int sHour = (timeStr[0]-'0')*10 + (timeStr[1]-'0');
        int sMin  = (timeStr[3]-'0')*10 + (timeStr[4]-'0');
        if (sHour != t.tm_hour || sMin != t.tm_min) continue;

        bool dayOk = false;
        for (JsonVariant d : sched["days"].as<JsonArray>()) {
            if (d.as<int>() == t.tm_wday) { dayOk = true; break; }
        }
        if (!dayOk) continue;

        dbgLog("[SCHED] MATCH — firing \"%s\" at %02d:%02d day=%d", (sched["label"] | ""), t.tm_hour, t.tm_min, t.tm_wday);

        for (JsonObject cmd : sched["commands"].as<JsonArray>()) {
            if (schedCmdQueueLen >= SCHED_CMD_QUEUE_MAX) break;
            const char* setting = cmd["setting"] | "";
            const char* value   = cmd["value"]   | "";
            if (strlen(setting) == 0) continue;
            schedCmdQueue[schedCmdQueueLen++] = buildScheduledCmd(String(setting), String(value));
        }

        // Record that this schedule fired so checkMissedSchedules() won't re-fire it on next boot
        sched["lastFiredEpoch"] = (long long)nowEpoch;
        anyFired = true;
    }

    if (anyFired) {
        // Persist updated lastFiredEpoch values
        String newJson;
        serializeJson(doc, newJson);
        schedulesJson = newJson;
        saveSchedulesNVS(newJson);
        startSchedPushTask(newJson);   // sync lastFiredEpoch to cloud
    }

    if (schedCmdQueueLen > 0)
        dbgLog("[SCHED] %d command(s) queued for dispatch", schedCmdQueueLen);
}

// ── POST /api/schedules handler ────────────────────────────────────────
void handleSchedulePost(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
    if (index == 0) {
        if (request->_tempObject) free(request->_tempObject);
        request->_tempObject = malloc(total + 1);
        if (!request->_tempObject) {
            request->send(500, "application/json",
                          "{\"status\":\"error\",\"message\":\"Out of memory\"}");
            return;
        }
    }
    if (request->_tempObject)
        memcpy((uint8_t*)request->_tempObject + index, data, len);
    if (index + len < total) return;   // more chunks coming

    ((uint8_t*)request->_tempObject)[total] = 0;
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, (char*)request->_tempObject);
    free(request->_tempObject);
    request->_tempObject = nullptr;

    if (err) {
        request->send(400, "application/json",
                      "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
        return;
    }
    if (!doc["schedules"].is<JsonArray>()) {
        request->send(400, "application/json",
                      "{\"status\":\"error\",\"message\":\"Missing schedules array\"}");
        return;
    }

    String newJson;
    serializeJson(doc["schedules"], newJson);
    schedulesJson    = newJson;
    saveSchedulesNVS(newJson);

    // Clear any stale queue so a just-deleted schedule can't still fire
    schedCmdQueueLen = 0;
    schedCmdQueueIdx = 0;

    // Push to cloud in background (non-blocking)
    startSchedPushTask(newJson);
    dbgLog("[SCHED] Saved %u bytes to NVS, cloud push started", newJson.length());

    request->send(200, "application/json",
                  "{\"status\":\"ok\",\"message\":\"Schedules saved\"}");
}

// ── POST /api/schedules/run — immediately fire one schedule by index ───────
void handleScheduleRunNow(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
    if (index == 0) {
        if (request->_tempObject) free(request->_tempObject);
        request->_tempObject = malloc(total + 1);
        if (!request->_tempObject) {
            request->send(500, "application/json",
                          "{\"status\":\"error\",\"message\":\"Out of memory\"}");
            return;
        }
    }
    if (request->_tempObject)
        memcpy((uint8_t*)request->_tempObject + index, data, len);
    if (index + len < total) return;

    ((uint8_t*)request->_tempObject)[total] = 0;
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, (char*)request->_tempObject);
    free(request->_tempObject);
    request->_tempObject = nullptr;

    if (err || !doc["index"].is<int>()) {
        request->send(400, "application/json",
                      "{\"status\":\"error\",\"message\":\"Missing or invalid index\"}");
        return;
    }

    int idx = doc["index"].as<int>();

    JsonDocument schedDoc;
    DeserializationError schedErr = deserializeJson(schedDoc, schedulesJson);
    if (schedErr || !schedDoc.is<JsonArray>()) {
        request->send(500, "application/json",
                      "{\"status\":\"error\",\"message\":\"Cannot parse schedules\"}");
        return;
    }
    JsonArray arr = schedDoc.as<JsonArray>();

    if (idx < 0 || idx >= (int)arr.size()) {
        request->send(400, "application/json",
                      "{\"status\":\"error\",\"message\":\"Index out of range\"}");
        return;
    }

    JsonObject sched = arr[idx].as<JsonObject>();
    JsonArray  cmds  = sched["commands"].as<JsonArray>();
    if (!cmds || cmds.size() == 0) {
        request->send(400, "application/json",
                      "{\"status\":\"error\",\"message\":\"Schedule has no commands\"}");
        return;
    }

    // Clear any pending queue before loading this run
    schedCmdQueueLen = 0;
    schedCmdQueueIdx = 0;

    for (JsonObject cmd : cmds) {
        if (schedCmdQueueLen >= SCHED_CMD_QUEUE_MAX) break;
        schedCmdQueue[schedCmdQueueLen++] = buildScheduledCmd(
            cmd["setting"].as<String>(), cmd["value"].as<String>());
    }
    webCmdIsScheduled = true;

    // Update lastFiredEpoch to now
    time_t nowEpoch = 0;
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) nowEpoch = mktime(&timeinfo);

    if (nowEpoch > 0) {
        sched["lastFiredEpoch"] = (long long)nowEpoch;
        String newJson;
        serializeJson(arr, newJson);
        schedulesJson = newJson;
        saveSchedulesNVS(newJson);
        startSchedPushTask(newJson);
    }

    dbgLog("[SCHED] Run-now: %d cmd(s) queued for schedule %d", schedCmdQueueLen, idx);

    String resp = "{\"status\":\"ok\",\"queued\":" + String(schedCmdQueueLen) +
                  ",\"lastFiredEpoch\":" + String((long long)nowEpoch) + "}";
    request->send(200, "application/json", resp);
}

