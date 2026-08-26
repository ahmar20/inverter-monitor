using CCSWE.nanoFramework.NeoPixel;
using CCSWE.nanoFramework.NeoPixel.Drivers;
using System;
using System.IO.Ports;
using System.Text;
using System.Drawing;
using System.Diagnostics;
using nanoFramework.Networking;
using System.Device.Wifi;
using System.Threading;
using System.Reflection;
using System.Net.NetworkInformation;

namespace inverter_monitor
{
    internal class Helpers
    {
        /// <summary>
        /// Inverter Serial Protocols
        /// </summary>
        public static SerialPort InverterSerialPortObject;
        public const string inverterSerialPortName = "COM2";
        public const int InverterSerialBaudRate = 2400;
        public const int InverterSerialTXPin = 16;
        public const int InverterSerialRXPin = 17;

        /// <summary>
        /// Wifi Connection Protocols
        /// </summary>
        private const string WiFiSsid1 = "244 A";
        private const string WiFiPass1 = "Fast244A";
        private const string WiFiSsid2 = "F7490";
        private const string WiFiPass2 = "F74907490";
        public static int ManageWiFiInterval = 5000;
        public static DateTime WiFiConnectionCheckTime;
        private static int LastWifiConnected = -1;
        private static WifiAvailableNetwork CurrentLocalNode = null;
        private static bool IsWifiScanning = false;
        private static bool IsWifiConnecting = false;
        private static string NameOfCurrentWifi
        {
            get
            {
                if (CurrentLocalNode == null)
                    return WiFiSsid1;
                else
                    return CurrentLocalNode.Ssid == WiFiSsid1 ? WiFiSsid1 : WiFiSsid2;
            }
        }
        private static string PassOfCurrentWifi
        {
            get
            {
                if (CurrentLocalNode == null)
                    return WiFiPass1;
                else
                    return CurrentLocalNode.Ssid == WiFiSsid1 ? WiFiPass1 : WiFiPass2;
            }
        }

        //private static bool IsWiFiConnected => WifiNetworkHelper.Status == NetworkHelperStatus.NetworkIsReady;
        private static bool IsWiFiConnected
        {
            get
            {
                var interfaces = NetworkInterface.GetAllNetworkInterfaces();
                if (interfaces == null || interfaces.Length == 0)
                    return false;

                var ni = interfaces[0]; // Get primary network interface
                if (ni.IPv4Address == System.Net.IPAddress.Any.ToString() || string.IsNullOrEmpty(ni.IPv4Address))
                    return false;
                // Valid IP is active, meaning the hardware link is maintained
                return true;
            }
        }

        public static void ManageWifiConnection()
        {
            if (IsWiFiConnected)
            {
                if (IsWifiConnecting)
                {
                    IsWifiConnecting = false;
                    IsWifiScanning = false;
                    // TODO for IP
                    if (CurrentLocalNode.Ssid == WiFiSsid1) LastWifiConnected = 0; else LastWifiConnected = 1;
                    Log(string.Format("[WiFi] Network connected.\n [WiFi] SSID: {0} - RSSI: {1} dBm", CurrentLocalNode.Ssid, CurrentLocalNode.NetworkRssiInDecibelMilliwatts));
                }
                return;
            }
            if (!IsWiFiConnected && !IsWifiScanning && !IsWifiConnecting && LastWifiConnected != -1)
            {
                // Network dropped while running. Reset node to force a fresh scan.
                Log("[WiFi] WiFi got disconnected.");
                if (NameOfCurrentWifi == WiFiSsid1) LastWifiConnected = 0; else LastWifiConnected = 1;
                WifiNetworkHelper.Disconnect();
                WifiNetworkHelper.Reset();
                CurrentLocalNode = null;
            }
            if (IsWifiScanning || IsWifiConnecting)
                return;
            IsWifiScanning = true;
            SetupWiFiAdapter();
            Log("[WiFi] Starting WiFi Scan...");
            WifiNetworkHelper.WifiAdapter.ScanAsync();
        }

        public static void SetupWiFiAdapter()
        {
            Log("Setting up WiFi connection...", false);
            WifiNetworkHelper.SetupNetworkHelper();
            WifiNetworkHelper.WifiAdapter.AvailableNetworksChanged += WifiAdapter_AvailableNetworksChanged;
            Log("Done.");
        }

        public static void WifiAdapter_AvailableNetworksChanged(WifiAdapter sender, object e)
        {
            Log("[WiFi] WiFi Scan completed.");
            var networks = sender.NetworkReport.AvailableNetworks;
            if (networks != null && networks.Length > 0)
            {
                Log($"[WiFi] Found {networks.Length} networks.");
                sbyte highestRssi = sbyte.MinValue;
                for(int i = 0; i < networks.Length; i++)
                {
                    WifiAvailableNetwork current = networks[i];
                    if ((current.Ssid == WiFiSsid1 && LastWifiConnected != 0) || (current.Ssid == WiFiSsid2 && LastWifiConnected != 1))
                    {
                        Log($"[WiFi] Found local match: {current.Ssid} | Signal: {current.NetworkRssiInDecibelMilliwatts} dBm");
                        if (current.NetworkRssiInDecibelMilliwatts > highestRssi)
                        {
                            highestRssi = (sbyte)current.NetworkRssiInDecibelMilliwatts;
                            CurrentLocalNode = current;
                        }
                    }
                }
                if (CurrentLocalNode is null)
                {
                    IsWifiScanning = false;
                    Log("[WiFi] No saved network found.");
                    return;
                }
                IsWifiScanning = false;
                Log($"[WiFi] Connecting to WiFi SSID: {CurrentLocalNode.Ssid}...");
                sender.Connect(CurrentLocalNode, WifiReconnectionKind.Manual, PassOfCurrentWifi);
                IsWifiConnecting = true;
            }
        }



        /// <summary>
        /// NeoPixel RGB LED Protocols
        /// </summary>
        public const byte NeoPixelLedPin = 48;
        public const int NumOfNeoPixelLEDs = 1;
        private const float LedBrightness = 10;
        public static NeoPixelStrip NeoPixelLedObject;
        public static Ws2812B NeoPixelLedDriver => new() { };
        public enum LED_State
        {
            None = 0,
            Query = 1,
            Response = 2,
            Fault = 3,
            Normal = 4
        }

        private static void TurnOnNeoPixelLed(Color color)
        {
            if (NeoPixelLedObject == null)
            {
                Log("[ERROR] NeoPixel Led not initialized.");
                return;
            }
            float brightnessLevel = LedBrightness / 255;
            int red = (int)(color.R * brightnessLevel);
            int green = (int)(color.G * brightnessLevel);
            int blue = (int)(color.B * brightnessLevel);
            Log(string.Format($"[LED] Color Value: R: {red}, G: {green}, B: {blue}"));
            NeoPixelLedObject.Fill(Color.FromArgb(r:red, g:green, b:blue));
            NeoPixelLedObject.Update();
        }

        private static void TurnOffNeoPixelLed()
        {
            if (NeoPixelLedObject == null)
            {
                Log("[ERROR] NeoPixel Led not initialized.");
                return;
            }
            NeoPixelLedObject.Clear();
            NeoPixelLedObject.Update();
        }

        public static void SetLedState(LED_State state)
        {
            switch (state)
            {
                case LED_State.Query:
                    TurnOnNeoPixelLed(Color.Yellow);
                    break;
                case LED_State.Response:
                    TurnOnNeoPixelLed(Color.Green);
                    break;
                case LED_State.Fault:
                    TurnOnNeoPixelLed(Color.Red);
                    break;
                case LED_State.Normal:
                    TurnOnNeoPixelLed(Color.Blue);
                    break;

                default:
                    TurnOffNeoPixelLed();
                    break;
            }
        }


        /// <summary>
        /// Log to Serial / Debug at baudrate of 921600
        /// </summary>
        /// <param name="message"></param>
        /// <param name="writeCompleteLine"></param>
        public static void Log(string message, bool writeCompleteLine = true)
        {
            if (writeCompleteLine)
            {
                //Debug.WriteLine(message);
                Console.WriteLine(message);
            }
            else
            {
                //Debug.Write(message);
                Console.Write(message);
            }
            
        }
    }
}
