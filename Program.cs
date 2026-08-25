using System;
using System.Diagnostics;
using System.Threading;
using System.IO.Ports;
using System.Device.Gpio;
using System.Drawing;
using nanoFramework.Hardware.Esp32;
using CCSWE.nanoFramework.NeoPixel;


namespace inverter_monitor
{
    public class Program
    {
        private static SerialPort inverterSerialPort;
        private const string inverterSerialPortName = "COM2";
        private const int InverterSerialBaudRate = 2400;
        private const int TXPin = 16;
        private const int RXPin = 17;
        private const byte LedPin = 48;
        private const int NumOfLEDs = 1;
        private static NeoPixelStrip LED;

        public static void Main()
        {
            Debug.WriteLine("Hello from nanoFramework!");

            Thread.Sleep(Timeout.Infinite);
        }

        public static void SetupInverterSSerial()
        {
            inverterSerialPort = new SerialPort(inverterSerialPortName, InverterSerialBaudRate);
            Configuration.SetPinFunction(RXPin, DeviceFunction.COM2_RX);
            Configuration.SetPinFunction(TXPin, DeviceFunction.COM2_TX);

        }

        public static void SetupRGBLE()
        {
            LED = new NeoPixelStrip(LedPin, NumOfLEDs, new CCSWE.nanoFramework.NeoPixel.Drivers.Ws2812B());
            //led.SetLed(0, Color.FromArgb(10, 10, 10));
            //led.Update();
        }
    }
}
