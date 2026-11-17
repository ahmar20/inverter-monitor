// ============================================================================
// main.cpp — entry point only. All logic now lives under include/:
//   config.h, types.h, utils.h, debug_log.h          - shared state & helpers
//   led_control.h, wifi_manager.h                    - LED + WiFi management
//   inverter_protocol.h                              - PI18 comms + parsing
//   cloud_queue.h, cloud_restore.h                   - telemetry upload/restore
//   schedules.h                                      - time-of-day automation
//   api_handlers.h, web_server.h, web_pages/*.h      - HTTP surface
// setup()/loop() below just sequences calls into those modules.
// ============================================================================
#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <FastLED.h>
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#include "config.h"
#include "types.h"
#include "utils.h"
#include "debug_log.h"
#include "led_control.h"
#include "wifi_manager.h"
#include "inverter_protocol.h"
#include "cloud_queue.h"
#include "cloud_restore.h"
#include "schedules.h"
#include "web_server.h"

void setup()
{
  Serial.begin(115200);
  Serial.println("\nStarting Inverter Monitor...");

  // RGB LED
  setupRGBLed();

  // WiFi
  setupWIFi();

  // Web Server
  setupWebServer();
  Serial.println("Async web server started.");

  // ESP OTA
  ElegantOTA.begin(&server);
  ElegantOTA.setAuth(SETTINGS_USER, SETTINGS_PASS);

  // Inverter Serial
  Serial.print("\nInitializing Inverter Serial...");
  INV_Serial.begin(UART_Baud, SERIAL_8N1, rxPin, txPin);
  Serial.println(" Done.");

  // Time
  Serial.print("Setting up SNTP configuration for time sync...");
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.println(" Done.");

  // Schedules — load from NVS; if empty, cloud fetch will happen after WiFi up
  schedulesJson = loadSchedulesNVS();
  Serial.println("[SCHED] Loaded from NVS: " + schedulesJson.substring(0, 80));

  cloudQueueInit();
  loadCloudConfigNVS();
}

void loop()
{
  manageWiFi();
  if (pendingReboot && (long)(millis() - pendingRebootAt) >= 0) {
    ESP.restart();
  }
  updateStatusLED();
  ElegantOTA.loop();
  processInverterResponse();
  checkInverterConnectivity();
  checkWifiConnectivity();
  updateEnergyCounters();
  checkDailyEnergyReset();

  // =========== Cloud restore-on-boot (today's data only) ===========
  maybeAttemptCloudRestore();

  // =========== Cloud Upload ===========
  maybeStartCloudUpload();

  // =========== Schedule cloud fetch (once after first WiFi connect) ===========
  maybeAttemptScheduleFetch();

   // =========== Boot-time missed schedule catch-up (runs once after NTP ready) ===========
  if (!missedSchedulesChecked) checkMissedSchedules();

  // =========== Scheduled settings automation ===========
  // checkAndFireSchedules() runs once per minute, populates schedCmdQueue[].
  checkAndFireSchedules();
  // Drain queue: fire one command per loop() when inverter is free.
  if (!webCmdPending && !commandInProcess && schedCmdQueueIdx < schedCmdQueueLen) {
      String cmdStr = schedCmdQueue[schedCmdQueueIdx++];
      webCmdLen         = buildWriteCommand(cmdStr, webCmdBuf);
      webCmdString      = cmdStr;
      webCmdPending     = true;
      webCmdDone        = false;
      webCmdSuccess     = false;
      webCmdSent        = false;
      webCmdResponse    = "";
      webCmdSentAt      = millis();
      webCmdIsScheduled = true;
      dbgLog("[SCHED] Sending: %s (%u bytes)", cmdStr.c_str(), webCmdLen);
  }

  // =========== Inverter Commands ===========
  // Non-blocking round-robin: one command is ever "in flight" at a time.
  // pollInverterCommands() only ever *starts* a command; the actual read of
  // the reply happens in processInverterResponse() above, spread over many
  // loop() iterations. This replaces the previous chained
  // startInverterCommand(A) && startInverterCommand(B) && ... call, which
  // looked like it queued every command but actually only ever sent the
  // first one: each function short-circuited on the very next call because
  // commandInProcess was already true from the command started microseconds
  // earlier in the same expression, so B/C/D/E/F never had a chance to run.
  pollInverterCommands();

  // Dispatch web-triggered command when inverter is free
  if (webCmdPending && !commandInProcess && !webCmdDone) {
      // Log hex frame being sent
      char hexLine[DBG_LINE_LEN];
      int pos = snprintf(hexLine, sizeof(hexLine), "[WEB] Sending %u bytes:", webCmdLen);
      for (size_t i = 0; i < webCmdLen && pos < (int)sizeof(hexLine) - 4; i++)
          pos += snprintf(hexLine + pos, sizeof(hexLine) - pos, " %02X", webCmdBuf[i]);
      dbgLog("%s", hexLine);
      webCmdSent = true;
      sendCommand(webCmdBuf, webCmdLen);
  }
}
