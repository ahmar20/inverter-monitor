#pragma once
// ============================================================================
// config.h — compile-time configuration: auth, network endpoints, timing
// constants, and pin assignments. No logic lives here, only constants.
//
// Credentials and the one-time cloud seed live in secrets.h (gitignored)
// instead of here - copy secrets.h.example to secrets.h and fill in your own
// values before building. See: SETTINGS_USER/SETTINGS_PASS,
// WIFI_FALLBACK_SSID_1/PASS_1, CLOUD_SEED_URL/CLOUD_SEED_KEY.
// ============================================================================

#include <Arduino.h>
#include "secrets.h"

// ===== WIFI =====
#define WIFI_MAX_PROFILES 8
#define WIFI_SCAN_MAX_RESULTS 20

#define WIFI_AP_SSID "InverterMonitor-Setup"   // open setup AP, no password
#define WIFI_AP_FAIL_THRESHOLD 2               // consecutive failed scans (~1 min) before the setup AP opens

#define WIFI_ROAM_CHECK_INTERVAL_MS 60000UL    // how often to re-check for a stronger sibling AP while connected
#define WIFI_ROAM_RSSI_MARGIN       10         // only roam if a sibling BSSID is at least this many dB stronger

// ===== CLOUD DB =====
// CLOUD_SEED_URL/CLOUD_SEED_KEY (the actual endpoint + API key) live in
// secrets.h - they're only a one-time seed copied into NVS on first boot.
// After that, change credentials at runtime from the "Cloud Upload" section
// on /settings instead of editing secrets.h. Leave them blank in secrets.h
// (or omit the device entirely) to run with cloud upload disabled - the
// local flash queue and /history's local fallback work either way.
const unsigned long intervalCloudUpdate = 15000UL; // 15 sec
const String inverterIDForDB = "infinisolar-5kw-01";  // pick your own device id
const unsigned long CLOUD_RESTORE_RETRY_INTERVAL = 10000UL; // retry every 10s until it succeeds/skips

// ===== NTP =====
static const char* ntpServer = "pool.ntp.org";
static const long  gmtOffset_sec = 18000;   // adjust to your timezone
static const int   daylightOffset_sec = 0;

// ===== Coulomb counting SOC =====
#define CC_CAPACITY_WH_DEFAULT 4000.0f  // Usable battery capacity in Wh
#define CC_CUTOFF_THRESHOLD_DEFAULT 10  // consecutive readings to consider cutoff sustained

// ===== Inverter polling cadence =====
// Target time for one full sweep through every polled command (GSX, PIRI, MOD,
// FLAG, ACCT, ACLT). Every command is polled once per sweep - there is no more
// per-command interval. If a full sweep consistently takes longer than this
// (visible via the "[POLL] cycle" log line), raise this value to match reality.
#define INTERVAL_INVERTER_POLLING    1000UL

// A command that times out / gets rejected this many times in a row is put on a
// temporary backoff so a persistently unresponsive command (e.g. ACLT) can't
// keep slowing down every other command's cadence.
#define POLL_MAX_CONSECUTIVE_FAILURES   5
#define POLL_BACKOFF_DURATION_MS        30000UL

// ===== Inverter UART =====
#define INV_Serial Serial2
static const unsigned long UART_Baud = 2400;
static const uint8_t txPin = 16;
static const uint8_t rxPin = 17;
static const unsigned long inverterReadIntervalMs = 1000;
static const unsigned long COMMAND_TIMEOUT = 3000;

// ===== RGB LED =====
static const uint8_t ledPin = 48;
static const int numLEDs = 1;
static const int ledBrightness = 10;
#define LED_Type NEOPIXEL

// ===== Debug log ring buffer (visible at /api/serial) =====
#define DBG_MAX_LINES  20
#define DBG_LINE_LEN  160

// ===== Scheduled command queue =====
#define SCHED_CMD_QUEUE_MAX 16

// ===== Inverter connectivity log =====
#define INV_DOWN_THRESHOLD_MS 5000UL   // only log a gap this long or longer
#define INV_LOG_MAX 32
