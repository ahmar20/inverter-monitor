#pragma once
// ============================================================================
// wifi_manager.h — saved WiFi profiles (NVS-backed + permanent compiled-in
// fallback), non-blocking scan/connect state machine, and the /home-facing
// scan results. Drive this from loop() via manageWiFi().
// ============================================================================

#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include "config.h"
#include "led_control.h"

struct WifiProfile { String ssid; String password; bool isFallback = false; uint8_t bssid[6] = {0,0,0,0,0,0}; };
WifiProfile wifiProfiles[WIFI_MAX_PROFILES];

String bssidToHex(const uint8_t* b) {
    char buf[13];
    snprintf(buf, sizeof(buf), "%02X%02X%02X%02X%02X%02X", b[0], b[1], b[2], b[3], b[4], b[5]);
    return String(buf);
}
void hexToBSSID(const String& hex, uint8_t* out) {
    if (hex.length() != 12) { memset(out, 0, 6); return; }
    for (int i = 0; i < 6; i++) out[i] = (uint8_t)strtoul(hex.substring(i * 2, i * 2 + 2).c_str(), nullptr, 16);
}

uint8_t wifiProfileCount = 0;
int selectedWiFi = -1;
int currentWiFi = -1;

struct WifiScanResult { String ssid; int rssi; bool secure; };
WifiScanResult wifiScanResults[WIFI_SCAN_MAX_RESULTS];
uint8_t wifiScanResultCount = 0;
bool homeScanReady = false;

bool wifiScanning = false;
bool wifiConnecting = false;
uint8_t wifiScanFailStreak = 0;
bool apModeActive = false;
bool wifiScanRequested = false;
unsigned long lastRoamCheck = 0;
bool roamCheckPending = false;   // true when the current scan was started to check for a stronger sibling AP

unsigned long wifiConnectStart = 0;
unsigned long lastWifiAttempt = 0;
unsigned long wifiScanStart = 0;
unsigned long lastWiFiScan = 0;
const unsigned long WIFI_SCAN_INTERVAL = 30000;   // Rescan every 30 sec
const unsigned long WIFI_SCAN_TIMEOUT  = 10000;   // Scan timeout
const unsigned long WIFI_RETRY_INTERVAL = 10000;  // Try every 10 seconds
const unsigned long WIFI_CONNECT_TIMEOUT = 15000; // Give each attempt 15 seconds

void startLocalWiFiScan();
void processWiFiScan();
void manageWiFi();
void startConfigAP();
void stopConfigAP();
void migrateFallbackCredentialsToNVS();
void disconnectWiFi();
void loadWifiProfilesIntoRAM();

const char* getCurrentSSID() {
    if (currentWiFi < 0 || currentWiFi >= wifiProfileCount) return "";
    return wifiProfiles[currentWiFi].ssid.c_str();
}
const char* getCurrentPassword() {
    if (currentWiFi < 0 || currentWiFi >= wifiProfileCount) return "";
    return wifiProfiles[currentWiFi].password.c_str();
}

// -- Seup WIFI ---------------------------------------------------
void setupWIFi()
{
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.setAutoReconnect(false);
    migrateFallbackCredentialsToNVS();  // one-time: copy compiled-in fallback creds into NVS
    loadWifiProfilesIntoRAM();
    wifiConnecting = false;
    wifiScanning = false;
    wifiScanFailStreak = 0;
    lastWiFiScan = millis() - WIFI_SCAN_INTERVAL;
    // Start first scan
    startLocalWiFiScan();
}

// ── NVS persistence ─────────────────────────────────────────────
String loadWifiProfilesNVS() {
    Preferences prefs;
    prefs.begin("inverter", true);
    String s = prefs.getString("wifiProfiles", "[]");
    prefs.end();
    return s;
}

void saveWifiProfilesNVS(const String& json) {
    Preferences prefs;
    prefs.begin("inverter", false);
    prefs.putString("wifiProfiles", json);
    prefs.end();
}

// Persists ONLY the user-saved (non-fallback) profiles to NVS.
void persistWifiProfilesFromRAM() {
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();
    for (uint8_t i = 0; i < wifiProfileCount; i++) {
        if (wifiProfiles[i].isFallback) continue;  // never write fallbacks to NVS
        JsonObject o = arr.add<JsonObject>();
        o["ssid"] = wifiProfiles[i].ssid;
        o["password"] = wifiProfiles[i].password;
        o["bssid"] = bssidToHex(wifiProfiles[i].bssid);
    }
    String json;
    serializeJson(doc, json);
    saveWifiProfilesNVS(json);
}

// Loads saved profiles from NVS, then ALWAYS appends the compiled-in
// fallback credentials at the end (lowest priority, only tried if nothing
// saved is in range). Fallbacks are never written back to NVS, so wiping
// NVS or deleting every saved profile can never strand the device.
void loadWifiProfilesIntoRAM() {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, loadWifiProfilesNVS());
    wifiProfileCount = 0;
    if (!err && doc.is<JsonArray>()) {
        for (JsonObject p : doc.as<JsonArray>()) {
            if (wifiProfileCount >= WIFI_MAX_PROFILES - 2) break;  // leave room for 2 fallbacks
                        const char* ssid = p["ssid"] | "";
            const char* pass = p["password"] | "";
            const char* bssidHex = p["bssid"] | "";
            if (strlen(ssid) == 0) continue;
            wifiProfiles[wifiProfileCount].ssid = String(ssid);
            wifiProfiles[wifiProfileCount].password = String(pass);
            wifiProfiles[wifiProfileCount].isFallback = false;
            hexToBSSID(String(bssidHex), wifiProfiles[wifiProfileCount].bssid);
            wifiProfileCount++;
        }
    }

    auto appendFallback = [](const char* ssid, const char* pass) {
        if (strlen(ssid) == 0) return;  // fallback blanked out in config.h - nothing to append
        for (uint8_t i = 0; i < wifiProfileCount; i++)
            if (wifiProfiles[i].ssid == ssid) return;  // already saved under this SSID
        
        if (wifiProfileCount < WIFI_MAX_PROFILES) {
            wifiProfiles[wifiProfileCount].ssid = String(ssid);
            wifiProfiles[wifiProfileCount].password = String(pass);
            wifiProfiles[wifiProfileCount].isFallback = true;
            wifiProfileCount++;
        }
    };
    appendFallback(WIFI_FALLBACK_SSID_1, WIFI_FALLBACK_PASS_1);

    Serial.printf("[WIFI] Loaded %u profile(s) (NVS + built-in fallback).\n", wifiProfileCount);
}

// One-time migration: on a device's very first boot with this firmware, copy
// the compiled-in fallback credentials into NVS as regular profiles. Once
// this has run, WIFI_FALLBACK_SSID_1/2 in config.h can be safely blanked out
// on the NEXT flash without losing WiFi access - the credentials already
// live in NVS by then.
void migrateFallbackCredentialsToNVS() {
    Preferences prefs;
    prefs.begin("inverter", true);
    bool migrated = prefs.getBool("wifiMigrated", false);
    prefs.end();
    if (migrated) return;

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, loadWifiProfilesNVS());
    JsonArray arr = (!err && doc.is<JsonArray>()) ? doc.as<JsonArray>() : doc.to<JsonArray>();

    auto alreadySaved = [&](const char* ssid) {
        for (JsonObject p : arr) if (String((const char*)(p["ssid"] | "")) == ssid) return true;
        return false;
    };
    bool changed = false;
    if (strlen(WIFI_FALLBACK_SSID_1) && !alreadySaved(WIFI_FALLBACK_SSID_1)) {
        JsonObject o = arr.add<JsonObject>();
        o["ssid"] = WIFI_FALLBACK_SSID_1;
        o["password"] = WIFI_FALLBACK_PASS_1;
        changed = true;
    }
    if (changed) {
        String json;
        serializeJson(arr, json);
        saveWifiProfilesNVS(json);
        Serial.println("[WIFI] Migrated compiled-in fallback credentials into NVS (first run).");
    }

    Preferences p2;
    p2.begin("inverter", false);
    p2.putBool("wifiMigrated", true);
    p2.end();
}

// ── Setup AP: lets you configure WiFi with zero usable saved profiles ─────
// Opens automatically once startLocalWiFiScan() finds no configured network
// in range WIFI_AP_FAIL_THRESHOLD scans in a row. Connect a phone/laptop to
// this open network, browse to 192.168.4.1/home, and save a real profile.
void startConfigAP() {
    if (apModeActive) return;
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(WIFI_AP_SSID);
    apModeActive = true;
    Serial.println("[WIFI] No known network in range - started open setup AP: " WIFI_AP_SSID);
}

void stopConfigAP() {
    if (!apModeActive) return;
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
    apModeActive = false;
    Serial.println("[WIFI] Connected to a real network - setup AP closed.");
}

// Drops the current connection and forces an immediate rescan (instead of
// waiting up to WIFI_SCAN_INTERVAL) so manageWiFi() picks a network again
// right away rather than sitting disconnected for up to 30s.
void disconnectWiFi() {
    WiFi.disconnect();
    currentWiFi = -1;
    wifiConnecting = false;
    lastWiFiScan = millis() - WIFI_SCAN_INTERVAL;
    Serial.println("[WIFI] Disconnected by user request - rescanning now.");
}

// ── Non-blocking scan / connect state machine ─────────────────────
void manageWiFi() {

    wl_status_t status = WiFi.status();

    // A scan requested from the web UI is only ever flagged here, never
    // started directly from the HTTP handler's task - WiFi.scanNetworks()
    // and WiFi.scanComplete() need to be driven from the same task.
    if (wifiScanRequested && !wifiScanning && !wifiConnecting) {
        wifiScanRequested = false;
        homeScanReady = false;
        startLocalWiFiScan();
    }

    // SCANNING — checked BEFORE the CONNECTED early-return below. Previously
    // this order was reversed, so a scan started while already connected
    // (e.g. the home page listing nearby networks) never got its result
    // processed: wifiScanning stayed stuck true forever and no scan could
    // ever run again afterwards. processWiFiScan() itself now skips the
    // auto-connect logic when we're already connected (see below).
    if (wifiScanning) {
        processWiFiScan();
        return;
    }
    // CONNECTED
    if (status == WL_CONNECTED) {
        if (wifiConnecting) {
            wifiConnecting = false;
            stopConfigAP();   // got onto a real network - drop the setup AP

            // Save whichever BSSID we actually landed on (initial connect or
            // a roam switch) so NVS reflects the AP we're really using, not
            // just the SSID. Fallback profiles are never persisted, matching
            // how their password already isn't.
            if (currentWiFi >= 0 && currentWiFi < wifiProfileCount && !wifiProfiles[currentWiFi].isFallback) {
                memcpy(wifiProfiles[currentWiFi].bssid, WiFi.BSSID(), 6);
                persistWifiProfilesFromRAM();
            }

            Serial.println();
            Serial.println("WiFi connected!");

            Serial.print("SSID: ");
            Serial.println(WiFi.SSID());

            Serial.print("IP address: ");
            Serial.println(WiFi.localIP());

            Serial.print("RSSI: ");
            Serial.print(WiFi.RSSI());
            Serial.println(" dBm");

            setLEDState(LED_NORMAL);
        }

        // Periodic roaming check: look for a meaningfully stronger sibling
        // AP under the same SSID. Independent of any page-triggered scan -
        // this fires on its own timer the whole time we're connected.
        if (millis() - lastRoamCheck >= WIFI_ROAM_CHECK_INTERVAL_MS) {
            lastRoamCheck = millis();
            if (!wifiScanning) {
                roamCheckPending = true;
                startLocalWiFiScan();
            }
        }
        return;
    }
    // CURRENTLY CONNECTING
    if (wifiConnecting) {
        if (millis() - wifiConnectStart >= WIFI_CONNECT_TIMEOUT) {

            Serial.println();
            Serial.println("WiFi connection timeout.");

            WiFi.disconnect();

            wifiConnecting = false;

            setLEDState(LED_WIFI);

            // Immediately start another scan
            startLocalWiFiScan();
        }

        return;
    }
    // NOT CONNECTED
    if (millis() - lastWiFiScan >= WIFI_SCAN_INTERVAL) {
        startLocalWiFiScan();
    }
}

// START WIFI SCAN
void startLocalWiFiScan() {

    if (wifiScanning) {
        return;
    }
    Serial.println();
    Serial.println("Scanning for WiFi networks...");
    WiFi.scanDelete();
    // Non-blocking scan
    WiFi.scanNetworks(true);
    wifiScanning = true;
    wifiScanStart = millis();
}

// PROCESS WIFI SCAN
void processWiFiScan() {
  if (!wifiScanning) return;

  int scanResult = WiFi.scanComplete();
  if (scanResult == WIFI_SCAN_RUNNING) {
    if (millis() - wifiScanStart >= WIFI_SCAN_TIMEOUT) {
      Serial.println("WiFi scan timeout.");
      WiFi.scanDelete();
      wifiScanning = false;
      lastWiFiScan = millis();
      roamCheckPending = false;   // don't let a failed roam-check scan attach to the next unrelated scan
    }
    return;
  }
  if (scanResult == WIFI_SCAN_FAILED) {
    Serial.println("WiFi scan failed.");
    wifiScanning = false;
    lastWiFiScan = millis();
    roamCheckPending = false;
    return;
  }

    int bestRSSI = -999;
  int bestNetwork = -1;
  uint8_t bestBSSID[6] = {0};
  int32_t bestChannel = 0;
  wifiScanResultCount = 0;

  for (int i = 0; i < scanResult; i++) {
    String ssid = WiFi.SSID(i);
    int rssi = WiFi.RSSI(i);
    uint8_t* bssid = WiFi.BSSID(i);
    int32_t channel = WiFi.channel(i);
    Serial.printf("Found: %s  RSSI: %d  BSSID: %02X:%02X:%02X:%02X:%02X:%02X  Ch: %d\n",
                  ssid.c_str(), rssi,
                  bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5], channel);

    if (wifiScanResultCount < WIFI_SCAN_MAX_RESULTS) {
      wifiScanResults[wifiScanResultCount].ssid   = ssid;
      wifiScanResults[wifiScanResultCount].rssi   = rssi;
      wifiScanResults[wifiScanResultCount].secure = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
      wifiScanResultCount++;
    }
    for (uint8_t p = 0; p < wifiProfileCount; p++) {
      // The same SSID can be broadcast by several APs/extenders on one
      // network - always track the specific radio (BSSID) with the
      // strongest signal, not just "any AP with this SSID". WiFi.begin()
      // below is pinned to this exact BSSID+channel for that reason.
      if (ssid == wifiProfiles[p].ssid && rssi > bestRSSI) {
        bestRSSI = rssi;
        bestNetwork = p;
        memcpy(bestBSSID, bssid, 6);
        bestChannel = channel;
      }
    }
  }
  homeScanReady = true;

    // If we're already connected, this scan refreshes the home page's network
  // list - and, when it was started by the periodic roam-check timer rather
  // than a page click, also looks for a meaningfully stronger sibling AP
  // under the same SSID and switches to it if one's found.
  if (WiFi.status() == WL_CONNECTED) {
    wifiScanning = false;
    lastWiFiScan = millis();

    if (roamCheckPending) {
        roamCheckPending = false;
        String curSSID = WiFi.SSID();
        uint8_t curBSSID[6];
        memcpy(curBSSID, WiFi.BSSID(), 6);
        int curRSSI = WiFi.RSSI();

        int bestRoamRSSI = curRSSI;
        int bestRoamIdx = -1;
        for (int i = 0; i < scanResult; i++) {
            if (WiFi.SSID(i) != curSSID) continue;
            uint8_t* candBSSID = WiFi.BSSID(i);
            if (memcmp(candBSSID, curBSSID, 6) == 0) continue;   // the AP we're already on
            if (WiFi.RSSI(i) > bestRoamRSSI) {
                bestRoamRSSI = WiFi.RSSI(i);
                bestRoamIdx = i;
            }
        }

        if (bestRoamIdx >= 0 && (bestRoamRSSI - curRSSI) >= WIFI_ROAM_RSSI_MARGIN) {
            uint8_t roamBSSID[6];
            memcpy(roamBSSID, WiFi.BSSID(bestRoamIdx), 6);
            int32_t roamChannel = WiFi.channel(bestRoamIdx);
            Serial.printf("[WIFI] Roaming: sibling AP %02X:%02X:%02X:%02X:%02X:%02X at %d dBm beats current %d dBm by >=%d - switching\n",
                          roamBSSID[0], roamBSSID[1], roamBSSID[2], roamBSSID[3], roamBSSID[4], roamBSSID[5],
                          bestRoamRSSI, curRSSI, WIFI_ROAM_RSSI_MARGIN);
            WiFi.disconnect();
            WiFi.begin(curSSID.c_str(), wifiProfiles[currentWiFi].password.c_str(), roamChannel, roamBSSID, true);
            wifiConnecting = true;
            wifiConnectStart = millis();
            setLEDState(LED_WIFI);
            WiFi.scanDelete();
            return;
        }
    }

    WiFi.scanDelete();
    return;
  }

  if (bestNetwork == -1) {
    Serial.println("No configured WiFi network found.");
    wifiScanning = false;
    lastWiFiScan = millis();
    WiFi.scanDelete();
    setLEDState(LED_WIFI);

    // After enough consecutive misses, open a setup AP so the device can
    // still be configured even with zero usable saved profiles.
    if (++wifiScanFailStreak >= WIFI_AP_FAIL_THRESHOLD) {
        startConfigAP();
    }
    return;
  }

    wifiScanFailStreak = 0;

  Serial.printf("Best WiFi: %s  RSSI: %d  BSSID: %02X:%02X:%02X:%02X:%02X:%02X  Ch: %d\n",
                wifiProfiles[bestNetwork].ssid.c_str(), bestRSSI,
                bestBSSID[0], bestBSSID[1], bestBSSID[2], bestBSSID[3], bestBSSID[4], bestBSSID[5], bestChannel);
  selectedWiFi = bestNetwork;
  wifiScanning = false;
  lastWiFiScan = millis();
  WiFi.scanDelete();

  Serial.print("Connecting to: ");
  Serial.println(wifiProfiles[selectedWiFi].ssid);
  // Pin the connection to the specific AP/extender we picked above -
  // WiFi.begin(ssid, pass) alone would leave the actual AP choice up to the
  // driver's own internal scan/selection, which is how this ended up on a
  // farther extender despite a nearer one sharing the same SSID.
  WiFi.begin(wifiProfiles[selectedWiFi].ssid.c_str(), wifiProfiles[selectedWiFi].password.c_str(),
             bestChannel, bestBSSID, true);
  currentWiFi = selectedWiFi;
  wifiConnecting = true;
  wifiConnectStart = millis();
  setLEDState(LED_WIFI);
}
