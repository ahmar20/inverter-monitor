#pragma once
// ============================================================================
// debug_log.h — tiny in-RAM ring buffer of recent debug lines, exposed at
// /api/serial via generateSerialDebug() (see debug_report.h).
// ============================================================================

#include <Arduino.h>
#include "config.h"

static char dbgBuf[DBG_MAX_LINES][DBG_LINE_LEN];
static int  dbgHead = 0;   // next line to write
static int  dbgCount = 0;  // total lines written (capped at DBG_MAX_LINES)

void dbgLog(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsnprintf(dbgBuf[dbgHead], DBG_LINE_LEN, fmt, args);
    va_end(args);
    dbgHead = (dbgHead + 1) % DBG_MAX_LINES;
    if (dbgCount < DBG_MAX_LINES) dbgCount++;
}

String dbgDump() {
    String out;
    out.reserve(dbgCount * 60);
    int start = (dbgCount < DBG_MAX_LINES) ? 0 : dbgHead;
    for (int i = 0; i < dbgCount; i++) {
        out += dbgBuf[(start + i) % DBG_MAX_LINES];
        out += "\n";
    }
    return out;
}
