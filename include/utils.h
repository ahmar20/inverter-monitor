#pragma once
// ============================================================================
// utils.h — small stateless helpers shared across modules (time formatting).
// ============================================================================

#include <Arduino.h>

String getTimeString() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 0)) return "N/A";
  char buf[32];
  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
  return String(buf);
}

String getISOTime()
{
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 0))
        return "";
    char buffer[30];
    strftime(buffer, sizeof(buffer),
             "%Y-%m-%dT%H:%M:%S%z",
             &timeinfo);
    return String(buffer);
}
