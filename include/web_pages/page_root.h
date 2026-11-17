#pragma once
// ============================================================================
// page_root.h — the bare landing page at '/': links to /dashboard and /home.
// Pure HTML/CSS/JS served as-is by web_server.h. No C++ logic here.
// ============================================================================
#include <Arduino.h>

const char root_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8"/>
  <meta name="viewport" content="width=device-width,initial-scale=1"/>
  <title>InfiniSolar VII 5KW</title>
  <style>
    body { margin:0; min-height:100vh; display:flex; flex-direction:column;
           align-items:center; justify-content:center; background:#0b1220;
           font-family:system-ui,sans-serif; }
    h1 { color:#e8eef8; font-size:22px; font-weight:700; margin-bottom:8px; text-align:center; }
    p  { color:#90a2bd; font-size:13px; margin-bottom:36px; text-align:center; }
    .links { display:flex; gap:16px; }
    a { display:inline-flex; align-items:center; gap:8px; padding:13px 28px;
        border-radius:12px; font-size:15px; font-weight:700; text-decoration:none;
        transition:opacity .2s; }
    a:hover { opacity:.82; }
    .a-home { background:#182545; color:#e8eef8; border:1px solid #24334f; }
    .a-dash { background:#f5a623; color:#1a1206; }
  </style>
</head>
<body>
  <h1>InfiniSolar VII 5KW</h1>
  <p>ESP32-S3 Solar Monitor</p>
  <div class="links">
    <a class="a-home" href="./home">
      <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
        <path d="m3 9 9-7 9 7v11a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2z"/>
        <polyline points="9 22 9 12 15 12 15 22"/>
      </svg>
      System Home
    </a>
    <a class="a-dash" href="./dashboard">
      <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
        <path d="M13 2 3 14h7l-1 8 10-12h-7l1-8z"/>
      </svg>
      Dashboard
    </a>
  </div>
</body>
</html>
)rawliteral";

