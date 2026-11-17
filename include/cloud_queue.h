#pragma once
// ============================================================================
//  cloud_queue.h  -  store-and-forward upload queue  (RAM  ->  flash)
//
//  Every 15 s a SAMPLE (timestamp + that interval's energy DELTAS + instant
//  values) is put into a queue.  A background task sends the queue oldest-first.
//
//    tier 1  RAM ring   ~1 h of samples.  Used first.
//    tier 2  flash      when the RAM ring is (nearly) full, the whole ring is
//                       written to one LittleFS chunk file and the ring is
//                       emptied.  Repeats for as long as the outage lasts.
//
//  Sending order is always: oldest flash chunk -> ... -> newest flash chunk
//  -> RAM ring.  Each sample keeps its ORIGINAL timestamp and its OWN delta,
//  so recovery uploads many small normal-looking records, never one lump.
//
//  Part of the include/ module split - see config.h, types.h, utils.h.
// ============================================================================
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <LittleFS.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include "config.h"
#include "types.h"
#include "utils.h"

// ---- one queued sample: 40 bytes ------------------------------------------
struct CloudRecord {
    uint32_t ts;                 // epoch seconds (UTC) when the sample was taken
    float    pvKWh;              // energy DELTAS since the previous sample
    float    gridKWh;
    float    battChKWh;
    float    battDisKWh;
    float    loadKWh;
    int16_t  gridPower;          // W
    int16_t  pvPower;            // W
    int16_t  battPower;          // W  (+charge / -discharge)
    int16_t  loadPower;          // W
    uint16_t pvVolt10;           // V x 10
    uint16_t battVolt100;        // V x 100
    uint8_t  battPct;            // %
    int8_t   calcSoc;            // %  (-1 = coulomb counter not initialised)
};
static_assert(sizeof(CloudRecord) == 40, "CloudRecord size changed - old flash chunks would be unreadable");

// ---- tunables ---------------------------------------------------------------
#define CQ_RAM_TARGET_RECORDS   240          // 1 h at one sample / 15 s
#define CQ_SPILL_HEADROOM       20           // spill to flash when this close to full (~5 min)
#define CQ_PSRAM_RESERVE_BYTES  262144       // PSRAM always left untouched for other uses of PSRAM
#define CQ_HEAP_RESERVE_BYTES   70000        // internal-heap fallback only: heap that must stay free for WiFi + TLS + web server
#define CQ_ALLOW_INTERNAL_FALLBACK 1         // 1 = use internal heap if there is no PSRAM; 0 = never touch internal RAM
#define CQ_DIR                  "/q"
#define CQ_FLASH_USE_PERCENT    80           // queue may use at most this % of the LittleFS partition
#define CQ_DRAIN_MAX_MS         8000UL       // one upload task never drains longer than this
#define CQ_SPILL_RETRY_MS       60000UL      // if a flash write fails, wait this long before trying again
#define CQ_CHUNK_SLACK_BYTES    8192         // LittleFS block rounding + metadata

// ---- state ------------------------------------------------------------------
static CloudRecord*      cqRam            = nullptr;
static uint16_t          cqRamCap         = 0;
static bool              cqRamInPsram     = false; // true = ring buffer lives in PSRAM
static uint16_t          cqRamHead        = 0;   // oldest record. Only the upload task moves this.
static uint16_t          cqRamCount       = 0;   // records in RAM (changed under cqMutex)
static SemaphoreHandle_t cqMutex          = nullptr;

static bool              cqFlashOk        = false;
static uint32_t          cqFlashHeadSeq   = 0;   // oldest chunk file  /q/<seq>.q1
static uint32_t          cqFlashNextSeq   = 0;   // next chunk file to create
static uint32_t          cqFlashHeadOff   = 0;   // records already sent from the oldest chunk
static uint32_t          cqFlashRecords   = 0;   // records on flash still waiting to be sent
static uint32_t          cqDropped        = 0;   // records lost (queue full / flash full / rejected by server)
static unsigned long     cqLastSpillTry   = 0;

String cloudUpdateUrl = "";
String cloudApiKey    = "";

bool cloudConfigured() { return cloudApiKey.length() > 0 && cloudUpdateUrl.length() > 0; }

void loadCloudConfigNVS() {
    Preferences prefs;
    prefs.begin("inverter", true);
    bool migrated = prefs.getBool("cloudMigrated", false);
    cloudUpdateUrl = prefs.getString("cloudUrl", "");
    cloudApiKey    = prefs.getString("cloudKey", "");
    prefs.end();

    if (!migrated) {
        Preferences p2;
        p2.begin("inverter", false);
        if (strlen(CLOUD_SEED_URL) && strlen(CLOUD_SEED_KEY)) {
            cloudUpdateUrl = CLOUD_SEED_URL;
            cloudApiKey    = CLOUD_SEED_KEY;
            p2.putString("cloudUrl", cloudUpdateUrl);
            p2.putString("cloudKey", cloudApiKey);
            Serial.println("[CLOUD] Migrated compiled-in credentials into NVS (first run).");
        }
        p2.putBool("cloudMigrated", true);
        p2.end();
    }
    Serial.println(cloudConfigured()
        ? "[CLOUD] Credentials loaded - upload enabled."
        : "[CLOUD] No credentials configured - upload disabled, queue still buffers locally.");
}

void saveCloudConfigNVS(const String& url, const String& key) {
    cloudUpdateUrl = url;
    cloudApiKey    = key;
    Preferences prefs;
    prefs.begin("inverter", false);
    prefs.putString("cloudUrl", url);
    prefs.putString("cloudKey", key);
    prefs.putBool("cloudMigrated", true);
    prefs.end();
}

void handleCloudConfigSet(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
    if (index == 0) {
        if (request->_tempObject) free(request->_tempObject);
        request->_tempObject = malloc(total + 1);
    }
    if (!request->_tempObject) {
        request->send(500, "application/json", "{\"status\":\"error\",\"message\":\"Out of memory\"}");
        return;
    }
    memcpy((uint8_t*)request->_tempObject + index, data, len);
    if (index + len != total) return;

    ((char*)request->_tempObject)[total] = '\0';
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, (char*)request->_tempObject);
    free(request->_tempObject);
    request->_tempObject = nullptr;
    if (err) {
        request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Bad JSON\"}");
        return;
    }
    String url = doc["url"] | "";
    String key = doc["key"] | "";
    if (url.length() > 0 && key.length() == 0) {
        request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Enter both URL and key, or clear both to disable\"}");
        return;
    }
    if (url.length() == 0 && key.length() > 0) {
        request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Enter both URL and key, or clear both to disable\"}");
        return;
    }
    saveCloudConfigNVS(url, key);
    Serial.println(cloudConfigured()
        ? "[CLOUD] Credentials updated via /settings."
        : "[CLOUD] Credentials cleared via /settings - upload disabled.");
    request->send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Saved\"}");
}

#define CQ_LOCK()    xSemaphoreTake(cqMutex, portMAX_DELAY)
#define CQ_UNLOCK()  xSemaphoreGive(cqMutex)

// ---- small helpers ----------------------------------------------------------
static int16_t cqClamp16(float v)
{
    if (v >  32767.0f) return  32767;
    if (v < -32768.0f) return -32768;
    return (int16_t)v;
}

static String cqChunkPath(uint32_t seq)
{
    char p[32];
    snprintf(p, sizeof(p), CQ_DIR "/%lu.q1", (unsigned long)seq);
    return String(p);
}

static uint32_t cqPendingTotal() { return (uint32_t)cqRamCount + cqFlashRecords; }

static size_t cqFlashBudget() { return (LittleFS.totalBytes() / 100) * CQ_FLASH_USE_PERCENT; }

// ============================================================================
//  INIT  -  call once at the END of setup()
//  Sizes the RAM tier from the heap that is really free, mounts LittleFS and
//  picks up any chunk files left over from before a reboot.
// ============================================================================
void cloudQueueInit()
{
    cqMutex = xSemaphoreCreateMutex();

    // ---- RAM tier: lives in PSRAM so the fast internal heap stays free for WiFi / TLS / web server ----
    const size_t rec  = sizeof(CloudRecord);
    const size_t want = (size_t)CQ_RAM_TARGET_RECORDS * rec;
    cqRam = nullptr;  cqRamCap = 0;  cqRamInPsram = false;  cqRamHead = 0;  cqRamCount = 0;

    if (psramFound()) {
        const size_t pBlock = ESP.getMaxAllocPsram();
        const size_t pUsable = (pBlock > CQ_PSRAM_RESERVE_BYTES) ? (pBlock - CQ_PSRAM_RESERVE_BYTES) : 0;
        const size_t pBytes  = (want < pUsable) ? want : pUsable;
        cqRamCap = (uint16_t)(pBytes / rec);
        if (cqRamCap > 0) cqRam = (CloudRecord*)ps_malloc((size_t)cqRamCap * rec);
        cqRamInPsram = (cqRam != nullptr);
        if (!cqRam) cqRamCap = 0;
        Serial.printf("[QUEUE] PSRAM: %u KB total, %u KB free, largest block %u KB\n",
                      (unsigned)(ESP.getPsramSize() / 1024), (unsigned)(ESP.getFreePsram() / 1024), (unsigned)(pBlock / 1024));
    } else {
        Serial.println("[QUEUE] No PSRAM detected.");
    }
#if CQ_ALLOW_INTERNAL_FALLBACK
    if (!cqRam) {                                // no PSRAM (or it failed): internal heap, but always keep CQ_HEAP_RESERVE_BYTES free
        const size_t block  = ESP.getMaxAllocHeap();
        const size_t usable = (block > CQ_HEAP_RESERVE_BYTES) ? (block - CQ_HEAP_RESERVE_BYTES) : 0;
        const size_t bytes  = (want < usable) ? want : usable;
        cqRamCap = (uint16_t)(bytes / rec);
        cqRam    = (cqRamCap > 0) ? (CloudRecord*)malloc((size_t)cqRamCap * rec) : nullptr;
        if (!cqRam) cqRamCap = 0;
    }
#endif
    Serial.printf("[QUEUE] Internal heap (untouched when PSRAM is used): free %u KB, largest block %u KB\n",
                  (unsigned)(ESP.getFreeHeap() / 1024), (unsigned)(ESP.getMaxAllocHeap() / 1024));
    Serial.printf("[QUEUE] RAM tier: %u records = %u min of data (%u KB) in %s\n",
                  (unsigned)cqRamCap, (unsigned)((uint32_t)cqRamCap * (intervalCloudUpdate / 1000UL) / 60UL),
                  (unsigned)((size_t)cqRamCap * rec / 1024), cqRamInPsram ? "PSRAM" : (cqRam ? "internal heap" : "NOWHERE (no memory)"));
    // ---- flash tier ----
    cqFlashRecords = 0;
    cqFlashOk = LittleFS.begin(true, "/littlefs", 10, "storage");   // partition is labelled "storage" (the default label is "spiffs")
    if (!cqFlashOk) {
        Serial.println("[QUEUE] LittleFS not available - RAM-only buffering (check the partition table).");
        return;
    }
    if (!LittleFS.exists(CQ_DIR)) LittleFS.mkdir(CQ_DIR);
    LittleFS.remove(CQ_DIR "/tmp.bin");          // half-written spill left by a power cut

    uint32_t minSeq = 0xFFFFFFFFUL, nextSeq = 0;
    bool any = false;
    File dir = LittleFS.open(CQ_DIR);
    for (File f = dir.openNextFile(); f; f = dir.openNextFile()) {
        String base = String(f.name());
        base = base.substring(base.lastIndexOf('/') + 1);
        if (!base.endsWith(".q1")) continue;
        const uint32_t seq = (uint32_t)strtoul(base.c_str(), nullptr, 10);
        cqFlashRecords += (uint32_t)(f.size() / sizeof(CloudRecord));
        if (seq < minSeq)      minSeq  = seq;
        if (seq + 1 > nextSeq) nextSeq = seq + 1;
        any = true;
    }
    dir.close();
    cqFlashHeadSeq = any ? minSeq  : 0;
    cqFlashNextSeq = any ? nextSeq : 0;
    cqFlashHeadOff = 0;

    const size_t   budget  = cqFlashBudget();
    const uint32_t cap     = (uint32_t)(budget / rec);
    Serial.printf("[QUEUE] Flash tier: FS %u KB, queue budget %u KB = %u records (~%u h at 15 s)\n",
                  (unsigned)(LittleFS.totalBytes() / 1024), (unsigned)(budget / 1024), (unsigned)cap,
                  (unsigned)((cap * (intervalCloudUpdate / 1000UL)) / 3600UL));
    if (cqFlashRecords)
        Serial.printf("[QUEUE] %u record(s) found on flash from before the reboot - will be uploaded.\n",
                      (unsigned)cqFlashRecords);
}

// ============================================================================
//  PRODUCER  (runs in loop(), every 15 s)
// ============================================================================
static void cqPushLocked(const CloudRecord &r)
{
    if (!cqRam || cqRamCap == 0 || cqRamCount >= cqRamCap) {
        cqDropped++;                             // no room (only when flash is unavailable / failing)
        return;
    }
    cqRam[(uint16_t)((cqRamHead + cqRamCount) % cqRamCap)] = r;
    cqRamCount++;
}

// Records ONE sample.  The energy fields are the DELTA since the previous sample
// (same maths as the old startCloudUpload), but they are now captured here,
// whether or not the network is up, instead of being thrown away on a failed upload.
static bool cqTakeSnapshot()
{
    const time_t nowT = time(nullptr);
    if (nowT < 1700000000L) return false;        // clock not synced yet: no valid timestamp, no baseline move

    CQ_LOCK();                                   // same lock the cloud-restore merge uses
    const float curPv    = inverter.gsx.dailyPVInputKWh;
    const float curGrid  = inverter.gsx.dailyEstimatedGridKWh;
    const float curBattC = inverter.gsx.dailyBatteryChargeKWh;
    const float curBattD = inverter.gsx.dailyBatteryDischargeKWh;
    const float curLoad  = inverter.gsx.dailyOutputEnergyKWh;

    CloudRecord r;
    memset(&r, 0, sizeof(r));
    r.ts = (uint32_t)nowT;

    if (!deltaInitialised) {
        deltaInitialised = true;                 // first sample only sets the baseline
    } else {
        // cur < last  ->  the midnight reset happened in between: delta is just the new value
        r.pvKWh      = (curPv    >= lastUploadedPvKWh)    ? curPv    - lastUploadedPvKWh    : curPv;
        r.gridKWh    = (curGrid  >= lastUploadedGridKWh)  ? curGrid  - lastUploadedGridKWh  : curGrid;
        r.battChKWh  = (curBattC >= lastUploadedBattCKWh) ? curBattC - lastUploadedBattCKWh : curBattC;
        r.battDisKWh = (curBattD >= lastUploadedBattDKWh) ? curBattD - lastUploadedBattDKWh : curBattD;
        r.loadKWh    = (curLoad  >= lastUploadedLoadKWh)  ? curLoad  - lastUploadedLoadKWh  : curLoad;
    }
    lastUploadedPvKWh    = curPv;
    lastUploadedGridKWh  = curGrid;
    lastUploadedBattCKWh = curBattC;
    lastUploadedBattDKWh = curBattD;
    lastUploadedLoadKWh  = curLoad;

    r.gridPower   = cqClamp16(inverter.gsx.estimatedGridPower);
    r.pvPower     = cqClamp16((float)inverter.gsx.pv1InputWatts);
    r.battPower   = cqClamp16(inverter.gsx.batteryChargePower - inverter.gsx.batteryDischargePower);
    r.loadPower   = cqClamp16((float)inverter.gsx.outputWatts);
    r.pvVolt10    = (uint16_t)constrain((int)lroundf(inverter.gsx.pv1InputVoltage * 10.0f), 0, 65535);
    r.battVolt100 = (uint16_t)constrain((int)lroundf(inverter.gsx.batteryVoltage * 100.0f), 0, 65535);
    r.battPct     = (uint8_t)constrain(inverter.gsx.batteryPercentage, 0, 100);
    r.calcSoc     = (int8_t)constrain((int)lroundf(ccSoc), -1, 100);

    cqPushLocked(r);
    CQ_UNLOCK();
    return true;
}

// ============================================================================
//  CONSUMER  (runs in the upload task - the only code that touches flash)
// ============================================================================

// Delete the oldest chunk file.  countDropped = true when it still held unsent data.
static void cqFlashDropHead(bool countDropped)
{
    const String p = cqChunkPath(cqFlashHeadSeq);
    uint32_t n = 0;
    File f = LittleFS.open(p.c_str(), "r");
    if (f) { n = (uint32_t)(f.size() / sizeof(CloudRecord)); f.close(); }
    LittleFS.remove(p.c_str());
    const uint32_t left = (n > cqFlashHeadOff) ? (n - cqFlashHeadOff) : 0;
    cqFlashRecords = (cqFlashRecords > left) ? (cqFlashRecords - left) : 0;
    if (countDropped) cqDropped += left;
    cqFlashHeadSeq++;
    cqFlashHeadOff = 0;
}

// RAM ring nearly full  ->  write the whole ring to ONE flash chunk and empty it.
// Works with WiFi down (that is the whole point).
static void cqSpillIfNeeded()
{
    if (!cqFlashOk || !cqRam || cqRamCap == 0) return;

    CQ_LOCK();
    const uint16_t head  = cqRamHead;
    const uint16_t count = cqRamCount;
    CQ_UNLOCK();

    if ((uint32_t)count + CQ_SPILL_HEADROOM < cqRamCap) return;                       // still room in RAM
    if (cqLastSpillTry && (millis() - cqLastSpillTry) < CQ_SPILL_RETRY_MS) return;    // recent failure: back off
    cqLastSpillTry = millis();

    const size_t bytes  = (size_t)count * sizeof(CloudRecord);
    const size_t needed = bytes + CQ_CHUNK_SLACK_BYTES;
    const size_t budget = cqFlashBudget();

    // flash budget reached -> sacrifice the OLDEST chunk (oldest data) to make room for the newest
    while (LittleFS.usedBytes() + needed > budget && cqFlashHeadSeq < cqFlashNextSeq) {
        Serial.printf("[QUEUE] Flash budget reached - dropping oldest chunk #%lu\n", (unsigned long)cqFlashHeadSeq);
        cqFlashDropHead(true);
    }
    if (LittleFS.usedBytes() + needed > budget) {
        Serial.println("[QUEUE] Flash spill skipped - no room even after dropping old chunks.");
        return;
    }

    // Records [head, head+count) are not touched by the producer (it only appends at the tail),
    // so the write can run without holding the mutex.
    File f = LittleFS.open(CQ_DIR "/tmp.bin", "w");
    if (!f) { Serial.println("[QUEUE] Flash spill failed: cannot create file."); return; }
    const size_t first = ((size_t)count < (size_t)(cqRamCap - head)) ? (size_t)count : (size_t)(cqRamCap - head);
    size_t w = f.write((const uint8_t*)&cqRam[head], first * sizeof(CloudRecord));
    if ((size_t)count > first)
        w += f.write((const uint8_t*)&cqRam[0], ((size_t)count - first) * sizeof(CloudRecord));
    f.close();
    if (w != bytes) {
        Serial.printf("[QUEUE] Flash spill failed: wrote %u of %u bytes.\n", (unsigned)w, (unsigned)bytes);
        LittleFS.remove(CQ_DIR "/tmp.bin");
        return;
    }
    const String finalPath = cqChunkPath(cqFlashNextSeq);
    if (!LittleFS.rename(CQ_DIR "/tmp.bin", finalPath.c_str())) {     // rename is atomic: chunk is complete or absent
        Serial.println("[QUEUE] Flash spill failed: rename.");
        LittleFS.remove(CQ_DIR "/tmp.bin");
        return;
    }
    cqFlashNextSeq++;
    cqFlashRecords += count;

    CQ_LOCK();                                   // release exactly the records that were written
    cqRamHead  = (uint16_t)((cqRamHead + count) % cqRamCap);
    cqRamCount = (uint16_t)(cqRamCount - count);
    CQ_UNLOCK();
    cqLastSpillTry = 0;
    Serial.printf("[QUEUE] RAM full - spilled %u records to flash chunk #%lu. Flash now holds %lu records.\n",
                  (unsigned)count, (unsigned long)(cqFlashNextSeq - 1), (unsigned long)cqFlashRecords);
}

// Copy the oldest queued record into 'out' WITHOUT removing it.
static bool cqPeekOldest(CloudRecord &out, bool &fromFlash)
{
    while (cqFlashOk && cqFlashHeadSeq < cqFlashNextSeq) {                  // flash is always older than RAM
        const String p = cqChunkPath(cqFlashHeadSeq);
        File f = LittleFS.open(p.c_str(), "r");
        const size_t n = f ? (f.size() / sizeof(CloudRecord)) : 0;
        if (cqFlashHeadOff < n) {
            f.seek(cqFlashHeadOff * sizeof(CloudRecord));
            if (f.read((uint8_t*)&out, sizeof(out)) == sizeof(out)) {
                fromFlash = true;
                return true;
            }
        }
        if (f) f.close();
        cqFlashDropHead(false);                                             // exhausted, missing or damaged chunk
    }
    fromFlash = false;
    bool have = false;
    CQ_LOCK();
    if (cqRamCount > 0) { out = cqRam[cqRamHead]; have = true; }
    CQ_UNLOCK();
    return have;
}

static void cqPopOldest(bool fromFlash)
{
    if (fromFlash) {
        cqFlashHeadOff++;                                                   // chunk file is deleted lazily by cqPeekOldest
        if (cqFlashRecords > 0) cqFlashRecords--;
    } else {
        CQ_LOCK();
        if (cqRamCount > 0) {
            cqRamHead = (uint16_t)((cqRamHead + 1) % cqRamCap);
            cqRamCount--;
        }
        CQ_UNLOCK();
    }
}

// Same JSON fields / formats as before.  recorded_at comes from the sample's OWN timestamp.
static String cqBuildJson(const CloudRecord &r)
{
    time_t t = (time_t)r.ts;
    struct tm tmv;
    localtime_r(&t, &tmv);
    char iso[32];
    strftime(iso, sizeof(iso), "%Y-%m-%dT%H:%M:%S%z", &tmv);

    char buf[512];
    snprintf(buf, sizeof(buf),
        "{\"device_id\":\"%s\",\"recorded_at\":\"%s\","
        "\"grid_power\":%d,\"grid_import_kwh\":%.3f,\"grid_export_kwh\":0,"
        "\"pv_voltage\":%.2f,\"pv_power\":%d,\"pv_energy_kwh\":%.3f,"
        "\"battery_voltage\":%.2f,\"battery_power\":%d,"
        "\"battery_charge_kwh\":%.3f,\"battery_discharge_kwh\":%.3f,"
        "\"battery_soc\":%d,\"calculated_soc\":%d,"
        "\"load_power\":%d,\"load_energy_kwh\":%.3f}",
        inverterIDForDB.c_str(), iso,
        (int)r.gridPower, (double)r.gridKWh,
        r.pvVolt10 / 10.0, (int)r.pvPower, (double)r.pvKWh,
        r.battVolt100 / 100.0, (int)r.battPower,
        (double)r.battChKWh, (double)r.battDisKWh,
        (int)r.battPct, (int)r.calcSoc,
        (int)r.loadPower, (double)r.loadKWh);
    return String(buf);
}

#define CQ_HIST_READ_RECORDS 256
static CloudRecord cqHistReadBuf[CQ_HIST_READ_RECORDS];

void handleLocalHistory(AsyncWebServerRequest* request)
{
    uint32_t from = 0, to = 0xFFFFFFFFUL;
    if (request->hasParam("from")) from = (uint32_t)request->getParam("from")->value().toInt();
    if (request->hasParam("to"))   to   = (uint32_t)request->getParam("to")->value().toInt();

    AsyncResponseStream *response = request->beginResponseStream("application/json");
    response->addHeader("Cache-Control", "no-cache");
    response->print("[");
    bool first = true;

    if (cqFlashOk) {
        for (uint32_t seq = cqFlashHeadSeq; seq < cqFlashNextSeq; seq++) {
            File f = LittleFS.open(cqChunkPath(seq).c_str(), "r");
            if (!f) continue;
            size_t i = (seq == cqFlashHeadSeq) ? cqFlashHeadOff : 0;
            const size_t n = f.size() / sizeof(CloudRecord);
            if (i > 0) f.seek(i * sizeof(CloudRecord));   // one seek per chunk, not per record - we read sequentially after this
            while (i < n) {
                const size_t want = (n - i < CQ_HIST_READ_RECORDS) ? (n - i) : CQ_HIST_READ_RECORDS;
                size_t got;
                CQ_LOCK();    // only to protect the shared static buffer from a second overlapping request
                got = f.read((uint8_t*)cqHistReadBuf, want * sizeof(CloudRecord)) / sizeof(CloudRecord);
                CQ_UNLOCK();
                if (got == 0) break;    // short read / EOF / error - stop this chunk
                for (size_t k = 0; k < got; k++) {
                    const CloudRecord &r = cqHistReadBuf[k];
                    if (r.ts < from || r.ts > to) continue;
                    if (!first) response->print(",");
                    response->print(cqBuildJson(r));    // network write happens OUTSIDE the lock
                    first = false;
                }
                i += got;
            }
            f.close();
        }
    }

    // Then the RAM ring, oldest first.
    uint16_t head, count, cap;
    CQ_LOCK(); head = cqRamHead; count = cqRamCount; cap = cqRamCap; CQ_UNLOCK();
    for (uint16_t i = 0; i < count; i++) {
        CloudRecord r;
        CQ_LOCK();
        r = cqRam[(uint16_t)((head + i) % cap)];
        CQ_UNLOCK();
        if (r.ts < from || r.ts > to) continue;
        if (!first) response->print(",");
        response->print(cqBuildJson(r));
        first = false;
    }

    response->print("]");
    request->send(response);
}

// 0 = try again later (network / 5xx / 401)   1 = stored   2 = permanently rejected by the server (dropped)
static int cqPostRecord(HTTPClient &https, WiFiClientSecure &client, const CloudRecord &r)
{
    if (!https.begin(client, cloudUpdateUrl)) {
        Serial.println("[CLOUD] HTTPS connection failed.");
        return 0;
    }
    https.addHeader("Content-Type", "application/json");
    https.addHeader("Authorization", String("Bearer ") + cloudApiKey);
    const int code = https.POST(cqBuildJson(r));
    String resp = (code > 0) ? https.getString() : HTTPClient::errorToString(code);   // read the body so the connection can be reused
    https.end();
    inverter.cloud.rawResponse = resp;

    if (code == HTTP_CODE_OK) return 1;

    // The server understood the record and refuses it - retrying can never work and would block
    // everything behind it.  (401/403/5xx/network are NOT in this list: those are retried.)
    if (code == 400 || code == 413 || code == 422) {
        Serial.printf("[CLOUD] Record ts=%lu rejected (HTTP %d) - dropped. %s\n",
                      (unsigned long)r.ts, code, resp.c_str());
        cqDropped++;
        return 2;
    }
    Serial.printf("[CLOUD] Upload failed - HTTP %d (%s)\n", code, resp.c_str());
    return 0;
}

// Send queued records oldest-first over ONE kept-alive TLS connection, until the
// queue is empty, a send fails, or CQ_DRAIN_MAX_MS has passed.
static void cqDrain()
{
    if (!cloudCountersRestored) return;          // today's cloud totals must be merged first or they would double-count
    if (cqPendingTotal() == 0) return;
    if (!cloudConfigured()) return;
    if (WiFi.status() != WL_CONNECTED) {
        Serial.printf("[CLOUD] WiFi down - %lu record(s) queued.\n", (unsigned long)cqPendingTotal());
        return;
    }

    WiFiClientSecure client;
    client.setInsecure();                        // same as the rest of the project
    HTTPClient https;
    https.setReuse(true);
    https.setTimeout(3000);

    const unsigned long t0 = millis();
    uint32_t stored = 0;
    while ((millis() - t0) < CQ_DRAIN_MAX_MS) {
        CloudRecord r;
        bool fromFlash = false;
        if (!cqPeekOldest(r, fromFlash)) break;                 // nothing left
        const int res = cqPostRecord(https, client, r);
        if (res == 0) break;                                    // retry next cycle, record stays queued
        cqPopOldest(fromFlash);
        if (res == 1) stored++;
        if (WiFi.status() != WL_CONNECTED) break;
    }
    if (stored) {
        inverter.cloud.lastSuccessfulCloudUpdate = getISOTime();
        Serial.printf("[QUEUE] Uploaded %lu record(s). Still queued: %lu (RAM %u, flash %lu)\n",
                      (unsigned long)stored, (unsigned long)cqPendingTotal(),
                      (unsigned)cqRamCount, (unsigned long)cqFlashRecords);
    }
}

// ============================================================================
//  TASK + ENTRY POINT  (replace the old uploadDataToOracleCloudVM / cloudUploadTask / startCloudUpload)
// ============================================================================
void cloudUploadTask(void *parameter)
{
    (void)parameter;
    cqSpillIfNeeded();                          // RAM -> flash if needed (no network required)
    cqDrain();                                   // flash + RAM -> cloud, oldest first
    inverter.cloud.cloudUploadPending = (cqPendingTotal() > 0);
    cloudUploadRunning = false;
    cloudUploadTaskHandle = NULL;
    vTaskDelete(NULL);
}

void startCloudUpload()
{
    cqTakeSnapshot();                            // ALWAYS record the sample - even if WiFi is down or the task is busy
    if (cloudUploadRunning) {
        return;                                  // previous task still sending; this sample waits in the queue
    }
    cloudUploadRunning = true;
    BaseType_t result = xTaskCreate(cloudUploadTask, "CloudUpload", 10240, NULL, 1, &cloudUploadTaskHandle);
    if (result != pdPASS) {
        cloudUploadRunning = false;
        cloudUploadTaskHandle = NULL;
        Serial.println("[CLOUD] Failed to create upload task.");
    }
}
