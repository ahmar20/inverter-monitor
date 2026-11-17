#pragma once
// ============================================================================
// main.h — thin backward-compatible aggregator. Nothing in the new module
// set needs this anymore (each header includes exactly what it uses), but
// it's kept in case anything outside src/main.cpp still does `#include
// <main.h>`. Prefer including the specific module header you need instead.
// ============================================================================

#include "config.h"
#include "types.h"
#include "utils.h"
#include "debug_log.h"
#include "led_control.h"
#include "wifi_manager.h"
#include "inverter_protocol.h"
#include "cloud_queue.h"
#include "cloud_restore.h"
#include "schedules.h"
#include "api_handlers.h"
#include "web_server.h"
