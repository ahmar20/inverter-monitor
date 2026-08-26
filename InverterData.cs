using nanoFramework.Hardware.Esp32;
using System;
using System.Collections;
using System.IO.Ports;
using System.Text;

namespace inverter_monitor
{
    internal class Inverter
    {
        /// <summary>
        /// Inverter Serial Protocols
        /// </summary>
        public static SerialPort InverterSerialPortObject;
        public const string inverterSerialPortName = "COM2";
        public const int InverterSerialBaudRate = 2400;
        public const int InverterSerialTXPin = 16;
        public const int InverterSerialRXPin = 17;
        public const int InverterPollingInterval = 1000;
        public static long InverterSerialLastPollingTime = 0;
        public static bool InverterSerialCommandInProcess = false;

        public enum InverterCommand
        {
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
        public static InverterCommand ActiveCommandType = InverterCommand.CMD_NONE;

        public static string StartInverterCommand(InverterCommand cmd)
        {
            ActiveCommandType = cmd;
            string response = "";
            switch (cmd)
            {
                case InverterCommand.CMD_GSX:
                    response = SendCommand(GSX_CMD, GSX_CMD.Length);
                    break;
                case InverterCommand.CMD_MOD:
                    response = SendCommand(MOD_CMD, MOD_CMD.Length);
                    break;
                case InverterCommand.CMD_FLAG:
                    response = SendCommand(FLAG_CMD, FLAG_CMD.Length);
                    break;
                case InverterCommand.CMD_PIRI:
                    response = SendCommand(PIRI_CMD, PIRI_CMD.Length);
                    break;
                case InverterCommand.CMD_ACCT:
                    response = SendCommand(ACCT_CMD, ACCT_CMD.Length);
                    break;
                case InverterCommand.CMD_ACLT:
                    response = SendCommand(ACLT_CMD, ACLT_CMD.Length);
                    break;
                case InverterCommand.CMD_FWV:
                    response = SendCommand(FWV_CMD, FWV_CMD.Length);
                    break;
                case InverterCommand.CMD_PID:
                    response = SendCommand(PID_CMD, PID_CMD.Length);
                    break;
                case InverterCommand.CMD_DID:
                    response = SendCommand(DID_CMD, DID_CMD.Length);
                    break;

                default:
                    ActiveCommandType = InverterCommand.CMD_NONE;
                    break;
            }
            return response;
        }

        private static string SendCommand(byte[] cmd, int cmdLength)
        {
            Helpers.SetLedState(Helpers.LED_State.Query);
            while (InverterSerialPortObject.BytesToRead > 0)
            {
                InverterSerialPortObject.ReadExisting();
            }
            InverterSerialPortObject.Write(cmd, 0, cmdLength);
            return ProcessInverterResponse();
        }

        private static string ProcessInverterResponse()
        {
            // Read serial data
            while (InverterSerialPortObject.BytesToRead > 0)
            {
                Helpers.SetLedState(Helpers.LED_State.Response);
                string r = InverterSerialPortObject.ReadExisting();
                // Check response
                if (Helpers.ErrorInResponse(r))
                {
                    Helpers.SetLedState(Helpers.LED_State.Fault);
                    Helpers.Log("WARNING: Bad response from inverter.");
                    Helpers.Log("[ERROR] Raw Response: " + r);
                }
                return r;
            }
            return null;
        }

        /// <summary>
        /// Inverter Data Classes
        /// </summary>
        public class InverterData
        {
            public static InverterData InverterSerialDataObject => new();
            public GSXData GS;
            public MODData MOD;
            public FLAGData FLAG;
            public PIRIData PIRI;
            public ACCTData ACCT;
            public ACLTData ACLT;
            //public FWVData FWV;
            //public DIDData DId;
            //public PIDData PId;
            public CloudUpdateData cloud;
        }

        public class GSXData
        {
            // Grid
            public double gridVoltage = 0.0;            // V   (field 0  ÷ 10)
            public double gridFrequency = 0.0;          // Hz  (field 1  ÷ 10)
            public bool gridAvailable = false;
            public double estimatedGridPower = 0.0;
            // Output
            public double outputVoltage = 0.0;          // V   (field 2  ÷ 10)
            public double outputFrequency = 0.0;        // Hz  (field 3  ÷ 10)
            public int outputVA = 0;                    // VA  (field 4)
            public int outputWatts = 0;                 // W   (field 5)
            public double dailyOutputEnergyKWh = 0;     // daily
            public int loadPercent = 0;                 // %   (field 6)
            // Battery
            public double batteryVoltage = 0.0;         // V   (field 7  ÷ 10)
            public int batteryVoltSCC1 = 0;             // V   (field 8  ÷ 10) -- we dont use this
            public int batteryVoltSCC2 = 0;             // V   (field 9  ÷ 10) -- we dont use this
            public double batteryDischargeAmps = 0.0;   // A   (field 10)
            public double batteryChargeAmps = 0.0;      // A   (field 11)
            public int batteryPercentage = 0;           // %   (field 12)
            public double batteryChargePower = 0.0;
            public double batteryDischargePower = 0.0;
            public double dailyBatteryChargeKWh = 0.0;  // daily
            public double dailyBatteryDischargeKWh = 0.0; // daily
            // PV / Solar
            public int inverterTemp = 0;                // *C  (field 13)
            public int mpptCharger1Temp = 0;            // *C  (field 14)
            public int mpptCharger2Temp = 0;            // *C  (field 15)
            public int pv1InputWatts = 0;               // W   (field 16)
            public int pv2InputWatts = 0;               // W   (field 17)
            public double pv1InputVoltage = 0.0;        // V   (field 18 ÷ 10)
            public double pv2InputVoltage = 0.0;        // V   (field 19 ÷ 10)
            public double dailyPVInputKWh = 0.0;        // daily
            public double dailyEstimatedGridKWh = 0.0;  // daily
            // Flags
            public int configState = 0;                 // (field 20) Configuration --> 0: Unchanged, 1: Changed
            public int mppt1ChargerStatus = 0;          // (field 21) MPPT 1 Charger --> 0: Disconnected, 1: Connected
            public int mppt2ChargerStatus = 0;          // (field 22) MPPT 2 Charger --> 0: Disconnected, 1: Connected
            public int loadConnected = 0;               // (field 23) Load Connected --> 0: Disconnected, 1: Connected
            public int batteryPowerDirection = 0;       // (field 24) Battery Power --> 0: Idle, 1: Charging, 2: Discharging
            public int dcAcDirection = 0;               // (field 25) DC/AC Power --> 0: Idle, 1: AC to DC, 2: DC to AC
            public int gridPowerDirection = 0;          // (field 26) Grid Power --> 0: Idle, 1: Grid Import. 2: Grid Export
            public int localParallelID = 0;             // (field 27) Inverter in Parallel --> 0: Single, N: Nth Number
            public String rawResponse = "";
            public bool valid = false;
            public long lastCmdUpdate = 0;
        }

        public class MODData
        {
            public string outputMode = "";          // field 0 -- only one field in response
            public string rawResponse = "";
            public bool valid = false;
            public long lastCmdUpdate = 0;
        }

        public class FLAGData
        {
            public bool buzzerEnabled = false;      // (field 0) BUZZ 0: Off, 1:ON
            public bool overloadBypass = false;     // (field 1) OLBP 0: Off, 1:ON
            public bool lcdTimeout = false;         // (field 2) LCDE 0: Off, 1:ON
            public bool overloadRestart = false;    // (field 3) OLRS 0: Off, 1:ON
            public bool overTempRestart = false;    // (field 4) OTRS 0: Off, 1:ON
            public bool backlightOn = false;        // (field 5) BLON 0: Off, 1:ON
            public bool alarmPrimaryInput = false;  // (field 6) ALRM 0: Off, 1:ON
            public bool faultCodeRecord = false;    // (field 7) FTCR 0: Off, 1:ON
            public String rawResponse = "";
            public bool valid = false;
            public long lastCmdUpdate = 0;
        }

        // ─── PIRI — Rated Parameters
        public class PIRIData
        {
            public double gridVoltageRating = 0.0;            // V (field 0) ÷ 10
            public double gridCurrentRating = 0.0;            // A (field 1) ÷ 10
            public double outputVoltageRating = 0.0;          // V (field 2) ÷ 10
            public double outputFreqRating = 0.0;             // Hz (field 3) ÷ 10
            public double outputCurrentRating = 0.0;          // A (field 4) ÷ 10
            public int outputVARating = 0;                   // VA (field 5)
            public int outputWattRating = 0;                 // W (field 6)
            public double battVoltageRating = 0.0;           // V (field 7) ÷ 10
            public double battStopDischargeVoltageOnGrid = 0.0; // V (field 8) ÷ 10
            public double battStopChargeVoltageOnGrid = 0.0; // V (field 9) ÷ 10
            public double battCutoffVoltage = 0.0;           // V (field 10) ÷ 10
            public double battBulkChargingVoltage = 0.0;     // V (field 11) ÷ 10
            public double battFloatVoltage = 0.0;            // V (field 12) ÷ 10
            public string batteryConfigType = "";            // int (field 13) 0: AGM, 1: Flooded, 2: User-defined
            public int maxACChargeCurrent = 0;               // A (field 14)
            public int maxTotalChargeCurrent = 0;            // A (field 15)
            public string inputVoltageRange = "";            // int (field 16) 0: APL (90-280V), 1: UPS (170-280V)
            public string outputSourcePriority = "";         // int (field 17) 0: SUB, 1: SBU
            public string chargerSourcePriority = "";        // int (field 18) 0: CSO, 1: SNU, 2: OSO
            public string parallelInverterType = "";         // int (field 19) 9: Single, N: Parallel ID N
            public string topology = "";                     // int (field 20) 0: Transformer-less, 1: Transformer-based
            public string outputModelSetting = "";           // int (field 21) 0: Single-phase, 1: 3 Phase P1, 2: 3 Phase P2
            public string solarPowerPriority = "";           // int (field 22) 0: BLU, 1: LBU
            public int mpptTrackerCount = 0;                 // int (field 23) 0/1: Single-channel
            public string pvOkConditionConfig = "";          // int (field 24) 0: PV Voltage > 0, 1: PV Voltage is > Battery Voltage
            public int cpuSubcode = 0;                       // int (field 25) TBD
            public string rawResponse = "";
            public bool valid = false;
            public long lastCmdUpdate = 0;
        }

        public class ACCTData
        {
            public string startTime = "";
            public string endTime = "";
            public string rawResponse = "";
            public bool valid = false;
            public long lastCmdUpdate = 0;
        }

        public class ACLTData
        {
            public string startTime = "";
            public string endTime = "";
            public string rawResponse = "";
            public bool valid = false;
            public long lastCmdUpdate = 0;
        }

        public class FWVData
        {
            public string rawResponse = "";
            public bool valid = false;
            public long lastCmdUpdate = 0;
        }

        public class DIDData
        {
            public string rawResponse = "";
            public bool valid = false;
            public long lastCmdUpdate = 0;
        }

        public class PIDData
        {
            public string rawResponse = "";
            public bool valid = false;
            public long lastCmdUpdate = 0;
        }

        public class CloudUpdateData
        {
            public string rawResponse = "";
            public bool cloudUploadPending = true;
            public string lastSuccessfulCloudUpdate = "";
            public long lastCmdUpdate = 0;
        }
        
        /// <summary>
        /// ===== Inverter Commands =====
        /// </summary>
        static byte[] GSX_CMD  => new byte[10] {0x5E, 0x50, 0x30, 0x30, 0x35, 0x47, 0x53, 0x58, 0x14, 0x0D};
        static byte[] MOD_CMD  => new byte[11] {0x5E, 0x50, 0x30, 0x30, 0x36, 0x4D, 0x4F, 0x44, 0xDD, 0xBE, 0x0D};
        static byte[] FWV_CMD  => new byte[11] {0x5E, 0x50, 0x30, 0x30, 0x36, 0x46, 0x57, 0x53, 0xC5, 0x43, 0x0D};
        static byte[] FLAG_CMD => new byte[12] {0x5E, 0x50, 0x30, 0x30, 0x37, 0x46, 0x4C, 0x41, 0x47, 0x8E, 0x18, 0x0D};
        static byte[] PIRI_CMD => new byte[12] {0x5E, 0x50, 0x30, 0x30, 0x37, 0x50, 0x49, 0x52, 0x49, 0xEE, 0x38, 0x0D};
        static byte[] DID_CMD  => new byte[10] {0x5E, 0x50, 0x30, 0x30, 0x35, 0x49, 0x44, 0x19, 0xCD, 0x0D};
        static byte[] PID_CMD  => new byte[10] {0x5E, 0x50, 0x30, 0x30, 0x35, 0x50, 0x49, 0x71, 0x8B, 0x0D};
        static byte[] ACCT_CMD => new byte[12] {0x5E, 0x50, 0x30, 0x30, 0x37, 0x41, 0x43, 0x43, 0x54, 0xB7, 0x34, 0x0D};
        static byte[] ACLT_CMD => new byte[12] {0x5E, 0x50, 0x30, 0x30, 0x37, 0x41, 0x43, 0x4C, 0x54, 0xA7, 0x0B, 0x0D};


        // ======== Inverter Data Parsers =========

        public static GSXData ParseGSXCommand(string response)
        {
            GSXData data = new()
            {
                valid = false,
                rawResponse = response
            };
            // Expected format: ^D106<f0>,<f1>,...<CRC_H><CRC_L>
            if (Helpers.ErrorInResponse(response))
                return data;

            string inner = Helpers.StripResponse(response);
            data.gridVoltage = double.Parse(Helpers.SplitField(inner, 0)) / 10.0;
            data.gridFrequency = double.Parse(Helpers.SplitField(inner, 1)) / 10.0;
            data.outputVoltage = double.Parse(Helpers.SplitField(inner, 2)) / 10.0;
            data.outputFrequency = double.Parse(Helpers.SplitField(inner, 3)) / 10.0;
            data.outputVA = int.Parse(Helpers.SplitField(inner, 4));
            data.outputWatts = int.Parse(Helpers.SplitField(inner, 5));
            data.loadPercent = int.Parse(Helpers.SplitField(inner, 6));
            data.batteryVoltage = double.Parse(Helpers.SplitField(inner, 7)) / 10.0;
            data.batteryVoltSCC1 = int.Parse(Helpers.SplitField(inner, 8));
            data.batteryVoltSCC2 = int.Parse(Helpers.SplitField(inner, 9));
            data.batteryDischargeAmps = int.Parse(Helpers.SplitField(inner, 10));
            data.batteryChargeAmps = int.Parse(Helpers.SplitField(inner, 11)); // Need to verify 8, 9, 10, 11 are correct
            data.batteryPercentage = int.Parse(Helpers.SplitField(inner, 12));
            data.inverterTemp = int.Parse(Helpers.SplitField(inner, 13));
            data.mpptCharger1Temp = int.Parse(Helpers.SplitField(inner, 14));
            data.mpptCharger2Temp = int.Parse(Helpers.SplitField(inner, 15));
            data.pv1InputWatts = int.Parse(Helpers.SplitField(inner, 16));
            data.pv2InputWatts = int.Parse(Helpers.SplitField(inner, 17));
            data.pv1InputVoltage = double.Parse(Helpers.SplitField(inner, 18)) / 10.0;
            data.pv2InputVoltage = double.Parse(Helpers.SplitField(inner, 19)) / 10.0;
            // Flags
            data.configState = int.Parse(Helpers.SplitField(inner, 20));
            data.mppt1ChargerStatus = int.Parse(Helpers.SplitField(inner, 21));
            data.mppt2ChargerStatus = int.Parse(Helpers.SplitField(inner, 22));
            data.loadConnected = int.Parse(Helpers.SplitField(inner, 23));
            data.batteryPowerDirection = int.Parse(Helpers.SplitField(inner, 24));
            data.dcAcDirection = int.Parse(Helpers.SplitField(inner, 25));
            data.gridPowerDirection = int.Parse(Helpers.SplitField(inner, 26));
            data.localParallelID = int.Parse(Helpers.SplitField(inner, 27));

            // Battery charging power
            if (data.batteryPowerDirection == 0 && InverterData.InverterSerialDataObject.PIRI != null 
                && InverterData.InverterSerialDataObject.PIRI.valid)
                data.batteryChargePower = InverterData.InverterSerialDataObject.PIRI.battBulkChargingVoltage * data.batteryChargeAmps;
            else
                data.batteryChargePower = 0.0;
            // Battery discharging power
            if (data.batteryPowerDirection == 1)
                data.batteryDischargePower = data.batteryVoltage * data.batteryDischargeAmps;
            else
                data.batteryDischargePower = 0.0;
            // Grid available
            if (data.gridVoltage >= 100.0)
            {
                data.gridAvailable = true;
                // PV available
                if (data.pv1InputWatts > 0)
                {
                    data.estimatedGridPower = data.outputWatts + data.batteryChargePower - data.pv1InputWatts;
                }
                // PV unavailable
                else
                {
                    data.estimatedGridPower = data.outputWatts + data.batteryChargePower;
                }
                // Grid import cannot be negative
                if (data.estimatedGridPower < 0.0)
                {
                    data.estimatedGridPower = 0.0;
                }
            }
            else
            {
                data.gridAvailable = false;
                data.estimatedGridPower = 0.0;
            }
            data.valid = true;
            data.lastCmdUpdate = Helpers.ElapsedTime;
            return data;
        }

        // UPDATE ENERGY COUNTERS
        public static void UpdateEnergyCounters()
        {
            var now = Helpers.ElapsedTime;
            // First run
            if (InverterData.InverterSerialDataObject.GS == null) return;
            if (InverterData.InverterSerialDataObject.GS.lastCmdUpdate == 0)
            {
                InverterData.InverterSerialDataObject.GS.lastCmdUpdate = now;
                return;
            }
            var elapsed = now - InverterData.InverterSerialDataObject.GS.lastCmdUpdate;
            // Ignore abnormal gaps
            if (elapsed > 10000)
            {
                InverterData.InverterSerialDataObject.GS.lastCmdUpdate = now;
                return;
            }
            InverterData.InverterSerialDataObject.GS.lastCmdUpdate = now;
            
            double hours = elapsed / 3600000.0;
            // BATTERY CHARGING ENERGY
            if (InverterData.InverterSerialDataObject.GS.batteryChargePower > 0.0)
            {
                /*inverter.gsx.totalBatteryChargeKWh +=*/
                InverterData.InverterSerialDataObject.GS.dailyBatteryChargeKWh += (InverterData.InverterSerialDataObject.GS.batteryChargePower * hours) / 1000.0;
            }
            // BATTERY DISCHARGING ENERGY
            if (InverterData.InverterSerialDataObject.GS.batteryDischargePower > 0.0)
            {
                /*inverter.gsx.totalBatteryDischargeKWh +=*/
                InverterData.InverterSerialDataObject.GS.dailyBatteryDischargeKWh += (InverterData.InverterSerialDataObject.GS.batteryDischargePower * hours) / 1000.0;
            }
            // GRID IMPORT ENERGY
            if (InverterData.InverterSerialDataObject.GS.estimatedGridPower > 0.0)
            {
                /*inverter.gsx.totalEstimatedGridKWh +=*/
                InverterData.InverterSerialDataObject.GS.dailyEstimatedGridKWh += (InverterData.InverterSerialDataObject.GS.estimatedGridPower * hours) / 1000.0;
            }
            // PV GENERATED
            if (InverterData.InverterSerialDataObject.GS.pv1InputWatts > 0.0)
            {
                /*inverter.gsx.totalPVInputKWh +=*/
                InverterData.InverterSerialDataObject.GS.dailyPVInputKWh += (InverterData.InverterSerialDataObject.GS.pv1InputWatts * hours) / 1000.0;
            }
            // OUTPUT ENERGY
            if (InverterData.InverterSerialDataObject.GS.outputWatts > 0.0)
            {
                /*inverter.gsx.totalOutputEnergyKWh +=*/
                InverterData.InverterSerialDataObject.GS.dailyOutputEnergyKWh += (InverterData.InverterSerialDataObject.GS.outputWatts * hours) / 1000.0;
            }
        }

        // TO DO - Add cloud data
        public static void CheckDailyEnergyReset()
        {
            if ((Helpers.ElapsedTime - Helpers.DailyEnergyResetLastCheck) < Helpers.DailyEnergyResetCheckInterval)
                return;   // check once per minute
            Helpers.DailyEnergyResetLastCheck = Helpers.ElapsedTime;

            if (Helpers.CurrentTime.Year < Helpers.CurrentYear) return; // time not updated yet
            
            if (Helpers.LastEnergyResetDayOfYear == -1)
            {
                // first valid read after boot
                Helpers.LastEnergyResetDayOfYear = Helpers.CurrentTime.DayOfYear;
                return;
            }

            if (Helpers.CurrentTime.DayOfYear != Helpers.LastEnergyResetDayOfYear)
            {
                Helpers.Log(string.Format("[ENERGY] New day {0}-{1}-{2} - resetting daily counters\n", Helpers.CurrentTime.Year, Helpers.CurrentTime.Month, Helpers.CurrentTime.Day));
                //if (!InverterData.InverterSerialDataObject.cloud.cloudUploadPending)
                {
                    InverterData.InverterSerialDataObject.GS.dailyBatteryChargeKWh = 0;   // your daily counters
                    InverterData.InverterSerialDataObject.GS.dailyBatteryDischargeKWh = 0;
                    InverterData.InverterSerialDataObject.GS.dailyEstimatedGridKWh = 0;
                    InverterData.InverterSerialDataObject.GS.dailyPVInputKWh = 0;
                    InverterData.InverterSerialDataObject.GS.dailyOutputEnergyKWh = 0;

                    Helpers.LastEnergyResetDayOfYear = Helpers.CurrentTime.DayOfYear;
                }
            }
        }

        // ─── MOD Parser — Operating Mode ────────────────────────────────
        public static MODData ParseMODCommand(string response)
        {
            MODData d = new()
            {
                valid = false,
                rawResponse = response
            };
            if (Helpers.ErrorInResponse(response))
                return d;
            string inner = Helpers.StripResponse(response);
            // Output mode
            string f0 = Helpers.SplitField(inner, 0);
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
            else
                d.outputMode = "Hybrid / Grid Tie";

            d.valid = true;
            d.lastCmdUpdate = Helpers.ElapsedTime;
            return d;
        }

        // ─── FLAG Parser — Device Flags ──────────────────────────────────
        public static FLAGData ParseFLAGCommand(string response)
        {
            FLAGData d = new()
            {
                valid = false,
                rawResponse = response
            };
            if (Helpers.ErrorInResponse(response))
                return d;
            string inner = Helpers.StripResponse(response);
            // Each field is 0 or 1
            d.buzzerEnabled = Helpers.SplitField(inner, 0) == "1";
            d.overloadBypass = Helpers.SplitField(inner, 1) == "1";
            d.lcdTimeout = Helpers.SplitField(inner, 2) == "1";
            d.overloadRestart = Helpers.SplitField(inner, 3) == "1";
            d.overTempRestart = Helpers.SplitField(inner, 4) == "1";
            d.backlightOn = Helpers.SplitField(inner, 5) == "1";
            d.alarmPrimaryInput = Helpers.SplitField(inner, 6) == "1";
            d.faultCodeRecord = Helpers.SplitField(inner, 7) == "1";
            d.valid = true;
            d.lastCmdUpdate = Helpers.ElapsedTime;
            return d;
        }

        // ─── PIRI Parser — Rated Parameters ──────────────────────────────
        public static PIRIData ParsePIRICommand(string response)
        {
            PIRIData d = new()
            {
                valid = false,
                rawResponse = response
            };
            if (Helpers.ErrorInResponse(response))
                return d;
            string inner = Helpers.StripResponse(response);
            d.gridVoltageRating             = double.Parse(Helpers.SplitField(inner, 0)) / 10.0;
            d.gridCurrentRating             = double.Parse(Helpers.SplitField(inner, 1));
            d.outputVoltageRating           = double.Parse(Helpers.SplitField(inner, 2)) / 10.0;
            d.outputFreqRating              = double.Parse(Helpers.SplitField(inner, 3)) / 10.0;
            d.outputCurrentRating           = double.Parse(Helpers.SplitField(inner, 4));
            d.outputVARating                = int.Parse(Helpers.SplitField(inner, 5));
            d.outputWattRating              = int.Parse(Helpers.SplitField(inner, 6));
            d.battVoltageRating             = double.Parse(Helpers.SplitField(inner, 7)) / 10.0;
            d.battStopDischargeVoltageOnGrid = double.Parse(Helpers.SplitField(inner, 8)) / 10.0;
            d.battStopChargeVoltageOnGrid   = double.Parse(Helpers.SplitField(inner, 9)) / 10.0;
            d.battCutoffVoltage             = double.Parse(Helpers.SplitField(inner, 10)) / 10.0;
            d.battBulkChargingVoltage       = double.Parse(Helpers.SplitField(inner, 11)) / 10.0;
            d.battFloatVoltage              = double.Parse(Helpers.SplitField(inner, 12)) / 10.0;

            string bt = Helpers.SplitField(inner, 13);
            if (bt == "0")
                d.batteryConfigType = "AGM";
            else if (bt == "1")
                d.batteryConfigType = "Flooded";
            else if (bt == "2")
                d.batteryConfigType = "User-defined";
            else
                d.batteryConfigType = bt;

            d.maxACChargeCurrent = int.Parse(Helpers.SplitField(inner, 14));
            d.maxTotalChargeCurrent = int.Parse(Helpers.SplitField(inner, 15));

            string ivr = Helpers.SplitField(inner, 16);
            if (ivr == "0")
                d.outputSourcePriority = "Appliance";
            else if (ivr == "1")
                d.outputSourcePriority = "UPS";
            else
                d.outputSourcePriority = ivr;

            string osp = Helpers.SplitField(inner, 17);
            if (osp == "0")
                d.outputSourcePriority = "Solar-Utility-Battery";
            else if (osp == "1")
                d.outputSourcePriority = "Solar-Battery-Utility";
            else
                d.outputSourcePriority = osp;

            string csp = Helpers.SplitField(inner, 18);
            if (csp == "0")
                d.chargerSourcePriority = "Solar-first";
            else if (csp == "1")
                d.chargerSourcePriority = "Solar + Utility";
            else if (csp == "2")
                d.chargerSourcePriority = "Only Solar";
            else
                d.chargerSourcePriority = csp;

            string mt = Helpers.SplitField(inner, 20);
            if (mt == "00")
                d.parallelInverterType = "Grid tie";
            else if (mt == "01")
                d.parallelInverterType = "Off Grid";
            else if (mt == "10")
                d.parallelInverterType = "Hybrid";
            else
                d.parallelInverterType = mt;

            d.valid = true;
            d.lastCmdUpdate = Helpers.ElapsedTime;
            return d;
        }

        // ─── ACCT Parser — AC Charge Time ────────────────────────────────
        public static ACCTData ParseACCTCommand(string response)
        {
            ACCTData d = new()
            {
                valid = false,
                rawResponse = response
            };
            if (Helpers.ErrorInResponse(response))
                return d;
            string inner = Helpers.StripResponse(response);
            string raw0 = Helpers.SplitField(inner, 0);
            string raw1 = Helpers.SplitField(inner, 1);
            // Format raw time "1600" → "16:00"
            d.startTime = raw0.Substring(0, 2) + ":" + raw0.Substring(2);
            d.endTime = raw1.Substring(0, 2) + ":" + raw1.Substring(2);
            d.valid = true;
            d.lastCmdUpdate = Helpers.ElapsedTime;
            return d;
        }

        // ─── ACLT Parser — AC Load Time ──────────────────────────────────
        public static ACLTData ParseACLTCommand(string response)
        {
            ACLTData d = new()
            {
                valid = false,
                rawResponse = response
            };
            if (Helpers.ErrorInResponse(response))
                return d;
            string inner = Helpers.StripResponse(response);
            string raw0 = Helpers.SplitField(inner, 0);
            string raw1 = Helpers.SplitField(inner, 1);
            d.startTime = raw0.Substring(0, 2) + ":" + raw0.Substring(2);
            d.endTime = raw1.Substring(0, 2) + ":" + raw1.Substring(2);
            d.valid = true;
            d.lastCmdUpdate = Helpers.ElapsedTime;
            return d;
        }

        // ─── FWV Parser — Firmware Version ──────────────────────────────────
        public static FWVData ParseFWVCommand(string response)
        {
            FWVData d = new()
            {
                valid = false,
                rawResponse = response
            };
            if (Helpers.ErrorInResponse(response))
                return d;
            string inner = Helpers.StripResponse(response);
            d.valid = true;
            d.lastCmdUpdate = Helpers.ElapsedTime;
            return d;
        }

        // ─── DID Parser — Device Identifier ──────────────────────────────────
        public static DIDData ParseDIDCommand(string response)
        {
            DIDData d = new()
            {
                valid = false,
                rawResponse = response
            };
            if (Helpers.ErrorInResponse(response))
                return d;
            string inner = Helpers.StripResponse(response);
            d.valid = true;
            d.lastCmdUpdate = Helpers.ElapsedTime;
            return d;
        }

        // ─── PID Parser — Protocol Identifier ──────────────────────────────────
        public static PIDData ParsePIDCommand(string response)
        {
            PIDData d = new()
            {
                valid = false,
                rawResponse = response
            };
            if (Helpers.ErrorInResponse(response))
                return d;
            String inner = Helpers.StripResponse(response);
            d.valid = true;
            d.lastCmdUpdate = Helpers.ElapsedTime;
            return d;
        }

    }
}
