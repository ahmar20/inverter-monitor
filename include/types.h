#pragma once
// ============================================================================
// types.h — all shared data structures and the global inverter/runtime state
// that other modules read and write. No behavior lives here.
// ============================================================================

#include <Arduino.h>
#include "config.h"

// ===== Cloud upload payload shape (per-update snapshot sent to the cloud) ====
struct CloudUploadData
{
    float gridImportKWh;
    float pvEnergyKWh;
    float batteryChargeKWh;
    float batteryDischargeKWh;
    float loadEnergyKWh;

    float gridPower;
    float pvVoltage;
    float pvPower;
    float batteryVoltage;
    float batteryPower;
    int   batteryPercentage;
    int   calculatedSoc = 0;
    float loadPower;
};
CloudUploadData cloudUploadData;

// true once the boot-time restore has been resolved either way: cloud data
// merged in (same day), or cloud data was from a previous day and skipped.
// Stays false (and gets retried) on network/parse failures so we don't start
// uploading - and potentially overwrite a larger cloud total with a smaller
// local-only one - before we've actually checked what the cloud has.
bool cloudCountersRestored = false;
bool cloudRecoveryInProgress = false;
TaskHandle_t cloudUploadTaskHandle = NULL;
volatile bool cloudUploadRunning = false;
unsigned long lastCloudUploadAttempt = 0;   // millis() of last upload kick-off, for non-blocking pacing
unsigned long lastCloudRestoreAttempt = 0;  // millis() of last restore kick-off, for retry pacing

// ── Coulomb counting SOC ─────────────────────────────────────
float   ccSoc          = -1.0f;   // -1 = not initialised yet
float   CC_CAPACITY_WH = CC_CAPACITY_WH_DEFAULT;
bool    ccRestored     = false;   // Whether CC SOC was restored from cloud
bool    ccInitialised  = false;   // true once seeded from a valid reading
uint8_t ccCutoffCount  = 0;       // consecutive readings at cutoff voltage
uint8_t ccCutoffThreshold = CC_CUTOFF_THRESHOLD_DEFAULT;

// ─── Inverter General Status ─────────────────────────────────────
typedef struct GSXData {
  // Grid
  float gridVoltage = 0.0;            // V   (field 0  ÷ 10)
  float gridFrequency = 0.0;          // Hz  (field 1  ÷ 10)
  bool  gridAvailable = false;
  float estimatedGridPower = 0.0;
  // Output
  float outputVoltage = 0.0;          // V   (field 2  ÷ 10)
  float outputFrequency = 0.0;        // Hz  (field 3  ÷ 10)
  int   outputVA = 0;                 // VA  (field 4)
  int   outputWatts = 0;              // W   (field 5)
  float dailyOutputEnergyKWh = 0;     // daily
  int   loadPercent = 0;              // %   (field 6)
  // Battery
  float batteryVoltage = 0.0;         // V   (field 7  ÷ 10)
  int   batteryVoltSCC1 = 0;          // V   (field 8  ÷ 10) -- we dont use this
  int   batteryVoltSCC2 = 0;          // V   (field 9  ÷ 10) -- we dont use this
  float batteryDischargeAmps = 0.0;   // A   (field 10)
  float batteryChargeAmps = 0.0;      // A   (field 11)
  int   batteryPercentage = 0;        // %   (field 12)
  float batteryChargePower = 0.0;
  float batteryDischargePower = 0.0;
  float dailyBatteryChargeKWh = 0.0;  // daily
  float dailyBatteryDischargeKWh = 0.0; // daily
  // PV / Solar
  int   inverterTemp = 0;             // *C  (field 13)
  int   mpptCharger1Temp = 0;         // *C  (field 14)
  int   mpptCharger2Temp = 0;         // *C  (field 15)
  int   pv1InputWatts = 0;            // W   (field 16)
  int   pv2InputWatts = 0;            // W   (field 17)
  float pv1InputVoltage = 0.0;        // V   (field 18 ÷ 10)
  float pv2InputVoltage = 0.0;        // V   (field 19 ÷ 10)
  float dailyPVInputKWh = 0.0;        // daily
  float dailyEstimatedGridKWh = 0.0;  // daily
  // Flags
  int configState = 0;                // (field 20) Configuration --> 0: Unchanged, 1: Changed
  int mppt1ChargerStatus = 0;         // (field 21) MPPT 1 Charger --> 0: Disconnected, 1: Connected
  int mppt2ChargerStatus = 0;         // (field 22) MPPT 2 Charger --> 0: Disconnected, 1: Connected
  int loadConnected = 0;              // (field 23) Load Connected --> 0: Disconnected, 1: Connected
  int batteryPowerDirection = 0;      // (field 24) Battery Power --> 0: Idle, 1: Charging, 2: Discharging
  int dcAcDirection = 0;              // (field 25) DC/AC Power --> 0: Idle, 1: AC to DC, 2: DC to AC
  int gridPowerDirection = 0;         // (field 26) Grid Power --> 0: Idle, 1: Grid Import. 2: Grid Export
  int localParallelID = 0;            // (field 27) Inverter in Parallel --> 0: Single, N: Nth Number
  String rawResponse = "";
  bool  valid = false;
  unsigned long lastCmdUpdate = 0;
} GSXData;

typedef struct {
  String outputMode = "";          // field 0 -- only one field in response
  String rawResponse = "";
  bool   valid = false;
  unsigned long lastCmdUpdate = 0;
} MODData;

typedef struct {
  bool buzzerEnabled = false;      // (field 0) BUZZ 0: Off, 1:ON
  bool overloadBypass = false;     // (field 1) OLBP 0: Off, 1:ON
  bool lcdTimeout = false;         // (field 2) LCDE 0: Off, 1:ON
  bool overloadRestart = false;    // (field 3) OLRS 0: Off, 1:ON
  bool overTempRestart = false;    // (field 4) OTRS 0: Off, 1:ON
  bool backlightOn = false;        // (field 5) BLON 0: Off, 1:ON
  bool alarmPrimaryInput = false;  // (field 6) ALRM 0: Off, 1:ON
  bool faultCodeRecord = false;    // (field 7) FTCR 0: Off, 1:ON
  String rawResponse = "";
  bool valid = false;
  unsigned long lastCmdUpdate = 0;
} FLAGData;

// ─── PIRI — Rated Parameters ─────────────────────────────────────
typedef struct {
  float gridVoltageRating = 0.0;            // V (field 0) ÷ 10
  float gridCurrentRating = 0.0;            // A (field 1) ÷ 10
  float outputVoltageRating = 0.0;          // V (field 2) ÷ 10
  float outputFreqRating = 0.0;             // Hz (field 3) ÷ 10
  float outputCurrentRating = 0.0;          // A (field 4) ÷ 10
  int   outputVARating = 0;                 // VA (field 5)
  int   outputWattRating = 0;               // W (field 6)
  float battVoltageRating = 0.0;            // V (field 7) ÷ 10
  float battStopDischargeVoltageOnGrid = 0.0; // V (field 8) ÷ 10
  float battStopChargeVoltageOnGrid = 0.0;  // V (field 9) ÷ 10
  float battCutoffVoltage = 0.0;            // V (field 10) ÷ 10
  float battBulkChargingVoltage = 0.0;      // V (field 11) ÷ 10
  float battFloatVoltage = 0.0;             // V (field 12) ÷ 10
  String batteryConfigType = "";            // int (field 13) 0: AGM, 1: Flooded, 2: User-defined
  int   maxACChargeCurrent = 0;             // A (field 14)
  int   maxTotalChargeCurrent = 0;          // A (field 15)
  String inputVoltageRange = "";            // int (field 16) 0: APL (90-280V), 1: UPS (170-280V)
  String outputSourcePriority = "";         // int (field 17) 0: SUB, 1: SBU
  String chargerSourcePriority = "";        // int (field 18) 0: CSO, 1: SNU, 2: OSO
  String parallelInverterType = "";         // int (field 19) 9: Single
  String machineType = "";                  // int (field 20) 0: Off-Grid, 1: Hybrid
  String topology = "";                     // int (field 21) 0: Transformer-less, 1: Transformer-based
  String outputModeSetting = "";            // int (field 22) 0: Single-phase, 1: 3 Phase P1, 2: 3 Phase P2
  String solarPowerPriority = "";           // int (field 23) 0: BLU, 1: LBU
  int    mpptStringsCount = 0;              // int (field 24) 1: Single String
  int    cpuSubcode = 0;                    // int (field 25) TBD
  String rawResponse = "";
  bool  valid = false;
  unsigned long lastCmdUpdate = 0;
} PIRIData;

typedef struct {
  String startTime = "";
  String endTime = "";
  String rawResponse = "";
  bool   valid = false;
  unsigned long lastCmdUpdate = 0;
} ACCTData, ACLTData;

typedef struct {
String rawResponse = "";
bool valid = false;
unsigned long lastCmdUpdate = 0;
}FWVData, DIDData, PIDData;

typedef struct {
String rawResponse = "";
bool cloudUploadPending = true;
String lastSuccessfulCloudUpdate = "";
unsigned long lastCmdUpdate = 0;
}CloudUpdateData;

// Delta tracking for incremental cloud uploads
static float lastUploadedPvKWh    = 0.0;
static float lastUploadedGridKWh  = 0.0;
static float lastUploadedBattCKWh = 0.0;
static float lastUploadedBattDKWh = 0.0;
static float lastUploadedLoadKWh  = 0.0;
static bool  deltaInitialised     = false;

typedef struct {
  GSXData      gsx;
  MODData      mod;
  FLAGData     flag;
  PIRIData     piri;
  ACCTData     acct;
  ACLTData     aclt;
  FWVData      fwv;
  DIDData      did;
  PIDData      pid;
  CloudUpdateData cloud;
  String       lastUpdated;
} InverterState;

InverterState inverter;

unsigned long lastSuccessfulInverterResponse = 0;
int lastResetYday = -1;

// Non-blocking inverter communication
bool commandInProcess = false;
String inverterResponse = "";
unsigned long commandStartTime = 0;
// Pointer to currently active command
const byte* activeCommand = nullptr;
size_t activeCommandLength = 0;
