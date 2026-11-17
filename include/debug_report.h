#pragma once
// ============================================================================
// debug_report.h — human-readable dumps: printAll() to the Serial monitor
// after every successful inverter update, and generateSerialDebug() which
// renders the full system/queue/inverter snapshot served at /api/serial.
// ============================================================================

#include <Arduino.h>
#include <LittleFS.h>
#include "config.h"
#include "types.h"
#include "debug_log.h"
#include "cloud_queue.h"

void printAll()
{
  Serial.println("\n=========================================");
  Serial.println("   INFINISOLAR VII 5KW  -  LIVE DATA");
  Serial.println("   " + inverter.lastUpdated);
  Serial.println("=========================================\n\n");

  Serial.println("GSX Data Valid: " + String(inverter.gsx.valid ? "YES" : "NO"));
  if (inverter.gsx.valid)
  {
    GSXData &d = inverter.gsx;
    Serial.println("  [ RAW RESPONSE - GSX Command]\n");
    Serial.println("    " + d.rawResponse + "\n\n");
    Serial.println("  [ GRID ]\n");
    if (d.gridVoltage < 10.0)
      Serial.println("    Status      : OFFLINE\n");
    else
      Serial.println("    Status      : Online\n");
    Serial.printf("    Voltage     : %.1f V\n", d.gridVoltage);
    Serial.printf("    Frequency   : %.1f Hz\n", d.gridFrequency);

    Serial.println("  [ OUTPUT ]\n");
    Serial.printf("    Voltage     : %.1f V\n", d.outputVoltage);
    Serial.printf("    Frequency   : %.1f Hz\n", d.outputFrequency);
    Serial.printf("    Active Pwr  : %d W\n", d.outputWatts);
    Serial.printf("    Apparent Pwr: %d VA\n", d.outputVA);
    Serial.printf("    Load        : %d %%\n", d.loadPercent);

    Serial.println("  [ BATTERY ]\n");
    Serial.printf("    Voltage     : %.1f V\n", d.batteryVoltage);
    Serial.printf("    Discharge A : %.1f A\n", d.batteryDischargeAmps);
    Serial.printf("    Charge A    : %.1f A\n", d.batteryChargeAmps);
    Serial.printf("    Percentage  : %d %%\n", d.batteryPercentage);
    Serial.printf("    Status      : %s\n", (d.batteryChargeAmps > 0) ? "Charging" : ((d.batteryDischargeAmps > 0) ? "Discharging" : "Full"));

    Serial.println("  [ SOLAR PV ]\n");
    Serial.printf("    Input V     : %.1f V\n", d.pv1InputVoltage);
    Serial.printf("    Input W     : %d W\n", d.pv1InputWatts);
    Serial.printf("    Inverter Temp    : %d *C\n", d.inverterTemp);

    Serial.println("  [ FLAGS ]\n");
    // Flags to be added here
  }

  Serial.println("MOD Data Valid: " + String(inverter.mod.valid ? "YES" : "NO"));
  if (inverter.mod.valid)
  {
    MODData &d = inverter.mod;
    Serial.println("  [ RAW RESPONSE - MOD Command]\n");
    Serial.println("    " + d.rawResponse + "\n\n");
    Serial.println("  [ MODE ]\n");
    Serial.println("    Output Mode : " + d.outputMode + "\n");
  }

  Serial.println("PIRI Data Valid: " + String(inverter.piri.valid ? "YES" : "NO"));
  if (inverter.piri.valid)
  {
    PIRIData &d = inverter.piri;
    Serial.println("  [ RAW RESPONSE - PIRI Command]\n");
    Serial.println("    " + d.rawResponse + "\n\n");
    Serial.println("  [ RATED PARAMS ]\n");
    Serial.printf("    Capacity    : %d W / %d VA\n", d.outputWattRating, d.outputVARating);
    Serial.printf("    Batt Float  : %.1f V\n", d.battFloatVoltage);
    Serial.printf("    Batt Bulk   : %.1f V\n", d.battBulkChargingVoltage);
    Serial.printf("    Batt Under V: %.1f V\n", d.battCutoffVoltage);
    Serial.println("    Batt Type   : " + d.batteryConfigType + "\n");
    Serial.println("    Output Prio : " + d.outputSourcePriority + "\n");
    Serial.println("    Charger Prio: " + d.chargerSourcePriority + "\n");
  }

  Serial.println("ACCT Data Valid: " + String(inverter.acct.valid ? "YES" : "NO"));
  if (inverter.acct.valid)
  {
    Serial.println("  [ AC CHARGE TIME ]\n");
    Serial.println("    Start       : " + inverter.acct.startTime + "\n");
    Serial.println("    End         : " + inverter.acct.endTime + "\n");
  }

  Serial.println("ACLT Data Valid: " + String(inverter.aclt.valid ? "YES" : "NO"));
  if (inverter.aclt.valid)
  {
    Serial.println("  [ AC LOAD TIME ]\n");
    Serial.println("    Start       : " + inverter.aclt.startTime + "\n");
    Serial.println("    End         : " + inverter.aclt.endTime + "\n");
  }

  Serial.println("FLAG Data Valid: " + String(inverter.flag.valid ? "YES" : "NO"));
  if (inverter.flag.valid)
  {
    FLAGData &f = inverter.flag;
    Serial.println("  [ RAW RESPONSE - FLAG Command]\n");
    Serial.println("    " + f.rawResponse + "\n\n");
    Serial.println("  [ FLAGS ]\n");
    Serial.println("    Buzzer           : " + String(f.buzzerEnabled ? "ON" : "off") + "\n");
    Serial.println("    LCD Timeout      : " + String(f.lcdTimeout ? "ON" : "off") + "\n");
    Serial.println("    Overload Bypass  : " + String(f.overloadBypass ? "ON" : "off") + "\n");
    Serial.println("    Overload Restart : " + String(f.overloadRestart ? "ON" : "off") + "\n");
    Serial.println("    Overtemperature Restart  : " + String(f.overTempRestart ? "ON" : "off") + "\n");
    Serial.println("    Fault Log        : " + String(f.faultCodeRecord ? "ON" : "off") + "\n");
  }

  Serial.println("FWV Data Valid: " + String(inverter.fwv.valid ? "YES" : "NO"));
  if (inverter.fwv.valid)
  {
    Serial.println("  [ FIRMWARE VERSION ]\n");
    Serial.println("  [RAW REPONSE - FWV COMMAND]\n" + inverter.fwv.rawResponse + "\n\n");
  }

  Serial.println("DID Data Valid: " + String(inverter.did.valid ? "YES" : "NO"));
  if (inverter.did.valid)
  {
    Serial.println("  [ Device Serial ]\n");
    Serial.println("  [RAW REPONSE - DID COMMAND]\n" + inverter.did.rawResponse + "\n\n");
  }

  Serial.println("PID Data Valid: " + String(inverter.pid.valid ? "YES" : "NO"));
  if (inverter.pid.valid)
  {
    Serial.println("  [ Protocol VERSION ]\n");
    Serial.println("  [RAW REPONSE - PID COMMAND]\n" + inverter.pid.rawResponse + "\n\n");
  }

  Serial.println("=========================================");
}

String generateSerialDebug()
{
    String s;
    s.reserve(3000);
    s += "=========================================\n";
    s += "   DEBUG LOG (last 20 lines)\n";
    s += "=========================================\n";
    s += dbgDump();
    s += "\n\n";
    s += "=========================================\n";
    s += "   INFINISOLAR VII 5KW  -  LIVE DATA\n";
    s += "   " + (inverter.lastUpdated.length() ? inverter.lastUpdated : "--") + "\n";
    s += "=========================================\n\n";

    // =========================================================
    // ESP32 MEMORY
    // =========================================================
    s += "[ ESP32 MEMORY ]\n\n";
    s += "    Free Heap   : " + String(ESP.getFreeHeap() / 1024) + " KB\n";
    s += "    Min Heap    : " + String(ESP.getMinFreeHeap() / 1024) + " KB\n";
    s += "    Heap Size   : " + String(ESP.getHeapSize() / 1024) + " KB\n";
    if (psramFound())
    {
        s += "    Free PSRAM  : " + String(ESP.getFreePsram() / 1024) + " KB\n";
        s += "    Total PSRAM : " + String(ESP.getPsramSize() / 1024) + " KB\n";
    }
    else
    {
        s += "    PSRAM       : NOT FOUND\n";
    }
    s += "\n\n";

    // =========================================================
    // GSX
    // =========================================================
    s += "GSX Data Valid: ";
    s += inverter.gsx.valid ? "YES\n" : "NO\n";
    s += "Last Update: " + String(inverter.gsx.lastCmdUpdate) + " ms\n";
    GSXData& gsx = inverter.gsx;
    s += "  [ RAW RESPONSE - GSX Command ]\n\n";
    s += "    " + gsx.rawResponse + "\n\n";
    s += "  [ GRID ]\n\n";
    if (gsx.gridVoltage < 10.0)
        s += "    Status      : OFFLINE\n";
    else
        s += "    Status      : Online\n";
    s += "    Voltage     : " + String(gsx.gridVoltage, 1) + " V\n";
    s += "    Frequency   : " + String(gsx.gridFrequency, 1) + " Hz\n";
    s += "    Grid Avail  : ";
    s += gsx.gridAvailable ? "YES\n" : "NO\n";
    s += "    Est Grid Pwr: " + String(gsx.estimatedGridPower, 1) + " W\n";

    // Grid Direction: 0: Idle, 1: Grid Import, 2: Grid Export
    String gridDirStr = (gsx.gridPowerDirection == 1) ? "Import" : (gsx.gridPowerDirection == 2) ? "Export" : "Idle";
    s += "    Power Dir   : " + String(gsx.gridPowerDirection) + " (" + gridDirStr + ")\n\n";

    s += "  [ OUTPUT ]\n\n";
    s += "    Voltage     : " + String(gsx.outputVoltage, 1) + " V\n";
    s += "    Frequency   : " + String(gsx.outputFrequency, 1) + " Hz\n";
    s += "    Active Pwr  : " + String(gsx.outputWatts) + " W\n";
    s += "    Apparent Pwr: " + String(gsx.outputVA) + " VA\n";
    s += "    Load        : " + String(gsx.loadPercent) + " %\n";

    // Load Connected: 0: Disconnected, 1: Connected
    String loadConnStr = (gsx.loadConnected == 1) ? "Connected" : "Disconnected";
    s += "    Load Conn   : " + String(gsx.loadConnected) + " (" + loadConnStr + ")\n\n";

    s += "  [ BATTERY ]\n\n";
    s += "    Voltage     : " + String(gsx.batteryVoltage, 1) + " V\n";
    s += "    Volt SCC1   : " + String(gsx.batteryVoltSCC1) + " V\n";
    s += "    Volt SCC2   : " + String(gsx.batteryVoltSCC2) + " V\n";
    s += "    Discharge A : " + String(gsx.batteryDischargeAmps, 1) + " A\n";
    s += "    Charge A    : " + String(gsx.batteryChargeAmps, 1) + " A\n";
    s += "    Discharge W : " + String(gsx.batteryDischargePower, 1) + " W\n";
    s += "    Charge W    : " + String(gsx.batteryChargePower, 1) + " W\n";
    s += "    Percentage  : " + String(gsx.batteryPercentage) + " %\n";

    // Battery Direction: 0: Idle, 1: Charging, 2: Discharging
    String battDirStr = (gsx.batteryPowerDirection == 1) ? "Charging" : (gsx.batteryPowerDirection == 2) ? "Discharging" : "Full / Idle";
    s += "    Status      : " + String(gsx.batteryPowerDirection) + " (" + battDirStr + ")\n\n";

    s += "  [ SOLAR PV ]\n\n";
    s += "    PV1 Input V : " + String(gsx.pv1InputVoltage, 1) + " V\n";
    s += "    PV1 Input W : " + String(gsx.pv1InputWatts) + " W\n";
    s += "    PV2 Input V : " + String(gsx.pv2InputVoltage, 1) + " V\n";
    s += "    PV2 Input W : " + String(gsx.pv2InputWatts) + " W\n";

    // MPPT Charger Status: 0: Disconnected, 1: Connected
    String mppt1Str = (gsx.mppt1ChargerStatus == 1) ? "Connected" : (gsx.mppt1ChargerStatus == 2) ? "Charging" : "Disconnected";
    String mppt2Str = (gsx.mppt2ChargerStatus == 1) ? "Connected" : (gsx.mppt2ChargerStatus == 2) ? "Charging" : "Disconnected";
    s += "    MPPT1 Status: " + String(gsx.mppt1ChargerStatus) + " (" + mppt1Str + ")\n";
    s += "    MPPT2 Status: " + String(gsx.mppt2ChargerStatus) + " (" + mppt2Str + ")\n";
    s += "    Inverter Temp : " + String(gsx.inverterTemp) + " *C\n";
    s += "    MPPT1 Temp  : " + String(gsx.mpptCharger1Temp) + " *C\n";
    s += "    MPPT2 Temp  : " + String(gsx.mpptCharger2Temp) + " *C\n\n";

    s += "  [ DAILY COUNTERS ]\n\n";
    s += "    Total Output Units: " + String(gsx.dailyOutputEnergyKWh, 3) + " kWh\n";
    s += "    Total PV Input Units: " + String(gsx.dailyPVInputKWh, 3) + " kWh\n";
    s += "    Total Grid Import Units: " + String(gsx.dailyEstimatedGridKWh, 3) + " kWh\n";
    s += "    Total Battery Charged Units: " + String(gsx.dailyBatteryChargeKWh, 3) + " kWh\n";
    s += "    Total Battery Discharged Units: " + String(gsx.dailyBatteryDischargeKWh, 3) + " kWh\n\n";

    s += "  [ FLAGS & SYSTEM ]\n\n";
    // Config State: 0: Unchanged, 1: Changed
    String cfgStr = (gsx.configState == 1) ? "Changed" : "Unchanged";
    s += "    Config State  : " + String(gsx.configState) + " (" + cfgStr + ")\n";

    // DC/AC Direction: 0: Idle, 1: AC to DC, 2: DC to AC
    String dcAcStr = (gsx.dcAcDirection == 1) ? "AC to DC" : (gsx.dcAcDirection == 2) ? "DC to AC" : "Idle";
    s += "    DC/AC Dir     : " + String(gsx.dcAcDirection) + " (" + dcAcStr + ")\n";

    // Parallel ID: 0: Single, N: Nth Number
    String parallelStr = (gsx.localParallelID == 0) ? "Single" : ("ID " + String(gsx.localParallelID));
    s += "    Parallel ID   : " + String(gsx.localParallelID) + " (" + parallelStr + ")\n\n";

    // =========================================================
    // MOD
    // =========================================================
    s += "MOD Data Valid: ";
    s += inverter.mod.valid ? "YES\n" : "NO\n";
    s += "Last Update: " + String(inverter.mod.lastCmdUpdate) + " ms\n";
    MODData& mod = inverter.mod;
    s += "  [ RAW RESPONSE - MOD Command ]\n\n";
    s += "    " + mod.rawResponse + "\n\n";
    s += "  [ MODE ]\n\n";
    s += "    Output Mode : " + mod.outputMode + "\n\n";

    // =========================================================
    // PIRI
    // =========================================================
    s += "PIRI Data Valid: ";
    s += inverter.piri.valid ? "YES\n" : "NO\n";
    s += "Last Update: " + String(inverter.piri.lastCmdUpdate) + " ms\n";
    PIRIData& piri = inverter.piri;
    s += "  [ RAW RESPONSE - PIRI Command ]\n\n";
    s += "    " + piri.rawResponse + "\n\n";
    s += "  [ RATED PARAMS ]\n\n";
    s += "    Grid Volt Rating : " + String(piri.gridVoltageRating, 1) + " V\n";
    s += "    Grid Curr Rating : " + String(piri.gridCurrentRating, 1) + " A\n";
    s += "    Load Volt Rating : " + String(piri.outputVoltageRating, 1) + " V\n";
    s += "    Load Freq Rating : " + String(piri.outputFreqRating, 1) + " Hz\n";
    s += "    Load Curr Rating : " + String(piri.outputCurrentRating, 1) + " A\n";
    s += "    Capacity         : " + String(piri.outputWattRating) + " W / " + String(piri.outputVARating) + " VA\n";
    s += "    Batt Volt Rating : " + String(piri.battVoltageRating, 1) + " V\n";
    s += "    Batt Stop Disch Grid  : " + String(piri.battStopDischargeVoltageOnGrid, 1) + " V\n";
    s += "    Batt Stop Charge Grid : " + String(piri.battStopChargeVoltageOnGrid, 1) + " V\n";
    s += "    Batt Float       : " + String(piri.battFloatVoltage, 1) + " V\n";
    s += "    Batt Bulk        : " + String(piri.battBulkChargingVoltage, 1) + " V\n";
    s += "    Batt Low Cutoff Volt : " + String(piri.battCutoffVoltage, 1) + " V\n";

    // Battery Config: 0: AGM, 1: Flooded, 2: User-defined
    String battCfgStr = (piri.batteryConfigType == "0") ? "AGM" : (piri.batteryConfigType == "1") ? "Flooded" : (piri.batteryConfigType == "2") ? "User-defined" : piri.batteryConfigType;
    s += "    Batt Type        : " + piri.batteryConfigType + " (" + battCfgStr + ")\n";

    s += "    Max AC Charge A  : " + String(piri.maxACChargeCurrent) + " A\n";
    s += "    Max Total Charge A: " + String(piri.maxTotalChargeCurrent) + " A\n";

    // Voltage Range: 0: APL (90-280V), 1: UPS (170-280V)
    String vRangeStr = (piri.inputVoltageRange == "0") ? "APL (90-280V)" : (piri.inputVoltageRange == "1") ? "UPS (170-280V)" : piri.inputVoltageRange;
    s += "    Input Volt Range : " + piri.inputVoltageRange + " (" + vRangeStr + ")\n";

    // Output Source Priority: 0: SUB, 1: SBU
    String outPrioStr = (piri.outputSourcePriority == "0") ? "SUB" : (piri.outputSourcePriority == "1") ? "SBU" : piri.outputSourcePriority;
    s += "    Output Prio      : " + piri.outputSourcePriority + " (" + outPrioStr + ")\n";

    // Charger Source Priority: 0: CSO, 1: SNU, 2: OSO
    String chgPrioStr = (piri.chargerSourcePriority == "0") ? "CSO (Solar First)" : (piri.chargerSourcePriority == "1") ? "SNU (Solar & Utility)" : (piri.chargerSourcePriority == "2") ? "OSO (Only Solar)" : piri.chargerSourcePriority;
    s += "    Charger Prio     : " + piri.chargerSourcePriority + " (" + chgPrioStr + ")\n";

    // Parallel Type: 9: Single, N: Parallel ID N
    String parTypeStr = (piri.parallelInverterType == "9") ? "Single" : ("Parallel ID " + piri.parallelInverterType);
    s += "    Parallel Type    : " + piri.parallelInverterType + " (" + parTypeStr + ")\n";

    // Topology: 0: Transformer-less, 1: Transformer-based
    String topoStr = (piri.topology == "0") ? "Transformer-less" : (piri.topology == "1") ? "Transformer-based" : piri.topology;
    s += "    Topology         : " + piri.topology + " (" + topoStr + ")\n";

    // Output Model Setting: 0: Single-phase, 1: 3 Phase P1, 2: 3 Phase P2
    String outModelStr = (piri.outputModeSetting == "0") ? "Single-phase" : (piri.outputModeSetting == "1") ? "3 Phase P1" : (piri.outputModeSetting == "2") ? "3 Phase P2" : piri.outputModeSetting;
    s += "    Output Model Set : " + piri.outputModeSetting + " (" + outModelStr + ")\n";

    // Solar Power Priority: 0: BLU, 1: LBU
    String solarPrioStr = (piri.solarPowerPriority == "0") ? "BLU (Battery-Load-Utility)" : (piri.solarPowerPriority == "1") ? "LBU (Load-Battery-Utility)" : piri.solarPowerPriority;
    s += "    Solar Power Prio : " + piri.solarPowerPriority + " (" + solarPrioStr + ")\n";

    // MPPT Trackers: 0/1: Single-channel
    String mpptCountStr = (piri.mpptStringsCount <= 1) ? "Single-channel" : String(piri.mpptStringsCount) + "-channels";
    s += "    MPPT Strings     : " + String(piri.mpptStringsCount) + " (" + mpptCountStr + ")\n";

    s += "    CPU Subcode      : " + String(piri.cpuSubcode) + "\n\n";

    // =========================================================
    // ACCT
    // =========================================================
    s += "ACCT Data Valid: ";
    s += inverter.acct.valid ? "YES\n" : "NO\n";
    s += "Last Update: " + String(inverter.acct.lastCmdUpdate) + " ms\n";
    ACCTData& _acct = inverter.acct;
    s += "  [ AC CHARGE TIME ]\n\n";
    s += "    Start       : " + _acct.startTime + "\n";
    s += "    End         : " + _acct.endTime + "\n";
    s += "    Raw Response: " + _acct.rawResponse + "\n\n";

    // =========================================================
    // ACLT
    // =========================================================
    s += "ACLT Data Valid: ";
    s += inverter.aclt.valid ? "YES\n" : "NO\n";
    s += "Last Update: " + String(inverter.aclt.lastCmdUpdate) + " ms\n";
    ACLTData& _aclt = inverter.aclt;
    s += "  [ AC LOAD TIME ]\n\n";
    s += "    Start       : " + _aclt.startTime + "\n";
    s += "    End         : " + _aclt.endTime + "\n";
    s += "    Raw Response: " + _aclt.rawResponse + "\n\n";

    // =========================================================
    // FLAG
    // =========================================================
    s += "FLAG Data Valid: ";
    s += inverter.flag.valid ? "YES\n" : "NO\n";
    s += "Last Update: " + String(inverter.flag.lastCmdUpdate) + " ms\n";
    FLAGData& f = inverter.flag;
    s += "  [ RAW RESPONSE - FLAG Command ]\n\n";
    s += "    " + f.rawResponse + "\n\n";
    s += "  [ FLAGS ]\n\n";
    s += "    Buzzer                  : ";
    s += f.buzzerEnabled ? "1 (ON)\n" : "0 (off)\n";
    s += "    Overload Bypass        : ";
    s += f.overloadBypass ? "1 (ON)\n" : "0 (off)\n";
    s += "    LCD Timeout            : ";
    s += f.lcdTimeout ? "1 (ON)\n" : "0 (off)\n";
    s += "    Overload Restart       : ";
    s += f.overloadRestart ? "1 (ON)\n" : "0 (off)\n";
    s += "    Overtemperature Restart: ";
    s += f.overTempRestart ? "1 (ON)\n" : "0 (off)\n";
    s += "    Backlight              : ";
    s += f.backlightOn ? "1 (ON)\n" : "0 (off)\n";
    s += "    Primary Source Alarm   : ";
    s += f.alarmPrimaryInput ? "1 (ON)\n" : "0 (off)\n";
    s += "    Fault Log              : ";
    s += f.faultCodeRecord ? "1 (ON)\n" : "0 (off)\n";
    s += "\n\n";

    s += "FWV Data Valid: ";
    s += inverter.fwv.valid ? "YES\n" : "NO\n";
    s += "Last Update: " + String(inverter.fwv.lastCmdUpdate) + " ms\n";
    FWVData& fwv = inverter.fwv;
    s += "  [ RAW RESPONSE - FWV Command ]\n\n";
    s += "    " + fwv.rawResponse + "\n\n";

    s += "DID Data Valid: ";
    s += inverter.did.valid ? "YES\n" : "NO\n";
    s += "Last Update: " + String(inverter.did.lastCmdUpdate) + " ms\n";
    DIDData& did = inverter.did;
    s += "  [ RAW RESPONSE - DID Command ]\n\n";
    s += "    " + did.rawResponse + "\n\n";

    s += "PID Data Valid: ";
    s += inverter.pid.valid ? "YES\n" : "NO\n";
    s += "Last Update: " + String(inverter.pid.lastCmdUpdate) + " ms\n";
    PIDData& pid = inverter.pid;
    s += "  [ RAW RESPONSE - PID Command ]\n\n";
    s += "    " + pid.rawResponse + "\n\n";

    s += "CLOUD UPDATE\n";
    s += "Update Interval: " + String(intervalCloudUpdate) + " ms\n";
    s += "Boot Restore Resolved: " + String(cloudCountersRestored ? "YES" : "NO (retrying)") + "\n";
    s += "Upload Pending: " + String(inverter.cloud.cloudUploadPending ? "YES" : "NO") + "\n";
    s += "Queue RAM: " + String(cqRamCount) + " / " + String(cqRamCap) + " records (" + String(cqRamInPsram ? "PSRAM" : "internal") + ")\n";
    s += "Queue Flash: " + (cqFlashOk ? (String(cqFlashRecords) + " records, FS " + String(LittleFS.usedBytes() / 1024) + " / " + String(LittleFS.totalBytes() / 1024) + " KB used") : String("not available (RAM only)")) + "\n";
    s += "Queue Dropped: " + String(cqDropped) + " | Heap free " + String(ESP.getFreeHeap() / 1024) + " KB, largest block " + String(ESP.getMaxAllocHeap() / 1024) + " KB\n";
    s += "Last Update: " + inverter.cloud.lastSuccessfulCloudUpdate + "\n";
    s += "Last Response: " + inverter.cloud.rawResponse + "\n\n";

    s += "\n=========================================\n";

    return s;
}

