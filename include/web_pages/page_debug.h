#pragma once
// ============================================================================
// page_debug.h — the /serial raw debug viewer.
// Pure HTML/CSS/JS served as-is by web_server.h. No C++ logic here.
// ============================================================================
#include <Arduino.h>

const char debug_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Inverter Debug</title>
    <style>
        html,
        body {
            margin: 0;
            padding: 0;
            background: #111;
            color: #ddd;
            font-family: monospace;
        }
        
        body {
            padding: 12px;
        }
        
        h2 {
            margin: 5px 0 12px 0;
            color: #fff;
        }
        
        #status {
            color: #888;
            margin-bottom: 10px;
        }
        
        #debug {
            white-space: pre;
            overflow-x: auto;
            background: #181818;
            border: 1px solid #333;
            padding: 15px;
            border-radius: 6px;
            line-height: 1.35;
            font-size: 13px;
        }
    </style>
</head>

<body>
    <h2>INFINISOLAR VII 5KW - DEBUG</h2>
    <div id="status">Connecting...</div> <pre id="debug">Loading...</pre>
    <script>
        async function updateDebug() {
            try {
                const response = await fetch('./api/serial', {
                    cache: 'no-store'
                });
                if (!response.ok) throw new Error('HTTP ' + response.status);
                const data = await response.text();
                // Replace the entire contents.
                // Nothing is appended.
                document.getElementById('debug').textContent = data;
                document.getElementById('status').textContent = 'Updated: ' + new Date().toLocaleTimeString();
            } catch (error) {
                document.getElementById('status').textContent = 'ERROR: ' + error;
            }
        }
        // First update immediately
        updateDebug();
        // Refresh every 1 seconds
        setInterval(updateDebug, 1000);
    </script>
</body>
</html>
)rawliteral";

