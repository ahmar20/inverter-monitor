using CCSWE.nanoFramework.NeoPixel;
using nanoFramework.Hardware.Esp32;
using System;
using System.Diagnostics;
using System.IO.Ports;
using System.Threading;


namespace inverter_monitor
{
    public class Program
    {
        public static void Main()
        {
            Helpers.Log("============================================================");
            Helpers.Log("\tStarting up Inverter Monitor for InfiniSolar VII 5KW");
            Helpers.Log("============================================================");

            Helpers.Log("Setting up RGB LED...", false);
            SetupRGBLed();
            Helpers.Log("Done.");

            Helpers.Log("Setting up Inverter Serial...", false);
            SetupInverterSSerial();
            Helpers.Log("Done.");
            Helpers.SetLedState(Helpers.LED_State.Normal);

            LocalWebServer.Start();

            while (Helpers.IsSystemLoopOn)
            {
                if ((Helpers.ElapsedTime - Helpers.WiFiConnectionLastCheckTime) >= Helpers.ManageWiFiInterval || Helpers.WiFiConnectionLastCheckTime <= 0)
                {
                    Helpers.Log($"[WiFi] Checking WiFi status at {Helpers.ElapsedTime} ms...");
                    Helpers.WiFiConnectionLastCheckTime = Helpers.ElapsedTime;
                    Helpers.ManageWifiConnection();
                }
                // Start Inverter Monitoring here every second unless the command is in process
                if ((Helpers.ElapsedTime - Inverter.InverterSerialLastPollingTime) >= Inverter.InverterPollingInterval)
                {
                    Inverter.InverterSerialLastPollingTime = Helpers.ElapsedTime;
                    GatherInverterData();
                    Inverter.UpdateEnergyCounters();
                }
            }
        }

        private static void SetupInverterSSerial()
        {
            Inverter.InverterSerialPortObject = new SerialPort(Inverter.inverterSerialPortName, Inverter.InverterSerialBaudRate)
            {
                Mode = SerialMode.Normal,
                Handshake = Handshake.None,
                ReadTimeout = 3000,
                WriteTimeout = 3000
            };
            
            Configuration.SetPinFunction(Inverter.InverterSerialRXPin, DeviceFunction.COM2_RX);
            Configuration.SetPinFunction(Inverter.InverterSerialTXPin, DeviceFunction.COM2_TX);
            Inverter.InverterSerialPortObject.Open();
        }

        private static void SetupRGBLed()
        {
            Helpers.NeoPixelLedObject = new NeoPixelStrip(Helpers.NeoPixelLedPin, Helpers.NumOfNeoPixelLEDs, Helpers.NeoPixelLedDriver);
            Helpers.SetLedState(Helpers.LED_State.None);
        }

        private static void GatherInverterData()
        {
            if (Inverter.InverterSerialCommandInProcess)
                return;
            Helpers.Log("[INVERTER] Starting Inverter polling...\t" + Helpers.ElapsedTime);
            Inverter.InverterSerialCommandInProcess = true;
            // PIRI
            var piriResponse = Inverter.StartInverterCommand(Inverter.InverterCommand.CMD_PIRI);
            Helpers.Log($"[INVERTER] PIRI Raw Response: {piriResponse}");
            // GSX
            var gsResponse = Inverter.StartInverterCommand(Inverter.InverterCommand.CMD_GSX);
            Helpers.Log($"[INVERTER] GS Raw Response: {gsResponse}");
            // MOD
            var modResponse = Inverter.StartInverterCommand(Inverter.InverterCommand.CMD_MOD);
            Helpers.Log($"[INVERTER] MOD Raw Response: {modResponse}");
            // FLAG
            var flagResponse = Inverter.StartInverterCommand(Inverter.InverterCommand.CMD_FLAG);
            Helpers.Log($"[INVERTER] FLAG Raw Response: {flagResponse}");
            // ACCT
            var acctResponse = Inverter.StartInverterCommand(Inverter.InverterCommand.CMD_ACCT);
            Helpers.Log($"[INVERTER] ACCT Raw Response: {acctResponse}");
            // ACLT
            var acltResponse = Inverter.StartInverterCommand(Inverter.InverterCommand.CMD_ACLT);
            Helpers.Log($"[INVERTER] ACLT Raw Response: {acltResponse}");

            Helpers.SetLedState(Helpers.LED_State.Normal);
            // Parse All responses to gather data
            Inverter.InverterData.InverterSerialDataObject.PIRI = Inverter.ParsePIRICommand(piriResponse);
            Inverter.GSXData newGsx = Inverter.ParseGSXCommand(gsResponse);
            if (Inverter.InverterData.InverterSerialDataObject.GS != null)
            {
                newGsx.dailyBatteryChargeKWh = Inverter.InverterData.InverterSerialDataObject.GS.dailyBatteryChargeKWh;
                newGsx.dailyBatteryDischargeKWh = Inverter.InverterData.InverterSerialDataObject.GS.dailyBatteryDischargeKWh;
                newGsx.dailyEstimatedGridKWh = Inverter.InverterData.InverterSerialDataObject.GS.dailyEstimatedGridKWh;
                newGsx.dailyPVInputKWh = Inverter.InverterData.InverterSerialDataObject.GS.dailyPVInputKWh;
                newGsx.dailyOutputEnergyKWh = Inverter.InverterData.InverterSerialDataObject.GS.dailyOutputEnergyKWh;
                newGsx.lastCmdUpdate = Inverter.InverterData.InverterSerialDataObject.GS.lastCmdUpdate;
            }
            Inverter.InverterData.InverterSerialDataObject.GS = newGsx;
            Inverter.InverterData.InverterSerialDataObject.MOD = Inverter.ParseMODCommand(modResponse);
            Inverter.InverterData.InverterSerialDataObject.FLAG = Inverter.ParseFLAGCommand(flagResponse);
            Inverter.InverterData.InverterSerialDataObject.ACCT = Inverter.ParseACCTCommand(acctResponse);
            Inverter.InverterData.InverterSerialDataObject.ACLT = Inverter.ParseACLTCommand(acltResponse);
            
            Inverter.InverterSerialCommandInProcess = false;
            Helpers.Log("[INVERTER] Completed Inverter polling...\t" + Helpers.ElapsedTime);
        }
    }
}
