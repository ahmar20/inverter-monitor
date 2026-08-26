using System;
using System.Diagnostics;
using System.Threading;
using System.IO.Ports;
using nanoFramework.Hardware.Esp32;
using CCSWE.nanoFramework.NeoPixel;


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

            while (true)
            {
                if (DateTime.UtcNow >= Helpers.WiFiConnectionCheckTime)
                {
                    Helpers.WiFiConnectionCheckTime = DateTime.UtcNow.AddMilliseconds(Helpers.ManageWiFiInterval);
                    Helpers.ManageWifiConnection();
                }
                
                // Start Inverter Monitoring here

                Thread.Sleep(1000);
            }
        }

        private static void SetupInverterSSerial()
        {
            Helpers.InverterSerialPortObject = new SerialPort(Helpers.inverterSerialPortName, Helpers.InverterSerialBaudRate)
            {
                Mode = SerialMode.Normal,
                Handshake = Handshake.None,
                ReadTimeout = 3000,
                WriteTimeout = 3000
            };
            Configuration.SetPinFunction(Helpers.InverterSerialRXPin, DeviceFunction.COM2_RX);
            Configuration.SetPinFunction(Helpers.InverterSerialTXPin, DeviceFunction.COM2_TX);
            Helpers.InverterSerialPortObject.Open();
        }

        private static void SetupRGBLed()
        {
            Helpers.NeoPixelLedObject = new NeoPixelStrip(Helpers.NeoPixelLedPin, Helpers.NumOfNeoPixelLEDs, Helpers.NeoPixelLedDriver);
            Helpers.SetLedState(Helpers.LED_State.None);
        }
    }
}
