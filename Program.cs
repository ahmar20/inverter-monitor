using System;
using System.Diagnostics;
using System.Threading;
using System.IO.Ports;
using System.Device.Gpio;
using System.Drawing;


namespace inverter_monitor
{
    public class Program
    {
        private static SerialPort serialPort;
        private static byte LedPin = 48;

        public static void Main()
        {
            Debug.WriteLine("Hello from nanoFramework!");

            var led = new CCSWE.nanoFramework.NeoPixel.NeoPixelStrip(LedPin, 1, new CCSWE.nanoFramework.NeoPixel.Drivers.Ws2812B());
            led.SetLed(0, Color.FromArgb(10, 10, 10));
            led.Update();

            Thread.Sleep(Timeout.Infinite);
        }

        public static void SetupInverterSSerial()
        {
            int baudRate = 2400;
            int rxPin = 16;
            int txPin = 17;

            serialPort = new SerialPort("COM2", baudRate);
            
        }
    }
}
