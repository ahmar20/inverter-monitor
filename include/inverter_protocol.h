#pragma once
// ============================================================================
// inverter_protocol.h — everything about talking to the PI18 inverter over
// UART: command frames + CRC framing, the non-blocking round-robin poll
// scheduler, response parsing into InverterState, per-command JSON writers
// (consumed by sendJSON() in web_server.h), and the authenticated
// POST /api/command handler for ad-hoc write commands.
// ============================================================================

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include "config.h"
#include "types.h"
#include "utils.h"
#include "debug_log.h"
#include "led_control.h"

// ===== Inverter Commands (PI18 frames, pre-computed CRC) =====
const byte GSX_CMD[10] {0x5E, 0x50, 0x30, 0x30, 0x35, 0x47, 0x53, 0x58, 0x14, 0x0D};
const byte MOD_CMD[11] {0x5E, 0x50, 0x30, 0x30, 0x36, 0x4D, 0x4F, 0x44, 0xDD, 0xBE, 0x0D};
const byte FWV_CMD[11] {0x5E, 0x50, 0x30, 0x30, 0x36, 0x46, 0x57, 0x53, 0xC5, 0x43, 0x0D};
const byte FLAG_CMD[12] {0x5E, 0x50, 0x30, 0x30, 0x37, 0x46, 0x4C, 0x41, 0x47, 0x8E, 0x18, 0x0D};
const byte PIRI_CMD[12] {0x5E, 0x50, 0x30, 0x30, 0x37, 0x50, 0x49, 0x52, 0x49, 0xEE, 0x38, 0x0D};
const byte DID_CMD[10] {0x5E, 0x50, 0x30, 0x30, 0x35, 0x49, 0x44, 0x19, 0xCD, 0x0D};
const byte PID_CMD[10] {0x5E, 0x50, 0x30, 0x30, 0x35, 0x50, 0x49, 0x71, 0x8B, 0x0D};
const byte ACCT_CMD[12] {0x5E,0x50,0x30,0x30,0x37,0x41,0x43,0x43,0x54,0xB7,0x34,0x0D};
const byte ACLT_CMD[12] {0x5E,0x50,0x30,0x30,0x37,0x41,0x43,0x4C,0x54,0xA7,0x0B,0x0D};

// ── Web-triggered command state ───────────────────────────────
volatile bool   webCmdPending    = false;
volatile bool   webCmdSent       = false;  // true only after sendCommand() fired for web cmd
volatile bool   webCmdDone       = false;
volatile bool   webCmdSuccess    = false;
String          webCmdResponse   = "";
String          webCmdString     = "";
uint8_t         webCmdBuf[64]    = {0};
size_t          webCmdLen        = 0;
unsigned long   webCmdSentAt     = 0;
bool            webCmdIsScheduled = false; // true = webCmd came from scheduler, not HTTP

enum InverterCommand {
  CMD_NONE,
  CMD_GSX,
  CMD_MOD,
  CMD_FLAG,
  CMD_PIRI,
  CMD_ACCT,
  CMD_ACLT,
  CMD_FWV,
  CMD_DID,
  CMD_PID
};
InverterCommand activeCommandType = CMD_NONE;

// ─── Round-robin command scheduler ───────────────────────────────
// One entry per command that gets polled every sweep. FWV/DID/PID are
// intentionally left out for now (per instructions) - they're static
// identifiers that don't need repeated polling or JSON output yet.
// jsonWriter populates this command's slice of the /api/data JSON, keyed by
// "name", so adding a new polled command only means adding one row here.
typedef void (*InverterJsonWriter)(JsonObject o);
void pollInverterCommands();
void registerCommandFailure();
void registerCommandSuccess();

struct InverterCommandDef {
  InverterCommand type;
  const char* name;              // JSON key, e.g. "GSX"
  InverterJsonWriter jsonWriter;
  uint8_t consecutiveFailures;   // runtime state
  unsigned long backoffUntil;    // millis() timestamp; 0 = not backed off
};

void jsonWriteGSX(JsonObject o);
void jsonWriteMOD(JsonObject o);
void jsonWriteFLAG(JsonObject o);
void jsonWritePIRI(JsonObject o);
void jsonWriteACCT(JsonObject o);
void jsonWriteACLT(JsonObject o);

InverterCommandDef inverterCommands[] = {
  { CMD_GSX,  "GSX",  jsonWriteGSX,  0, 0 },
  { CMD_PIRI, "PIRI", jsonWritePIRI, 0, 0 },
  { CMD_MOD,  "MOD",  jsonWriteMOD,  0, 0 },
  { CMD_FLAG, "FLAG", jsonWriteFLAG, 0, 0 },
  { CMD_ACCT, "ACCT", jsonWriteACCT, 0, 0 },
  { CMD_ACLT, "ACLT", jsonWriteACLT, 0, 0 },
};

const uint8_t NUM_INVERTER_COMMANDS = sizeof(inverterCommands) / sizeof(inverterCommands[0]);
enum PollSchedulerState { SCHED_ACTIVE, SCHED_WAITING };
PollSchedulerState pollSchedulerState = SCHED_ACTIVE;
uint8_t pollCmdIndex = 0;              // which entry in inverterCommands[] runs next
unsigned long pollCycleStart = 0;      // millis() when the current sweep began
unsigned long pollNextCycleAt = 0;     // millis() target to start the next sweep
int8_t activeCommandDefIndex = -1;     // index into inverterCommands[] currently in flight, -1 = none

// ===== Prototypes =====
bool sendCommand(const byte cmd[], size_t cmdLength);
GSXData parseGSXCommand(String response);
MODData  parseMODCommand(String response);
FLAGData parseFLAGCommand(String response);
PIRIData parsePIRICommand(String response);
ACCTData parseACCTCommand(String response);
ACLTData parseACLTCommand(String response);
FWVData parseFWVCommand(String response);
DIDData parseDIDCommand(String response);
PIDData parsePIDCommand(String response);
bool startInverterCommand(InverterCommand cmd);
void processInverterResponse();
void handleInverterCommand(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total);
void updateEnergyCounters();
void checkDailyEnergyReset();
extern bool haveInverterData; // defined in web_server.h

// Extern so main.cpp's printAll() (debug_report.h) can log it after each update
extern void printAll();

// ─── Low-level framing helpers ────────────────────────────────────

// Extract Nth comma-separated field from a string
String splitField(String& data, int index, char delimiter = ',') {
  int count = 0, start = 0;
  for (int i = 0; i <= (int)data.length(); i++) {
    if (i == (int)data.length() || data[i] == delimiter) {
      if (count == index) return data.substring(start, i);
      count++;
      start = i + 1;
    }
  }
  return "";
}

// Strip response header ^DXXX (first 5 chars) and last 2 CRC bytes
String stripResponse(String response) {
  if (response.length() < 6) return "";
  return response.substring(5, response.length() - 2);
}

bool errorInResponse(String response) {
  if (response.length() == 0) {
        setLEDState(LED_ERROR);
        Serial.println("WARNING: No response from inverter. Check wiring.");
        return true;
    }
    if (response.startsWith("^0")) {
        setLEDState(LED_ERROR);
        Serial.println("WARNING: Inverter rejected command (^0). Check CRC.");
        Serial.println("Raw: " + response);
        return true;
    }
  return false;
}

// CRC calculation for inverter communication
uint16_t crc_xmodem(const uint8_t* data, size_t len) {
  uint16_t crc = 0x0000;
  for (size_t i = 0; i < len; i++) {
    crc ^= (uint16_t)data[i] << 8;
    for (int j = 0; j < 8; j++) {
      if (crc & 0x8000) crc = (crc << 1) ^ 0x1021;
      else              crc = crc << 1;
      crc &= 0xFFFF;
    }
  }
  return crc;
}

static uint8_t adjust_crc_byte(uint8_t b) {
  // These bytes are reserved in the protocol and must be incremented
  if (b == 0x28 || b == 0x0D || b == 0x0A) b++;
  return b;
}

size_t buildWriteCommand(const String& cmdStr, uint8_t* outBuf) {
    size_t len = cmdStr.length();
    for (size_t i = 0; i < len; i++) outBuf[i] = (uint8_t)cmdStr[i];
    uint16_t crc = crc_xmodem(outBuf, len);
    uint8_t crcH = adjust_crc_byte((crc >> 8) & 0xFF);
    uint8_t crcL = adjust_crc_byte(crc & 0xFF);
    size_t pos = len;
    if (crcH != 0x00) outBuf[pos++] = crcH;
    if (crcL != 0x00) outBuf[pos++] = crcL;
    outBuf[pos++] = 0x0D;
    return pos;
}

// ─── Auto-generated JSON, one function per command ───────────────
// Each function fills in only its own nested object; sendJSON() below just
// loops over inverterCommands[] and calls each one. Adding a new polled
// command means: write its jsonWriteXXX() function, add one row to
// inverterCommands[] in main.h - sendJSON() itself never needs to change.
// FWV/DID/PID are intentionally left out for now, per instructions.

void jsonWriteGSX(JsonObject o)
{
  GSXData &d = inverter.gsx;
  o["valid"] = d.valid;
  if (!d.valid) return;
  o["gridVoltage"] = d.gridVoltage;
  o["gridFrequency"] = d.gridFrequency;
  o["gridAvailable"] = d.gridAvailable;
  o["estimatedGridPower"] = d.estimatedGridPower;
  o["dailyEstimatedGridKWh"] = d.dailyEstimatedGridKWh;
  o["outputVoltage"] = d.outputVoltage;
  o["outputFrequency"] = d.outputFrequency;
  o["outputVA"] = d.outputVA;
  o["outputWatts"] = d.outputWatts;
  o["dailyOutputEnergyKWh"] = d.dailyOutputEnergyKWh;
  o["loadPercent"] = d.loadPercent;
  o["batteryVoltage"] = d.batteryVoltage;
  o["batteryDischargeAmps"] = d.batteryDischargeAmps;
  o["batteryChargeAmps"] = d.batteryChargeAmps;
  o["batteryPercentage"] = d.batteryPercentage;
  o["batteryChargePower"] = d.batteryChargePower;
  o["batteryDischargePower"] = d.batteryDischargePower;
  o["dailyBatteryChargeKWh"] = d.dailyBatteryChargeKWh;
  o["dailyBatteryDischargeKWh"] = d.dailyBatteryDischargeKWh;
  o["inverterTemp"] = d.inverterTemp;
  o["mpptCharger1Temp"] = d.mpptCharger1Temp;
  o["mpptCharger2Temp"] = d.mpptCharger2Temp;
  o["pv1InputWatts"] = d.pv1InputWatts;
  o["pv1InputVoltage"] = d.pv1InputVoltage;
  o["dailyPVInputKWh"] = d.dailyPVInputKWh;
  o["configState"] = d.configState;
  o["mppt1ChargerStatus"] = d.mppt1ChargerStatus;
  o["mppt2ChargerStatus"] = d.mppt2ChargerStatus;
  o["loadConnected"] = d.loadConnected;
  o["batteryPowerDirection"] = d.batteryPowerDirection;
  o["dcAcDirection"] = d.dcAcDirection;
  o["gridPowerDirection"] = d.gridPowerDirection;
  o["localParallelID"] = d.localParallelID;
}

void jsonWriteMOD(JsonObject o)
{
  MODData &d = inverter.mod;
  o["valid"] = d.valid;
  if (!d.valid) return;
  o["outputMode"] = d.outputMode;
}

void jsonWriteFLAG(JsonObject o)
{
  FLAGData &d = inverter.flag;
  o["valid"] = d.valid;
  if (!d.valid) return;
  o["buzzerEnabled"] = d.buzzerEnabled;
  o["overloadBypass"] = d.overloadBypass;
  o["lcdTimeout"] = d.lcdTimeout;
  o["overloadRestart"] = d.overloadRestart;
  o["overTempRestart"] = d.overTempRestart;
  o["backlightOn"] = d.backlightOn;
  o["alarmPrimaryInput"] = d.alarmPrimaryInput;
  o["faultCodeRecord"] = d.faultCodeRecord;
}

void jsonWritePIRI(JsonObject o)
{
  PIRIData &d = inverter.piri;
  o["valid"] = d.valid;
  if (!d.valid) return;
  o["gridVoltageRating"] = d.gridVoltageRating;
  o["gridCurrentRating"] = d.gridCurrentRating;
  o["outputVoltageRating"] = d.outputVoltageRating;
  o["outputFreqRating"] = d.outputFreqRating;
  o["outputCurrentRating"] = d.outputCurrentRating;
  o["outputVARating"] = d.outputVARating;
  o["outputWattRating"] = d.outputWattRating;
  o["battVoltageRating"] = d.battVoltageRating;
  o["battStopDischargeVoltageOnGrid"] = d.battStopDischargeVoltageOnGrid;
  o["battStopChargeVoltageOnGrid"] = d.battStopChargeVoltageOnGrid;
  o["battCutoffVoltage"] = d.battCutoffVoltage;
  o["battBulkChargingVoltage"] = d.battBulkChargingVoltage;
  o["battFloatVoltage"] = d.battFloatVoltage;
  o["batteryConfigType"] = d.batteryConfigType;
  o["maxACChargeCurrent"] = d.maxACChargeCurrent;
  o["maxTotalChargeCurrent"] = d.maxTotalChargeCurrent;
  o["inputVoltageRange"] = d.inputVoltageRange;
  o["outputSourcePriority"] = d.outputSourcePriority;
  o["chargerSourcePriority"] = d.chargerSourcePriority;
  o["parallelInverterType"] = d.parallelInverterType;
  o["topology"] = d.topology;
  o["outputModeSetting"] = d.outputModeSetting;
  o["solarPowerPriority"] = d.solarPowerPriority;
  o["mpptStringsCount"] = d.mpptStringsCount;
  o["cpuSubcode"] = d.cpuSubcode;
}

void jsonWriteACCT(JsonObject o)
{
  ACCTData &d = inverter.acct;
  o["valid"] = d.valid;
  if (!d.valid) return;
  o["startTime"] = d.startTime;
  o["endTime"] = d.endTime;
}

void jsonWriteACLT(JsonObject o)
{
  ACLTData &d = inverter.aclt;
  o["valid"] = d.valid;
  if (!d.valid) return;
  o["startTime"] = d.startTime;
  o["endTime"] = d.endTime;
}

// Send command to inverter and read response
bool sendCommand(const byte cmd[], size_t cmdLength)
{
  // Don't start another command while one is active
  if (commandInProcess)
  {
    return false;
  }

  while (INV_Serial.available())
  {
    INV_Serial.read();
  }
  inverterResponse = "";
  activeCommand = cmd;
  activeCommandLength = cmdLength;

  commandInProcess = true;
  commandStartTime = millis();
  setLEDState(LED_QUERY);
  INV_Serial.write(cmd, cmdLength);
  return true;
}

void processInverterResponse()
{
  if (!commandInProcess)
  {
    return;
  }

  // Read serial data
  while (INV_Serial.available())
  {
    char c = INV_Serial.read();
    if ((uint8_t)c == 0x0D)
    {
      // Complete response
      commandInProcess = false;
      setLEDState(LED_RESPONSE);
      String r = inverterResponse;
      inverterResponse = "";

      // If this was a web-triggered or scheduled command, capture the result
      if (webCmdPending && webCmdSent) {
          webCmdSuccess  = !errorInResponse(r);
          webCmdResponse = r;
          webCmdPending  = false;
          webCmdSent     = false;
          webCmdDone     = true;
          // Scheduled command — just log and let the queue drain; no HTTP caller waiting
          if (webCmdIsScheduled) {
              dbgLog("[SCHED] Response success=%d cmd=%s", webCmdSuccess, webCmdString.c_str());
              webCmdIsScheduled = false;
              return;
          }
          // HTTP-triggered command — log full hex + text for /serial debugging
          char hexLine[DBG_LINE_LEN];
          int pos = snprintf(hexLine, sizeof(hexLine), "[WEB] Response success=%d raw:", webCmdSuccess);
          for (int i = 0; i < (int)r.length() && pos < (int)sizeof(hexLine) - 4; i++)
              pos += snprintf(hexLine + pos, sizeof(hexLine) - pos, " %02X", (uint8_t)r[i]);
          dbgLog("%s", hexLine);
          dbgLog("[WEB] Response text: %s", r.c_str());
          return;
      }

      // Check response
            if (errorInResponse(r))
      {
        setLEDState(LED_ERROR);
        if (r.length() == 0)
        {
          Serial.println("WARNING: No response from inverter.");
        }
        else if (r.startsWith("^0"))
        {
          Serial.println("WARNING: Inverter rejected command (^0). Check CRC.\nRaw Response: " + r);
        }
        // "valid" tracks GSX specifically - it's the command carrying live
        // telemetry. Letting every command's pass/fail touch it made it
        // flicker based on whichever command happened to run last.
        if (activeCommandType == CMD_GSX) haveInverterData = false;
        registerCommandFailure();
      }
      else
      {
        registerCommandSuccess();
        // Parse according to command
        switch (activeCommandType)
        {
          case CMD_GSX:
          {
            GSXData newGsx = parseGSXCommand(r);
            newGsx.dailyBatteryChargeKWh    = inverter.gsx.dailyBatteryChargeKWh;
            newGsx.dailyBatteryDischargeKWh = inverter.gsx.dailyBatteryDischargeKWh;
            newGsx.dailyEstimatedGridKWh    = inverter.gsx.dailyEstimatedGridKWh;
            newGsx.dailyPVInputKWh          = inverter.gsx.dailyPVInputKWh;
            newGsx.dailyOutputEnergyKWh     = inverter.gsx.dailyOutputEnergyKWh;
            newGsx.lastCmdUpdate            = inverter.gsx.lastCmdUpdate;
            inverter.gsx = newGsx;
            lastSuccessfulInverterResponse = millis();
            haveInverterData = true;
            break;
          }
          case CMD_MOD:
            inverter.mod = parseMODCommand(r);
            inverter.mod.lastCmdUpdate = millis();
            break;
          case CMD_FLAG:
            inverter.flag = parseFLAGCommand(r);
            inverter.flag.lastCmdUpdate = millis();
            break;
          case CMD_PIRI:
            inverter.piri = parsePIRICommand(r);
            inverter.piri.lastCmdUpdate = millis();
            break;
          case CMD_ACCT:
            inverter.acct = parseACCTCommand(r);
            inverter.acct.lastCmdUpdate = millis();
            break;
          case CMD_ACLT:
            inverter.aclt = parseACLTCommand(r);
            inverter.aclt.lastCmdUpdate = millis();
            break;
          case CMD_FWV:
            inverter.fwv = parseFWVCommand(r);
            inverter.fwv.lastCmdUpdate = millis();
            break;
          case CMD_PID:
            inverter.pid = parsePIDCommand(r);
            inverter.pid.lastCmdUpdate = millis();
            break;
          case CMD_DID:
            inverter.did = parseDIDCommand(r);
            inverter.did.lastCmdUpdate = millis();
            break;

          default:
            break;
        }
        inverter.lastUpdated = getTimeString();
        printAll();
      }
      activeCommandType = CMD_NONE;
      activeCommandDefIndex = -1;
      return;
    }
    // Add character to response
    inverterResponse += c;
  }

  // Timeout
  if (millis() - commandStartTime >= COMMAND_TIMEOUT)
  {
    commandInProcess = false;
    registerCommandFailure();
    if (activeCommandType == CMD_GSX) haveInverterData = false;
    activeCommandType = CMD_NONE;
    activeCommandDefIndex = -1;
    inverterResponse = "";
    setLEDState(LED_ERROR);
    Serial.println(
        "WARNING: Inverter response timeout.");
  }
}

// Mark the currently in-flight command (by its slot in inverterCommands[]) as
// having failed. After POLL_MAX_CONSECUTIVE_FAILURES in a row, it's put on a
// temporary backoff so a persistently unresponsive command (e.g. ACLT on
// firmware that doesn't support it) can't keep stealing a slot from every
// other command's 1-second cadence.
void registerCommandFailure()
{
  if (activeCommandDefIndex < 0 || activeCommandDefIndex >= NUM_INVERTER_COMMANDS)
    return;
  InverterCommandDef &def = inverterCommands[activeCommandDefIndex];
  def.consecutiveFailures++;
  if (def.consecutiveFailures >= POLL_MAX_CONSECUTIVE_FAILURES)
  {
    def.backoffUntil = millis() + POLL_BACKOFF_DURATION_MS;
    Serial.printf("WARNING: %s failed %u times in a row - backing off for %lu ms.\n",
                  def.name, def.consecutiveFailures, POLL_BACKOFF_DURATION_MS);
  }
}

void registerCommandSuccess()
{
  if (activeCommandDefIndex < 0 || activeCommandDefIndex >= NUM_INVERTER_COMMANDS)
    return;
  InverterCommandDef &def = inverterCommands[activeCommandDefIndex];
  def.consecutiveFailures = 0;
  def.backoffUntil = 0;
}

bool startInverterCommand(InverterCommand cmd)
{
  if (commandInProcess)
  {
    return false;
  }
  activeCommandType = cmd;
  bool started = false;
  switch (cmd)
  {
  case CMD_GSX:
    started = sendCommand(GSX_CMD, sizeof(GSX_CMD));
    break;
  case CMD_MOD:
    started = sendCommand(MOD_CMD, sizeof(MOD_CMD));
    break;
  case CMD_FLAG:
    started = sendCommand(FLAG_CMD, sizeof(FLAG_CMD));
    break;
  case CMD_PIRI:
    started = sendCommand(PIRI_CMD, sizeof(PIRI_CMD));
    break;
  case CMD_ACCT:
    started = sendCommand(ACCT_CMD, sizeof(ACCT_CMD));
    break;
  case CMD_ACLT:
    started = sendCommand(ACLT_CMD, sizeof(ACLT_CMD));
    break;
  case CMD_FWV:
    started = sendCommand(FWV_CMD, sizeof(FWV_CMD));
    break;
  case CMD_PID:
    started = sendCommand(PID_CMD, sizeof(PID_CMD));
    break;
  case CMD_DID:
    started = sendCommand(DID_CMD, sizeof(DID_CMD));
    break;

  default:
    activeCommandType = CMD_NONE;
    return false;
  }
  if (started)
  {
    // lastCommandStarted = commandTypeName(cmd);
    // lastCommandStartedAt = millis();
  }
  return started;
}

// Non-blocking round-robin scheduler over inverterCommands[]. Call this once
// per loop() iteration. It never blocks: it either does nothing (a command is
// already in flight, or we're waiting out the rest of the current second),
// or starts exactly one command and returns immediately. The actual response
// is read incrementally by processInverterResponse(), which is what lets
// this be called every loop() without stalling anything else (WiFi, OTA,
// web server, LED, etc).
void pollInverterCommands()
{
  if (commandInProcess || webCmdPending)
    return; // a reply is still being read; processInverterResponse() owns it

  unsigned long now = millis();

  if (pollSchedulerState == SCHED_WAITING)
  {
    if ((long)(now - pollNextCycleAt) < 0)
      return; // not time for the next sweep yet
    pollSchedulerState = SCHED_ACTIVE;
    pollCmdIndex = 0;
    pollCycleStart = now;
  }

  // Skip any command currently on backoff (e.g. ACLT timing out repeatedly)
  while (pollCmdIndex < NUM_INVERTER_COMMANDS &&
         inverterCommands[pollCmdIndex].backoffUntil != 0 &&
         (long)(now - inverterCommands[pollCmdIndex].backoffUntil) < 0)
  {
    pollCmdIndex++;
  }

  if (pollCmdIndex >= NUM_INVERTER_COMMANDS)
  {
    // Finished a full sweep (or everything left is backed off). Pace the
    // next sweep so we target one sweep every INTERVAL_INVERTER_POLLING -
    // but never wait if we're already running behind.
    unsigned long elapsed = now - pollCycleStart;
    Serial.printf("[POLL] cycle of %u commands took %lu ms (target %lu ms)\n",
                  NUM_INVERTER_COMMANDS, elapsed, (unsigned long)INTERVAL_INVERTER_POLLING);
    pollNextCycleAt = (elapsed < INTERVAL_INVERTER_POLLING)
                          ? (pollCycleStart + INTERVAL_INVERTER_POLLING)
                          : now;
    pollSchedulerState = SCHED_WAITING;
    return;
  }

  activeCommandDefIndex = pollCmdIndex;
  bool started = startInverterCommand(inverterCommands[pollCmdIndex].type);
  if (!started)
  {
    // Shouldn't normally happen (we already checked commandInProcess above),
    // but if it does, don't advance - just try this same slot again next loop().
    activeCommandDefIndex = -1;
    return;
  }
  pollCmdIndex++; // next call to pollInverterCommands() moves on to the next command
}

// ─── Parse GSX Response ──────────────────────────────────────────
GSXData parseGSXCommand(String response)
{
  GSXData data;
  data.valid = false;
  data.rawResponse = response;
  // Expected format: ^D106<f0>,<f1>,...<CRC_H><CRC_L>
  if (!response.startsWith("^D"))
    return data;
  if (response.length() < 8)
    return data;

  String inner = stripResponse(response);
  data.gridVoltage = splitField(inner, 0).toInt() / 10.0;
  data.gridFrequency = splitField(inner, 1).toInt() / 10.0;
  data.outputVoltage = splitField(inner, 2).toInt() / 10.0;
  data.outputFrequency = splitField(inner, 3).toInt() / 10.0;
  data.outputVA = splitField(inner, 4).toInt();
  data.outputWatts = splitField(inner, 5).toInt();
  data.loadPercent = splitField(inner, 6).toInt();
  data.batteryVoltage = splitField(inner, 7).toInt() / 10.0;
  data.batteryVoltSCC1 = splitField(inner, 8).toInt();
  data.batteryVoltSCC2 = splitField(inner, 9).toInt();
  data.batteryDischargeAmps = splitField(inner, 10).toInt();
  data.batteryChargeAmps = splitField(inner, 11).toInt(); // Need to verify 8, 9, 10, 11 are correct
  data.batteryPercentage = splitField(inner, 12).toInt();
  data.inverterTemp = splitField(inner, 13).toInt();
  data.mpptCharger1Temp = splitField(inner, 14).toInt();
  data.mpptCharger2Temp = splitField(inner, 15).toInt();
  data.pv1InputWatts = splitField(inner, 16).toInt();
  data.pv2InputWatts = splitField(inner, 17).toInt();
  data.pv1InputVoltage = splitField(inner, 18).toInt() / 10.0;
  data.pv2InputVoltage = splitField(inner, 19).toInt() / 10.0;
  // Flags
  data.configState = splitField(inner, 20).toInt();
  data.mppt1ChargerStatus = splitField(inner, 21).toInt();
  data.mppt2ChargerStatus = splitField(inner, 22).toInt();
  data.loadConnected = splitField(inner, 23).toInt();
  data.batteryPowerDirection = splitField(inner, 24).toInt();
  data.dcAcDirection = splitField(inner, 25).toInt();
  data.gridPowerDirection = splitField(inner, 26).toInt();
  data.localParallelID = splitField(inner, 27).toInt();

  // Battery charging power
  if (inverter.piri.valid && data.batteryPowerDirection == 1)
    data.batteryChargePower = inverter.piri.battBulkChargingVoltage * data.batteryChargeAmps;

  // Battery discharging power
  if (data.batteryPowerDirection == 2)
      data.batteryDischargePower = data.batteryVoltage * data.batteryDischargeAmps;
  
  // Grid available
  if (data.gridVoltage >= 50.0)
  {
    data.gridAvailable = true;
    bool bypassMode = inverter.mod.valid && inverter.mod.outputMode == "Grid Bypass";

    if (bypassMode) {
      data.estimatedGridPower = data.outputWatts;
    } else {
      if (data.gridPowerDirection == 1) {
        data.estimatedGridPower = data.outputWatts + data.batteryChargePower - data.batteryDischargePower - data.pv1InputWatts;
      } else {
        data.estimatedGridPower = 0.0;
      }
    }

    if (data.estimatedGridPower < 0.0) data.estimatedGridPower = 0.0;
  }
  else
  {
    data.gridAvailable = false;
    data.estimatedGridPower = 0.0;
  }
  data.valid = true;
  return data;
}

// UPDATE ENERGY COUNTERS
void updateEnergyCounters()
{
  unsigned long now = millis();
  // First run
  if (inverter.gsx.lastCmdUpdate == 0)
  {
    inverter.gsx.lastCmdUpdate = now;
    return;
  }
  unsigned long elapsed = now - inverter.gsx.lastCmdUpdate;
  // Ignore abnormal gaps
  if (elapsed > 10000)
  {
    inverter.gsx.lastCmdUpdate = now;
    return;
  }
  inverter.gsx.lastCmdUpdate = now;

  if (millis() - lastSuccessfulInverterResponse >= INV_DOWN_THRESHOLD_MS)
    return;

  // milliseconds → hours
  float hours = elapsed / 3600000.0;

  // BATTERY CHARGING ENERGY
  if (inverter.gsx.batteryChargePower > 0.0)
  {
    inverter.gsx.dailyBatteryChargeKWh += (inverter.gsx.batteryChargePower * hours) / 1000.0;
  }
  // BATTERY DISCHARGING ENERGY
  if (inverter.gsx.batteryDischargePower > 0.0)
  {
    inverter.gsx.dailyBatteryDischargeKWh += (inverter.gsx.batteryDischargePower * hours) / 1000.0;
  }
  // GRID IMPORT ENERGY
  if (inverter.gsx.estimatedGridPower > 0.0)
  {
    inverter.gsx.dailyEstimatedGridKWh += (inverter.gsx.estimatedGridPower * hours) / 1000.0;
  }
  // PV GENERATED
  if (inverter.gsx.pv1InputWatts > 0.0)
  {
    inverter.gsx.dailyPVInputKWh += (inverter.gsx.pv1InputWatts * hours) / 1000.0;
  }
  // OUTPUT ENERGY
  if (inverter.gsx.outputWatts > 0.0)
  {
    inverter.gsx.dailyOutputEnergyKWh += (inverter.gsx.outputWatts * hours) / 1000.0;
  }

  // ── Coulomb counting SOC ──────────────────────────────────
  if (ccSoc < 0.0f) {
      if (inverter.gsx.valid && inverter.gsx.batteryVoltage > 40.0f) {
          ccSoc = (float)inverter.gsx.batteryPercentage;
          ccInitialised = true;
          Serial.printf("[CC] Initialised CC SOC from inverter: %.1f%%\n", ccSoc);
      }
      return;
  } else {
      // Net battery power: positive = charging, negative = discharging
      float netW = inverter.gsx.batteryChargePower - inverter.gsx.batteryDischargePower;
      float deltaWh = netW * hours;
      float deltaSoc = (deltaWh / CC_CAPACITY_WH) * 100.0f;
      ccSoc += deltaSoc;
      ccSoc = constrain(ccSoc, 0.0f, 100.0f);
  }

  // Reset to 100% when battery reaches float stage:
  // voltage >= bulk charge voltage AND charge current very low (< 2A)
  if (inverter.gsx.batteryVoltage >= inverter.piri.battFloatVoltage &&
      inverter.gsx.batteryChargeAmps > 0 &&
      inverter.gsx.batteryChargeAmps < 3) {
      if (ccSoc < 99.0f) {
          Serial.printf("[CC] Battery full — resetting CC SOC from %.1f%% to 100%%\n", ccSoc);
      }
      ccSoc = 100.0f;
  }

  // Reset to 0% only after sustained cutoff voltage — not on momentary sag
  if (inverter.gsx.batteryVoltage <= inverter.piri.battCutoffVoltage + 0.3f &&
      inverter.gsx.batteryDischargePower > 200.0f) {
      ccCutoffCount++;
      if (ccCutoffCount >= ccCutoffThreshold) {  // 10 consecutive readings = ~10 seconds sustained
          if (ccSoc > 1.0f) {
              Serial.printf("[CC] Battery cutoff sustained — resetting CC SOC from %.1f%% to 0%%\n", ccSoc);
          }
          ccSoc = 0.0f;
          ccCutoffCount = 0;
      }
  }
  else {
      ccCutoffCount = 0;  // reset counter if voltage recovers
  }
}

void checkDailyEnergyReset()
{
    static unsigned long lastCheck = 0;
    if (millis() - lastCheck < 60000UL) return;   // check once per minute
    lastCheck = millis();

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 0))               // 0 = don't block the loop
        return;

    if (lastResetYday == -1) {                     // first valid read after boot
        lastResetYday = timeinfo.tm_yday;
        return;
    }

    if (timeinfo.tm_yday != lastResetYday) {
        Serial.printf("[ENERGY] New day (%04d-%02d-%02d) - resetting daily counters\n",
            timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday);
        inverter.gsx.dailyBatteryChargeKWh    = 0;   // your daily counters
        inverter.gsx.dailyBatteryDischargeKWh = 0;
        inverter.gsx.dailyEstimatedGridKWh    = 0;
        inverter.gsx.dailyPVInputKWh          = 0;
        inverter.gsx.dailyOutputEnergyKWh     = 0;

        lastResetYday = timeinfo.tm_yday;
    }
}

// ─── MOD Parser — Operating Mode ────────────────────────────────
MODData parseMODCommand(String response)
{
  MODData d;
  d.valid = false;
  d.rawResponse = response;
  if (!response.startsWith("^D") || response.length() < 6)
    return d;
  String inner = stripResponse(response);
  // Output mode
  String f0 = splitField(inner, 0);
  if (f0 == "00")
    d.outputMode = "Power On";
  else if (f0 == "01")
    d.outputMode = "Standby";
  else if (f0 == "02")
    d.outputMode = "Grid Bypass";
  else if (f0 == "03")
    d.outputMode = "Battery / Solar";
  else if (f0 == "04")
    d.outputMode = "Fault";
  else if (f0 == "05")
    d.outputMode = "Hybrid / Grid Tie";
  else
    d.outputMode = "Unknown (" + f0 + ")";

  d.valid = true;
  return d;
}

// ─── FLAG Parser — Device Flags ──────────────────────────────────
FLAGData parseFLAGCommand(String response)
{
  FLAGData d;
  d.valid = false;
  d.rawResponse = response;
  if (!response.startsWith("^D") || response.length() < 6)
    return d;
  String inner = stripResponse(response);
  // Each field is 0 or 1
  d.buzzerEnabled = splitField(inner, 0) == "1";
  d.overloadBypass = splitField(inner, 1) == "1";
  d.lcdTimeout = splitField(inner, 2) == "1";
  d.overloadRestart = splitField(inner, 3) == "1";
  d.overTempRestart = splitField(inner, 4) == "1";
  d.backlightOn = splitField(inner, 5) == "1";
  d.alarmPrimaryInput = splitField(inner, 6) == "1";
  d.faultCodeRecord = splitField(inner, 7) == "1";
  d.valid = true;
  return d;
}

// ─── PIRI Parser — Rated Parameters ──────────────────────────────
PIRIData parsePIRICommand(String response)
{
  PIRIData d;
  d.valid = false;
  d.rawResponse = response;
  if (!response.startsWith("^D") || response.length() < 8)
    return d;
  String inner = stripResponse(response);
  d.gridVoltageRating = splitField(inner, 0).toInt() / 10.0;
  d.gridCurrentRating = splitField(inner, 1).toFloat() / 10.0;
  d.outputVoltageRating = splitField(inner, 2).toInt() / 10.0;
  d.outputFreqRating = splitField(inner, 3).toInt() / 10.0;
  d.outputCurrentRating = splitField(inner, 4).toFloat() / 10.0;
  d.outputVARating = splitField(inner, 5).toInt();
  d.outputWattRating = splitField(inner, 6).toInt();
  d.battVoltageRating = splitField(inner, 7).toInt() / 10.0;
  d.battStopDischargeVoltageOnGrid = splitField(inner, 8).toInt() / 10.0;
  d.battStopChargeVoltageOnGrid = splitField(inner, 9).toInt() / 10.0;
  d.battCutoffVoltage = splitField(inner, 10).toInt() / 10.0;
  d.battBulkChargingVoltage = splitField(inner, 11).toInt() / 10.0;
  d.battFloatVoltage = splitField(inner, 12).toInt() / 10.0;

  String bt = splitField(inner, 13);
  if (bt == "0")
    d.batteryConfigType = "AGM";
  else if (bt == "1")
    d.batteryConfigType = "Flooded";
  else if (bt == "2")
    d.batteryConfigType = "User-defined";
  else
    d.batteryConfigType = bt;

  d.maxACChargeCurrent = splitField(inner, 14).toInt();
  d.maxTotalChargeCurrent = splitField(inner, 15).toInt();

  String ivr = splitField(inner, 16);
  if (ivr == "0")
    d.inputVoltageRange = "APL (90-280V)";
  else if (ivr == "1")
    d.inputVoltageRange = "UPS (170-280V)";
  else
    d.inputVoltageRange = ivr;

  String osp = splitField(inner, 17);
  if (osp == "0")
    d.outputSourcePriority = "S->U->B";
  else if (osp == "1")
    d.outputSourcePriority = "S->B->U";
  else
    d.outputSourcePriority = osp;

  String csp = splitField(inner, 18);
  if (csp == "0")
    d.chargerSourcePriority = "Solar-first";
  else if (csp == "1")
    d.chargerSourcePriority = "Solar + Grid";
  else if (csp == "2")
    d.chargerSourcePriority = "Only Solar";
  else
    d.chargerSourcePriority = csp;

  String machType = splitField(inner, 20);
  d.machineType = machType == "0" ? "Off-Grid" : machType == "1" ? "Hybrid" : "Unknown " + machType;

  String top = splitField(inner, 21);
  if (top == "0")
    d.topology = "Tranformer-less";
  else if (top == "1")
    d.topology = "Tranformer-based";

  d.outputModeSetting = splitField(inner, 22);

  String om = splitField(inner, 23);
  if (om == "0")
    d.solarPowerPriority = "B->L->U";
  else if (om == "1")
    d.solarPowerPriority = "L->B->U";
  else
    d.solarPowerPriority = om;

  d.mpptStringsCount = splitField(inner, 24).toInt();

  d.valid = true;
  return d;
}

// ─── ACCT Parser — AC Charge Time ────────────────────────────────
ACCTData parseACCTCommand(String response)
{
  ACCTData d;
  d.valid = false;
  d.rawResponse = response;
  if (!response.startsWith("^D") || response.length() < 6)
    return d;
  String inner = stripResponse(response);
  String raw0 = splitField(inner, 0);
  String raw1 = splitField(inner, 1);
  // Format raw time "1600" → "16:00"
  d.startTime = raw0.substring(0, 2) + ":" + raw0.substring(2);
  d.endTime = raw1.substring(0, 2) + ":" + raw1.substring(2);
  d.valid = true;
  return d;
}

// ─── ACLT Parser — AC Load Time ──────────────────────────────────
ACLTData parseACLTCommand(String response)
{
  ACLTData d;
  d.valid = false;
  d.rawResponse = response;
  if (!response.startsWith("^D") || response.length() < 6)
    return d;
  String inner = stripResponse(response);
  String raw0 = splitField(inner, 0);
  String raw1 = splitField(inner, 1);
  d.startTime = raw0.substring(0, 2) + ":" + raw0.substring(2);
  d.endTime = raw1.substring(0, 2) + ":" + raw1.substring(2);
  d.valid = true;
  return d;
}

// ─── FWV Parser — Firmware Version ──────────────────────────────────
FWVData parseFWVCommand(String response)
{
  FWVData d;
  d.valid = false;
  d.rawResponse = response;
  if (!response.startsWith("^D"))
    return d;
  String inner = stripResponse(response);
  d.valid = true;
  return d;
}

// ─── DID Parser — Device Identifier ──────────────────────────────────
DIDData parseDIDCommand(String response)
{
  DIDData d;
  d.valid = false;
  d.rawResponse = response;
  if (!response.startsWith("^D"))
    return d;
  String inner = stripResponse(response);
  d.valid = true;
  return d;
}

// ─── PID Parser — Protocol Identifier ──────────────────────────────────
PIDData parsePIDCommand(String response)
{
  PIDData d;
  d.valid = false;
  d.rawResponse = response;
  if (!response.startsWith("^D"))
    return d;
  String inner = stripResponse(response);
  d.valid = true;
  return d;
}

void handleInverterCommand(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
    // Accumulate body chunks; only process when last chunk arrives
    if (index == 0) {
        if (request->_tempObject) { free(request->_tempObject); }
        request->_tempObject = malloc(total + 1);
        if (!request->_tempObject) {
            request->send(500, "application/json", "{\"status\":\"error\",\"message\":\"Out of memory\"}");
            return;
        }
    }
    if (request->_tempObject) {
        memcpy((uint8_t*)request->_tempObject + index, data, len);
    }
    if (index + len < total) return;  // more chunks coming

    ((uint8_t*)request->_tempObject)[total] = 0;
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, (char*)request->_tempObject);
    free(request->_tempObject);
    request->_tempObject = nullptr;
    if (err) {
        request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
        return;
    }

    const char* cmd = doc["command"];
    if (!cmd || strlen(cmd) == 0) {
        request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing command field\"}");
        return;
    }

    // Safety: only allow known command prefixes
    String cmdStr = String(cmd);
    bool allowed = cmdStr.startsWith("^S") &&
                   (cmdStr.indexOf("POP")    >= 0 ||   // Output priority
                    cmdStr.indexOf("PCP")    >= 0 ||   // Charger priority
                    cmdStr.indexOf("PSP")    >= 0 ||   // Solar priority
                    cmdStr.indexOf("PGR")    >= 0 ||   // AC input voltage range
                    cmdStr.indexOf("PBT")    >= 0 ||   // Battery type
                    cmdStr.indexOf("POPM")   >= 0 ||   // Output mode
                    cmdStr.indexOf("MCHGV")  >= 0 ||   // Bulk+float voltage
                    cmdStr.indexOf("MUCHGC") >= 0 ||   // Max AC charge current
                    cmdStr.indexOf("MCHGC")  >= 0 ||   // Max total charge current
                    cmdStr.indexOf("PSDV")   >= 0 ||   // Cutoff voltage
                    cmdStr.indexOf("BUCD")   >= 0 ||   // Stop discharge/charge V
                    cmdStr.indexOf("LON")    >= 0 ||   // Loads on/off
                    cmdStr.indexOf("DAT")    >= 0 ||   // Date/time
                    cmdStr.indexOf("ACCT")   >= 0 ||   // AC charge timer
                    cmdStr.indexOf("ACLT")   >= 0 ||   // AC load timer
                    cmdStr.indexOf("V")      >= 0 ||   // Output voltage
                    cmdStr.indexOf("F5")     >= 0 ||   // Frequency 50Hz
                    cmdStr.indexOf("F6")     >= 0 ||   // Frequency 60Hz
                    (cmdStr.indexOf("PE")    >= 0 &&   // Flag enable
                    cmdStr.length() <= 12)        ||  
                    (cmdStr.indexOf("PD")    >= 0 &&   // Flag disable
                    cmdStr.length() <= 12));

    if (!allowed) {
        request->send(403, "application/json", "{\"status\":\"error\",\"message\":\"Command not permitted\"}");
        return;
    }

    // Only reject if a web command is already queued; commandInProcess clears quickly
    if (webCmdPending) {
        request->send(503, "application/json", "{\"status\":\"error\",\"message\":\"Command already queued, try again\"}");
        return;
    }

    // Build the framed command
    webCmdString = cmdStr;
    webCmdLen    = buildWriteCommand(cmdStr, webCmdBuf);
    webCmdPending = true;
    webCmdDone    = false;
    webCmdSuccess = false;
    webCmdSent    = false;
    webCmdResponse = "";
    webCmdSentAt  = millis();
    dbgLog("[WEB] Queued: %s  (%u frame bytes)", cmdStr.c_str(), webCmdLen);

    // Wait up to 3 seconds for response (polling, non-blocking friendly)
    unsigned long start = millis();
    while (!webCmdDone && (millis() - start) < 3000) {
        delay(10);
    }

    if (!webCmdDone) {
        webCmdPending = false;
        dbgLog("[WEB] TIMEOUT waiting for inverter response (cmd=%s)", cmdStr.c_str());
        request->send(504, "application/json", "{\"status\":\"error\",\"message\":\"Inverter timeout\"}");
        return;
    }

    String cleanResp = "";
    for (size_t i = 0; i < webCmdResponse.length(); i++) {
        char c = webCmdResponse[i];
        if (c >= 0x20 && c < 0x7F) cleanResp += c;
    }
    String respJson = "{\"status\":\"" + String(webCmdSuccess ? "ok" : "error") +
                      "\",\"message\":\"" + (webCmdSuccess ? "Command accepted" : "Inverter rejected command") +
                      "\",\"inverter_response\":\"" + cleanResp + "\"}";
    request->send(webCmdSuccess ? 200 : 400, "application/json", respJson);
}

