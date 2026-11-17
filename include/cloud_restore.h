#pragma once
// ============================================================================
// cloud_restore.h — boot-time restore of today's energy counters from the
// cloud (so a reboot doesn't zero out daily totals), run in its own FreeRTOS
// task so the blocking HTTPS GET never stalls loop(). Also kicks off the
// cloud upload task (cloud_queue.h) on its own pacing.
// ============================================================================

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "config.h"
#include "types.h"
#include "cloud_queue.h"

void restoreEnergyCountersFromCloud()
{
    if (cloudCountersRestored || cloudRecoveryInProgress)
        return;

    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("[Cloud] restoreEnergyCountersFromCloud task not executed due to WIFi.");
        return;
    }
    
    cloudRecoveryInProgress = true;
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient https;

    // Get today's date for the daily endpoint
    struct tm tinfo;
    char todayStr[11] = "";
    if (getLocalTime(&tinfo, 0))
        snprintf(todayStr, sizeof(todayStr), "%04d-%02d-%02d", tinfo.tm_year + 1900, tinfo.tm_mon + 1, tinfo.tm_mday);
    String url = String(cloudUpdateUrl) + "/daily" + "?device_id=" + String(inverterIDForDB) +
                 "&from_date=" + String(todayStr) + "&to_date="   + String(todayStr);
    Serial.println("[CLOUD] Restoring energy counters...");
    Serial.println("[CLOUD] GET: " + url);
    if (!https.begin(client, url))
    {
        Serial.println("[CLOUD] HTTPS connection failed.");
        cloudRecoveryInProgress = false;
        return;
    }
    https.setTimeout(3000);
    https.addHeader("Authorization", String("Bearer ") + cloudApiKey);
    int httpCode = https.GET();
    if (httpCode == HTTP_CODE_OK)
    {
        String response = https.getString();
        Serial.printf("[CLOUD] Recovery successful - HTTP %d\n", httpCode);
        Serial.println("[CLOUD] Response: " + response);
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, response);
        if (error)
        {
            Serial.print("[CLOUD] JSON parse failed: ");
            Serial.println(error.c_str());
            https.end();
            cloudRecoveryInProgress = false;
            return;
        }
        // Daily endpoint returns { "days": [ { "date": "...", "pv_energy_kwh": ... } ] }
        JsonArray days = doc["days"];
        if (days.isNull() || days.size() == 0) {
            Serial.println("[CLOUD] No daily data for today yet — starting from local values.");
            cloudCountersRestored = true;
            https.end();
            cloudRecoveryInProgress = false;
            return;
        }
        JsonObject today_data = days[0];
        String recordedAt = today_data["date"] | "";
        float cloudGridImport       = today_data["grid_import_kwh"]        | 0.0f;
        float cloudGridExport       = today_data["grid_export_kwh"]        | 0.0f;
        float cloudPV               = today_data["pv_energy_kwh"]          | 0.0f;
        float cloudBatteryCharge    = today_data["battery_charge_kwh"]     | 0.0f;
        float cloudBatteryDischarge = today_data["battery_discharge_kwh"]  | 0.0f;
        float cloudLoad             = today_data["load_energy_kwh"]        | 0.0f;
        
        float localGridImport = inverter.gsx.dailyEstimatedGridKWh;
        float localPV = inverter.gsx.dailyPVInputKWh;
        float localBatteryCharge = inverter.gsx.dailyBatteryChargeKWh;
        float localBatteryDischarge = inverter.gsx.dailyBatteryDischargeKWh;
        float localLoad = inverter.gsx.dailyOutputEnergyKWh;

        bool cloudIsToday = recordedAt == String(todayStr);
        if (cloudIsToday)
        {
          xSemaphoreTake(cqMutex, portMAX_DELAY);
          inverter.gsx.dailyEstimatedGridKWh = cloudGridImport + localGridImport;
          inverter.gsx.dailyPVInputKWh = cloudPV + localPV;
          inverter.gsx.dailyBatteryChargeKWh = cloudBatteryCharge + localBatteryCharge;
          inverter.gsx.dailyBatteryDischargeKWh = cloudBatteryDischarge + localBatteryDischarge;
          inverter.gsx.dailyOutputEnergyKWh = cloudLoad + localLoad;
          if (deltaInitialised) {
              lastUploadedGridKWh  += cloudGridImport;
              lastUploadedPvKWh    += cloudPV;
              lastUploadedBattCKWh += cloudBatteryCharge;
              lastUploadedBattDKWh += cloudBatteryDischarge;
              lastUploadedLoadKWh  += cloudLoad;
          }
          xSemaphoreGive(cqMutex);
          cloudCountersRestored = true;
          Serial.println("[CLOUD] Energy counters restored.");
          Serial.printf("[ENERGY] Grid: %.3f + %.3f = %.3f kWh\n",cloudGridImport, localGridImport, inverter.gsx.dailyEstimatedGridKWh);
          Serial.printf("[ENERGY] PV: %.3f + %.3f = %.3f kWh\n", cloudPV, localPV, inverter.gsx.dailyPVInputKWh);
          Serial.printf("[ENERGY] Battery charge: %.3f + %.3f = %.3f kWh\n", cloudBatteryCharge, localBatteryCharge, inverter.gsx.dailyBatteryChargeKWh);
          Serial.printf("[ENERGY] Battery discharge: %.3f + %.3f = %.3f kWh\n", cloudBatteryDischarge, localBatteryDischarge, inverter.gsx.dailyBatteryDischargeKWh);
          Serial.printf("[ENERGY] Load: %.3f + %.3f = %.3f kWh\n", cloudLoad, localLoad, inverter.gsx.dailyOutputEnergyKWh);
        }
        else
        {
          // Not a bug to retry here - the cloud simply has no record for
          // today yet, so there's nothing to merge. Resolve the flag so we
          // don't keep hitting the endpoint every 10s for the rest of the day.
          cloudCountersRestored = true;
          Serial.println("[CLOUD] Cloud data is NOT from today - starting today's counters from local values.");
        }
    }
    else
    {
        Serial.printf("[CLOUD] Recovery failed - HTTP %d\n", httpCode);
        if (httpCode > 0)
        {
            Serial.println("[CLOUD] Response: " + https.getString());
        }
    }
    https.end();
    cloudRecoveryInProgress = false;
}

// Elapsed-time based replacement for the old "upload on wall-clock second
// 14/29/44/59" hack (which was also never actually called - startCloudUpload()
// was commented out). Call every loop(); non-blocking, just a millis() check.
void maybeStartCloudUpload()
{
    //if (!cloudCountersRestored)
    //    return; // don't upload until we know whether today's cloud data needs merging first
    unsigned long now = millis();
    if (now - lastCloudUploadAttempt < intervalCloudUpdate)
        return;
    lastCloudUploadAttempt = now;
    startCloudUpload();
}

// restoreEnergyCountersFromCloud() does a blocking HTTPS GET (up to a few
// seconds on a slow/absent connection). Running it directly from loop() would
// freeze WiFi handling, OTA, the web server and inverter polling for that
// whole time, so it runs in its own FreeRTOS task, the same way uploads do.
void cloudRestoreTask(void *parameter)
{
    restoreEnergyCountersFromCloud();
    cloudRecoveryInProgress = false;
    vTaskDelete(NULL);
}

void startCloudRestoreTask()
{
    if (cloudCountersRestored || cloudRecoveryInProgress)
        return;
    if (WiFi.status() != WL_CONNECTED)
        return;
    BaseType_t result = xTaskCreate(cloudRestoreTask, "CloudRestore", 8192, NULL, 1, NULL);
    if (result != pdPASS)
    {
        cloudRecoveryInProgress = false;
        Serial.println("[CLOUD] Failed to create restore task.");
    }
}

// Called every loop(). Cheap no-op once cloudCountersRestored is true; before
// that, retries every CLOUD_RESTORE_RETRY_INTERVAL (handles: WiFi not
// connected yet at boot, NTP time not synced yet, or a transient HTTP
// failure) until the restore is conclusively resolved either way.
void maybeAttemptCloudRestore()
{
    if (cloudCountersRestored)
        return;
    if (!cloudConfigured()) {
        cloudCountersRestored = true;   // nothing to restore from - local counters run on their own
        return;
    }
    unsigned long now = millis();
    if (now - lastCloudRestoreAttempt < CLOUD_RESTORE_RETRY_INTERVAL)
        return;
    lastCloudRestoreAttempt = now;
    startCloudRestoreTask();
}

