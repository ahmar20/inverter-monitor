#pragma once
// ============================================================================
// led_control.h — the single status NeoPixel: color/flash state machine
// driven from loop() via updateStatusLED(), and day/night brightness.
// ============================================================================

#include <Arduino.h>
#include <FastLED.h>
#include <WiFi.h>
#include "config.h"

CRGB strip[numLEDs];

enum LedState {
  LED_NORMAL,
  LED_QUERY,
  LED_RESPONSE,
  LED_ERROR,
  LED_WIFI
};
unsigned long ledFlashUntil = 0;
bool ledFlashActive = false;
bool ledFlashGreen = false;
unsigned long ledBreathStart = 0;
static LedState currentLedState = LED_NORMAL;

void setLEDState(LedState state);
void updateStatusLED();

int getLEDBrightness() {

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 10)) {
        // If time is not available yet, use the lower brightness
        return 25;
    }
    int hour = timeinfo.tm_hour;
    // Day: 06:00 to 17:59
    if (hour >= 6 && hour < 18) {
        return 75;
    }
    // Night: 18:00 to 05:59
    return 25;
}

void setupRGBLed()
{
  Serial.print("\nSetting up RGB LED...");
  FastLED.addLeds<LED_Type, ledPin>(strip, numLEDs).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(ledBrightness);
  strip[0] = CRGB::Black; // Start with LED off
  FastLED.show();
  Serial.println(" Done.");
}

void setLEDState(LedState state)
{
  currentLedState = state;
  if (state == LED_QUERY)
  {
    // Command sent = AMBER
    ledFlashUntil = millis() + 100;
    strip[0] = CRGB(255, 100, 0);
    FastLED.show();
  }

  else if (state == LED_RESPONSE)
  {
    // Response received = GREEN
    ledFlashUntil = millis() + 100;
    strip[0] = CRGB(0, 255, 0);
    FastLED.show();
  }

  else if (state == LED_ERROR)
  {
    // Permanent RED
    ledFlashUntil = 0;
    strip[0] = CRGB(255, 0, 0);
    FastLED.show();
  }

  else if (state == LED_NORMAL)
  {

    // Normal state; updateStatusLED() will handle blinking
    ledFlashUntil = 0;
  }
  else if (state == LED_WIFI)
  {

    ledFlashUntil = 0;
  }
}

void updateStatusLED()
{
  unsigned long now = millis();
  int ledBrightness = getLEDBrightness();
  // COMMAND SENT - AMBER FLASH
  if (currentLedState == LED_QUERY)
  {
    if (now < ledFlashUntil)
    {
      return;
    }
    // Flash finished
    currentLedState = WiFi.status() == WL_CONNECTED ? LED_NORMAL : LED_WIFI;
  }

  // RESPONSE RECEIVED - GREEN FLASH
  if (currentLedState == LED_RESPONSE)
  {
    if (now < ledFlashUntil)
    {
      return;
    }
    // Flash finished
    currentLedState = WiFi.status() == WL_CONNECTED ? LED_NORMAL : LED_WIFI;
  }

  // ERROR - STEADY RED
  if (currentLedState == LED_ERROR)
  {
    strip[0] = CRGB(255, 0, 0);
    FastLED.setBrightness(ledBrightness);
    FastLED.show();
    return;
  }

  // WIFI DISCONNECTED
  // Yellow blinking
  if (currentLedState == LED_WIFI)
  {
    // remove comment for blinking led
    // bool ledOn = (now / 200) % 2;
    // strip[0] = ledOn ? CRGB(255, 180, 0) : CRGB::Black;
    strip[0] = CRGB(255, 180, 0);
    FastLED.setBrightness(ledBrightness);
    FastLED.show();

    return;
  }

  // NORMAL - FAST BLINKING BLUE
  if (currentLedState == LED_NORMAL)
  {
    // remove comment for blinking led
    // bool ledOn = (now / 200) % 2;
    // if (ledOn) {
    strip[0] = CRGB(0, 0, 255);
    //}
    // else {
    // strip[0] = CRGB::Black;
    //}
    FastLED.setBrightness(ledBrightness);
    FastLED.show();
    return;
  }
}
