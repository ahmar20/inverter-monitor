#pragma once
// ============================================================================
// page_dashboard.h — the live inverter dashboard at /dashboard.
// Pure HTML/CSS/JS served as-is by web_server.h. No C++ logic here.
// ============================================================================
#include <Arduino.h>

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en" data-theme="dark">
  <head>
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0" />
    <meta name="theme-color" content="#0b1220" />
    <link rel="manifest" href="./manifest.json" />
    <meta name="mobile-web-app-capable" content="yes" />
    <meta name="apple-mobile-web-app-capable" content="yes" />
    <meta name="apple-mobile-web-app-status-bar-style" content="black-translucent" />
    <meta name="apple-mobile-web-app-title" content="Inverter" />
    <link rel="apple-touch-icon" href="./icon.svg" />
    <title>Inverter Monitor</title>
    <style>
      /* ============================================================
           THEME TOKENS
           ============================================================ */
      :root {
        --font-ui: system-ui, -apple-system, "Segoe UI", Roboto, sans-serif;
        --font-num: ui-monospace, "SFMono-Regular", "JetBrains Mono", "Menlo", monospace;
        --amber: #f5a623;
        --charge: #34d399;
        --discharge: #fb7185;
        --grid: #38bdf8;
        --radius: 16px;
        --gap: 18px;
        --ease: cubic-bezier(0.22, 1, 0.36, 1);
      }

      html[data-theme="dark"] {
        color-scheme: dark;
        --bg: #0b1220;
        --bg-grad: radial-gradient(1200px 600px at 80% -10%, #16233b 0%, transparent 60%),
          radial-gradient(900px 500px at -10% 110%, #101a2e 0%, transparent 55%);
        --surface: #131d31;
        --surface-2: #182545;
        --surface-rgb: 19, 29, 49;
        --surface-2-rgb: 24, 37, 69;
        --border: #24334f;
        --text: #e8eef8;
        --muted: #90a2bd;
        --shadow: 0 18px 40px -24px rgba(0, 0, 0, 0.8);
        --glass-bg-light: rgba(19, 29, 49, .45);
        --glass-bg: rgba(19, 29, 49, .78);
        --glass-bg-strong: rgba(19, 29, 49, .90);
        --flow-bg: rgba(24, 37, 69, .72);
        --aurora-opacity: .50;
      }

      html[data-theme="light"] {
        color-scheme: light;
        --bg: #eef2f8;
        --bg-grad: radial-gradient(1200px 600px at 80% -10%, #dbe6f5 0%, transparent 60%),
          radial-gradient(900px 500px at -10% 110%, #e2ecf7 0%, transparent 55%);
        --surface: #ffffff;
        --surface-2: #f4f7fc;
        --surface-rgb: 255, 255, 255;
        --surface-2-rgb: 244, 247, 252;
        --border: #d7e0ee;
        --text: #17233a;
        --muted: #5c6b83;
        --shadow: 0 18px 40px -28px rgba(30, 50, 90, 0.4);
        --glass-bg-light: rgba(255, 255, 255, .45);
        --glass-bg: rgba(255, 255, 255, .82);
        --glass-bg-strong: rgba(255, 255, 255, .94);
        --flow-bg: rgba(244, 247, 252, .88);
        --aurora-opacity: .50;
      }

      * {
        box-sizing: border-box;
        margin: 0;
        padding: 0;
      }

      body {
        font-family: var(--font-ui);
        background: var(--bg);
        background-image: var(--bg-grad);
        background-attachment: fixed;
        color: var(--text);
        line-height: 1.5;
        min-height: 100vh;
        -webkit-font-smoothing: antialiased;
        transition: background-color 0.4s var(--ease), color 0.4s var(--ease);
      }

      .wrap {
        width: 100%;
        padding: 22px 18px 64px;
      }

      /* ============================================================
           HEADER
           ============================================================ */
      header {
        display: flex;
        flex-wrap: wrap;
        align-items: center;
        justify-content: space-between;
        gap: 16px;
        margin-bottom: 0;
        /* ── Sticky translucent bar ── */
        position: sticky;
        top: 0;
        z-index: 100;
        background: var(--glass-bg-light);
        backdrop-filter: blur(20px);
        -webkit-backdrop-filter: blur(20px);
        border-bottom: 1px solid var(--border);
        padding: 14px 18px;
        transition: background 0.4s var(--ease), border-color 0.4s var(--ease);
      }

      .brand {
        display: flex;
        align-items: center;
        gap: 14px;
      }

      .logo {
        width: 46px;
        height: 46px;
        border-radius: 13px;
        display: grid;
        place-items: center;
        background: linear-gradient(145deg, var(--amber), #d97706);
        color: #1a1206;
        box-shadow: 0 8px 20px -8px rgba(245, 166, 35, 0.7);
        flex-shrink: 0;
      }

      .logo svg {
        width: 26px;
        height: 26px;
      }

      .brand h1 {
        font-size: 20px;
        font-weight: 700;
        letter-spacing: -0.01em;
      }

      .brand p {
        font-size: 12.5px;
        color: var(--muted);
        font-family: var(--font-num);
      }

      .header-controls {
        display: flex;
        align-items: center;
        gap: 10px;
        flex-wrap: wrap;
      }

      .pill {
        display: inline-flex;
        align-items: center;
        gap: 9px;
        padding: 9px 15px;
        border-radius: 999px;
        background: var(--surface);
        border: 1px solid var(--border);
        font-size: 13px;
        font-weight: 600;
        box-shadow: var(--shadow);
        transition: all 0.3s var(--ease);
      }

      .status-dot {
        width: 9px;
        height: 9px;
        border-radius: 50%;
        background: var(--muted);
        position: relative;
        flex-shrink: 0;
      }

      .status-dot.online {
        background: var(--charge);
        animation: dot-ring 2s ease infinite;
      }

      .status-dot.online::after {
        display: none;
      }

      .status-dot.offline {
        background: var(--discharge);
      }

      @keyframes dot-ring {
        0% {
          box-shadow: 0 0 0 0 rgba(52, 211, 153, .55);
        }

        70% {
          box-shadow: 0 0 0 8px rgba(52, 211, 153, 0);
        }

        100% {
          box-shadow: 0 0 0 0 rgba(52, 211, 153, 0);
        }
      }

      @keyframes flow-r {
        from {
          stroke-dashoffset: 26;
        }

        to {
          stroke-dashoffset: 0;
        }
      }

      .ctrl {
        display: inline-flex;
        align-items: center;
        gap: 8px;
        padding: 8px 12px;
        border-radius: 12px;
        background: var(--surface);
        border: 1px solid var(--border);
        font-size: 13px;
        color: var(--muted);
        box-shadow: var(--shadow);
      }

      select,
      button.tbtn,
      a.tbtn {
        font-family: var(--font-ui);
        font-size: 13px;
        font-weight: 600;
        background: var(--surface-2);
        color: var(--text);
        border: 1px solid var(--border);
        border-radius: 9px;
        padding: 6px 10px;
        cursor: pointer;
        transition: all 0.2s var(--ease);
        text-decoration: none;
      }

      button.tbtn,
      a.tbtn {
        display: inline-flex;
        align-items: center;
        gap: 7px;
      }

      button.tbtn:hover,
      a.tbtn:hover,
      select:hover {
        border-color: var(--amber);
        color: var(--text);
      }

      a.tbtn svg {
        color: var(--muted);
        transition: color 0.2s var(--ease);
      }

      a.tbtn:hover svg {
        color: var(--amber);
      }

      html[data-theme="light"] select,
      html[data-theme="light"] button.tbtn,
      html[data-theme="light"] a.tbtn {
        background: rgba(255, 255, 255, .88);
        color: var(--text);
      }

      html[data-theme="light"] select option {
        background: #fff;
        color: #17233a;
      }

      html[data-theme="dark"] select option {
        background: #182545;
        color: #e8eef8;
      }

      /* ============================================================
           SUMMARY STRIP
           ============================================================ */
      .summary {
        display: grid;
        grid-template-columns: repeat(4, minmax(0, 1fr));
        gap: var(--gap);
        margin-bottom: var(--gap);
      }

      .stat {
        background: var(--glass-bg);
        border: 1px solid var(--border);
        border-radius: var(--radius);
        padding: 18px 20px;
        box-shadow: var(--shadow);
        position: relative;
        overflow: hidden;
        transition: transform 0.3s var(--ease), border-color 0.3s var(--ease);
        min-width: 0;
      }

      .stat::before {
        content: "";
        position: absolute;
        left: 0;
        top: 0;
        bottom: 0;
        width: 3px;
        background: var(--accent, var(--amber));
      }

      .stat:hover {
        transform: translateY(-3px);
      }

      .stat .k {
        font-size: 12px;
        color: var(--muted);
        text-transform: uppercase;
        letter-spacing: 0.07em;
        font-weight: 600;
      }

      .stat .v {
        font-family: var(--font-num);
        font-size: 30px;
        font-weight: 700;
        margin-top: 6px;
        letter-spacing: -0.02em;
      }

      .stat .v small {
        font-size: 15px;
        color: var(--muted);
        font-weight: 500;
        margin-left: 3px;
      }

      .stat .sub {
        font-size: 12.5px;
        color: var(--muted);
        margin-top: 3px;
      }

      /* ============================================================
           CARD GRID
           ============================================================ */
      .grid {
        display: grid;
        grid-template-columns: repeat(12, minmax(0, 1fr));
        gap: var(--gap);
      }

      .grid+.grid {
        margin-top: var(--gap);
      }

      .card {
        background: var(--glass-bg);
        backdrop-filter: blur(16px);
        -webkit-backdrop-filter: blur(16px);
        box-shadow: var(--shadow), inset 0 1px 0 rgba(255, 255, 255, .04);
        border: 1px solid var(--border);
        border-radius: var(--radius);
        padding: 22px;
        transition: border-color 0.3s var(--ease), transform 0.3s var(--ease), box-shadow 0.3s var(--ease);
        animation: rise 0.5s var(--ease) both;
        min-width: 0;
      }

      .card:hover {
        border-color: color-mix(in srgb, var(--accent, var(--amber)) 30%, var(--border));
        box-shadow: var(--shadow),
          0 0 0 1px color-mix(in srgb, var(--accent, var(--amber)) 12%, transparent),
          inset 0 1px 0 rgba(255, 255, 255, .04);
      }

      html[data-theme="light"] .card,
      html[data-theme="light"] .stat,
      html[data-theme="light"] details,
      html[data-theme="light"] .pill,
      html[data-theme="light"] .ctrl {
        box-shadow: 0 16px 38px -25px rgba(30, 50, 90, .30),
          inset 0 1px 0 rgba(255, 255, 255, .85);
      }

      html[data-theme="dark"] .card,
      html[data-theme="dark"] .stat,
      html[data-theme="dark"] details,
      html[data-theme="dark"] .pill,
      html[data-theme="dark"] .ctrl {
        box-shadow: var(--shadow), inset 0 1px 0 rgba(255, 255, 255, .045);
      }

      @keyframes rise {
        from {
          opacity: 0;
          transform: translateY(14px);
        }

        to {
          opacity: 1;
          transform: none;
        }
      }

      .col-4 {
        grid-column: span 4;
      }

      .col-6 {
        grid-column: span 6;
      }

      .col-8 {
        grid-column: span 8;
      }

      .col-12 {
        grid-column: span 12;
      }

      .card-head {
        display: flex;
        align-items: center;
        justify-content: space-between;
        margin-bottom: 18px;
        gap: 10px;
        flex-wrap: wrap;
      }

      .card-title {
        display: flex;
        align-items: center;
        gap: 11px;
        font-size: 16px;
        font-weight: 700;
      }

      .icon {
        width: 34px;
        height: 34px;
        border-radius: 10px;
        display: grid;
        place-items: center;
        background: color-mix(in srgb, var(--accent) 16%, transparent);
        color: var(--accent);
        flex-shrink: 0;
      }

      .icon svg {
        width: 19px;
        height: 19px;
      }

      .badge {
        font-family: var(--font-num);
        font-size: 11.5px;
        font-weight: 700;
        padding: 4px 10px;
        border-radius: 999px;
        letter-spacing: 0.04em;
        background: var(--surface-2);
        color: var(--muted);
        border: 1px solid var(--border);
        transition: all 0.3s var(--ease);
        white-space: nowrap;
      }

      .badge.good {
        background: color-mix(in srgb, var(--charge) 18%, transparent);
        color: var(--charge);
        border-color: transparent;
      }

      .badge.bad {
        background: color-mix(in srgb, var(--discharge) 18%, transparent);
        color: var(--discharge);
        border-color: transparent;
      }

      /* ============================================================
           METRIC ROWS
           ============================================================ */
      .metrics {
        display: grid;
        grid-template-columns: repeat(2, minmax(0, 1fr));
        gap: 14px;
      }

      .metrics-4 {
        grid-template-columns: repeat(4, minmax(0, 1fr));
      }

      .metric {
        background: var(--surface-2);
        border-radius: 12px;
        padding: 13px 15px;
        min-width: 0;
        overflow: hidden;
      }

      .metric-wide {
        grid-column: span 2;
      }

      .metric .lbl {
        font-size: 11.5px;
        color: var(--muted);
        text-transform: uppercase;
        letter-spacing: 0.05em;
        font-weight: 600;
        white-space: nowrap;
        overflow: hidden;
        text-overflow: ellipsis;
      }

      .metric .num {
        font-family: var(--font-num);
        font-size: 23px;
        font-weight: 700;
        margin-top: 4px;
        letter-spacing: -0.02em;
        white-space: nowrap;
        overflow: hidden;
        text-overflow: ellipsis;
      }

      .metric .num small {
        font-size: 13px;
        color: var(--muted);
        font-weight: 500;
        margin-left: 2px;
      }

      /* ============================================================
           GAUGES
           ============================================================ */
      .gauge-wrap {
        display: flex;
        align-items: center;
        flex-wrap: wrap;
        gap: 22px;
        justify-content: center;
      }

      .gauge {
        position: relative;
        width: 132px;
        height: 132px;
        flex: 0 0 132px;
      }

      .gauge svg {
        display: block;
        width: 100%;
        height: 100%;
        transform: rotate(-90deg);
      }

      .gauge-new svg {
        transform: none !important;
      }

      .gauge .track {
        fill: none;
        stroke: var(--border);
        stroke-width: 12;
      }

      .gauge .prog {
        fill: none;
        stroke: var(--accent);
        stroke-width: 12;
        stroke-linecap: round;
        transition: stroke-dashoffset 0.7s var(--ease), stroke 0.4s var(--ease);
      }

      .gauge .center {
        position: absolute;
        inset: 0;
        display: flex;
        flex-direction: column;
        align-items: center;
        justify-content: center;
        text-align: center;
        line-height: 1;
        pointer-events: none;
      }

      .gauge .center .g-num {
        font-family: var(--font-num);
        font-size: 28px;
        font-weight: 700;
        line-height: 1;
        white-space: nowrap;
      }

      .gauge .center .g-lbl {
        margin-top: 6px;
        font-size: 10px;
        line-height: 1;
        color: var(--muted);
        white-space: nowrap;
      }

      .gauge-side {
        flex: 1 1 180px;
        min-width: 0;
        display: grid;
        grid-template-columns: repeat(2, minmax(0, 1fr));
        gap: 12px;
      }

      .metrics-row {
        display: grid;
        grid-template-columns: repeat(2, minmax(0, 1fr));
        gap: 12px;
        margin-top: 16px;
      }

      /* ============================================================
           PROGRESS BAR
           ============================================================ */
      .bar {
        height: 10px;
        border-radius: 999px;
        background: var(--border);
        overflow: hidden;
      }

      .bar>span {
        display: block;
        height: 100%;
        border-radius: 999px;
        background: linear-gradient(90deg, var(--accent), color-mix(in srgb, var(--accent) 55%, #fff));
        width: 0%;
        transition: width 0.7s var(--ease);
      }

      /* ============================================================
           ENERGY FLOW
           ============================================================ */
      .flow {
        display: grid;
        grid-template-columns: 1fr auto 1fr auto 1fr;
        align-items: center;
        gap: 10px;
        background: var(--flow-bg);
        border-radius: 12px;
        border: 1px solid var(--border);
        padding: 20px 24px;
        margin-top: 4px;
        min-height: 178px;
        position: relative;
        overflow: hidden;
      }

      .flow::before {
        content: "";
        position: absolute;
        inset: 0;
        background:
          radial-gradient(circle at 20% 50%, color-mix(in srgb, var(--amber) 9%, transparent), transparent 30%),
          radial-gradient(circle at 50% 50%, color-mix(in srgb, var(--charge) 8%, transparent), transparent 30%),
          radial-gradient(circle at 80% 50%, color-mix(in srgb, var(--grid) 9%, transparent), transparent 30%);
        pointer-events: none;
      }

      .flow>* {
        position: relative;
        z-index: 1;
      }

      .flow-line-active {
        stroke-dasharray: 8 5;
        animation: flowAnim 0.7s linear infinite;
      }
      .flow-line-idle {
        opacity: 0.12;
      }
      @keyframes flowAnim {
        to { stroke-dashoffset: -13; }
      }
      .fnode-active { opacity: 1; transition: opacity .4s; }
      .fnode-idle   { opacity: .25; transition: opacity .4s; }
      .fnode-active circle.outer { filter: url(#fng); }

      .flow-node-slot {
        position: relative;
        min-width: 0;
      }

      .flow-node {
        text-align: center;
        min-width: 0;
        transition: opacity 0.35s var(--ease), transform 0.35s var(--ease);
      }

      .flow-node.hidden {
        display: none;
      }

      .flow-node .fn-ico {
        width: 54px;
        height: 54px;
        border-radius: 15px;
        margin: 0 auto 9px;
        display: grid;
        place-items: center;
        background: color-mix(in srgb, var(--fc, var(--amber)) 18%, transparent);
        color: var(--fc, var(--amber));
        transition: box-shadow 0.4s var(--ease);
        box-shadow: 0 0 24px -7px color-mix(in srgb, var(--fc, var(--amber)) 75%, transparent);
      }

      .flow-node .fn-ico svg {
        width: 28px;
        height: 28px;
      }

      .flow-node .fn-val {
        font-family: var(--font-num);
        font-weight: 700;
        font-size: clamp(22px, 2.2vw, 30px);
        line-height: 1.1;
        letter-spacing: -.025em;
        white-space: nowrap;
        overflow: hidden;
        text-overflow: ellipsis;
      }

      .flow-node .fn-lbl {
        font-size: 14px;
        color: var(--muted);
        margin-top: 5px;
        font-weight: 700;
        letter-spacing: .03em;
        text-transform: uppercase;
      }

      .flow-node.source-active .fn-ico {
        animation: node-pulse-enhanced 2s var(--ease) infinite;
      }

      @keyframes node-pulse-enhanced {

        0%,
        100% {
          box-shadow: 0 0 0 0 color-mix(in srgb, var(--fc, var(--amber)) 48%, transparent),
            0 0 22px -7px color-mix(in srgb, var(--fc, var(--amber)) 70%, transparent);
        }

        70% {
          box-shadow: 0 0 0 12px color-mix(in srgb, var(--fc, var(--amber)) 0%, transparent),
            0 0 28px -4px color-mix(in srgb, var(--fc, var(--amber)) 80%, transparent);
        }
      }

      .flow-arrow {
        color: var(--muted);
        display: grid;
        place-items: center;
      }

      .flow-arrow svg {
        width: 38px;
        height: 18px;
      }

      .flow-arrow.active {
        color: var(--amber);
        filter: drop-shadow(0 0 5px currentColor);
      }

      .flow-arrow.active svg {
        animation: slide 1.2s linear infinite;
      }

      @keyframes slide {
        0% {
          opacity: 0.3;
        }

        50% {
          opacity: 1;
        }

        100% {
          opacity: 0.3;
        }
      }

      /* ============================================================
           COLLAPSIBLE
           ============================================================ */
      details {
        background: var(--glass-bg);
        border: 1px solid var(--border);
        border-radius: var(--radius);
        box-shadow: var(--shadow);
        overflow: hidden;
      }

      details summary {
        list-style: none;
        cursor: pointer;
        padding: 18px 22px;
        display: flex;
        align-items: center;
        justify-content: space-between;
        font-weight: 700;
        font-size: 15px;
        gap: 10px;
      }

      details summary::-webkit-details-marker {
        display: none;
      }

      .chev {
        transition: transform 0.3s var(--ease);
        color: var(--muted);
        flex-shrink: 0;
      }

      details[open] .chev {
        transform: rotate(180deg);
      }

      .collapsible-content {
        padding: 0 22px 22px;
      }

      /* ============================================================
           SETTINGS / VALUES
           ============================================================ */
      .value-grid {
        display: grid;
        grid-template-columns: repeat(auto-fit, minmax(150px, 1fr));
        gap: 14px;
      }

      .value-box {
        background: var(--surface-2);
        border-radius: 12px;
        padding: 14px 16px;
        min-width: 0;
        overflow: hidden;
      }

      .value-box .value-label {
        font-size: 11.5px;
        color: var(--muted);
        text-transform: uppercase;
        letter-spacing: 0.05em;
        font-weight: 600;
      }

      .value-box .value {
        font-family: var(--font-num);
        font-weight: 700;
        font-size: 21px;
        margin-top: 5px;
        white-space: nowrap;
        overflow: hidden;
        text-overflow: ellipsis;
      }

      /* ============================================================
           FLAGS
           ============================================================ */
      .flag {
        display: inline-flex;
        align-items: center;
        gap: 7px;
        padding: 8px 13px;
        border-radius: 10px;
        font-size: 13px;
        font-weight: 600;
        border: 1px solid var(--border);
      }

      .flag .fdot {
        width: 7px;
        height: 7px;
        border-radius: 50%;
      }

      /* ============================================================
           FOOTER
           ============================================================ */
      .footer {
        text-align: center;
        margin-top: 34px;
        color: var(--muted);
        font-size: 12.5px;
        font-family: var(--font-num);
      }

      /* ============================================================
           OFFLINE BANNER
           ============================================================ */
      #offlineBanner {
        display: none;
        align-items: center;
        gap: 12px;
        background: color-mix(in srgb, var(--discharge) 12%, var(--surface));
        border: 1px solid color-mix(in srgb, var(--discharge) 45%, transparent);
        color: var(--text);
        border-radius: var(--radius);
        padding: 14px 20px;
        margin-bottom: var(--gap);
        box-shadow: var(--shadow);
        animation: rise 0.4s var(--ease);
      }

      #offlineBanner.show {
        display: flex;
      }

      #offlineBanner .ob-ico {
        color: var(--discharge);
        flex-shrink: 0;
        display: grid;
        place-items: center;
      }

      #offlineBanner .ob-ico svg {
        width: 22px;
        height: 22px;
      }

      #offlineBanner strong {
        display: block;
        font-size: 14px;
      }

      #offlineBanner span {
        font-size: 12.5px;
        color: var(--muted);
      }

      .stale {
        opacity: 0.5;
        transition: opacity 0.3s var(--ease);
      }

      /* ============================================================
           ANIMATED BACKGROUND
           ============================================================ */
      
      body {
        background-color: var(--bg);
        position: relative;
      }

      body::before {
        content: "";
        position: fixed;
        inset: 0;
        z-index: -3;
        pointer-events: none;
        background-image:
          radial-gradient(ellipse 80vw 80vw at 8%  12%, rgba(255,182,72,.60),  transparent 70%),
          radial-gradient(ellipse 80vw 80vw at 92% 10%, rgba(157,123,255,.55), transparent 70%),
          radial-gradient(ellipse 80vw 80vw at 15% 88%, rgba(62,224,143,.50),  transparent 70%),
          radial-gradient(ellipse 80vw 80vw at 62% 50%, rgba(79,195,255,.38),  transparent 70%);
      }
      html[data-theme="light"] body::before {
        background-image:
          radial-gradient(ellipse 80vw 80vw at 8%  12%, rgba(255,182,72,.28),  transparent 70%),
          radial-gradient(ellipse 80vw 80vw at 92% 10%, rgba(157,123,255,.22), transparent 70%),
          radial-gradient(ellipse 80vw 80vw at 15% 88%, rgba(62,224,143,.20),  transparent 70%),
          radial-gradient(ellipse 80vw 80vw at 62% 50%, rgba(79,195,255,.16),  transparent 70%);
      }

        /* ============================================================
           XL SPLIT: FLOW (left) + SUMMARY STRIP (right)
           On ≥1280 px the flow diagram and the 4 summary stat boxes
           sit side-by-side. Below that they stack as before.
           ============================================================ */
        .xl-split {
          display: flex;
          flex-direction: column;
          gap: var(--gap);
          margin-bottom: var(--gap);
        }
        .xl-split .summary {
          margin-bottom: 0;
        }

        /* ============================================================
           RESPONSIVE LAYOUT
           ============================================================ */
        @media (min-width: 1101px) {
          .wrap {
            padding-left: 20px; padding-right: 20px;
          }

          .summary, .grid {
            gap: 18px; grid-auto-flow: dense;
          }

          .col-xl-8 {
            grid-column: span 8;
          }

          .col-xl-4 {
            grid-column: span 4;
          }
          
          .xl-split {
            display: grid;
            grid-template-columns: 7fr 5fr;
            align-items: start;
          }
          /* reflow 4 stats into 2 × 2 grid */
          .xl-split .summary {
            grid-template-columns: repeat(2, minmax(0, 1fr));
          }
          /* give stat boxes a bit more breathing room in 2-col layout */
          .xl-split .summary .stat {
            min-height: 110px;
          }
        }

        @media (max-width: 1100px) {
          .wrap {
            padding: 18px 14px 50px;
          }

          header {
            align-items: flex-start;
          }

          .header-controls {
            width: 100%;
          }

          .col-4, .col-6, .col-8 {
            grid-column: span 12;
          }

          .metrics-4 {
            grid-template-columns: repeat(4, minmax(0, 1fr));
          }

          .gauge {
            width: 128px; height: 128px; flex-basis: 128px;
          }
        }

        @media (max-width: 670px) {
          .wrap {
            padding: 14px 10px 40px;
          }

          header {
            gap: 12px; margin-bottom: 0;
          }

          .brand {
            gap: 10px;
          }

          .logo {
            width: 40px; height: 40px; border-radius: 11px;
          }

          .logo svg {
            width: 22px; height: 22px;
          }

          .brand h1 {
            font-size: 17px;
          }

          .brand p {
            font-size: 11px;
          }

          .header-controls {
            display: grid;
            grid-template-columns: repeat(2, minmax(0, 1fr));
            width: 100%;
            gap: 8px;
          }

          .header-controls > * {
            min-width: 0; width: 100%; justify-content: center;
          }

          .pill {
            padding: 8px 10px;
          }

          .ctrl {
            padding: 7px 9px;
          }

          .summary {
            grid-template-columns: repeat(2, minmax(0, 1fr)); gap: 10px; margin-bottom: 10px;
          }

          .stat {
            padding: 12px 14px;
          }

          .stat .k {
            font-size: 11px;
          }

          .stat .v {
            font-size: 22px;
          }

          .stat .v small {
            font-size: 13px;
          }

          .stat .sub {
            font-size: 11.5px;
          }

          .grid {
            gap: 10px;
          }

          .grid + .grid {
            margin-top: 10px;
          }

          .card {
            padding: 16px; border-radius: 14px;
          }

          .card-head {
            margin-bottom: 14px;
          }

          .metrics {
            grid-template-columns: repeat(2, minmax(0, 1fr)) !important; gap: 9px;
          }

          .metrics-4 {
            grid-template-columns: repeat(2, minmax(0, 1fr)) !important;
          }

          .metric {
            min-width: 0; padding: 11px 10px; overflow: hidden;
          }

          .metric-wide {
            grid-column: span 2 !important;
          }

          .metric .lbl {
            white-space: nowrap; overflow: hidden; text-overflow: ellipsis;
          }

          .metric .num {
            font-size: 19px; white-space: nowrap;
          }

          .gauge-wrap {
            flex-direction: column; align-items: stretch; gap: 14px;
          }

          .gauge {
            width: 128px; height: 128px; flex-basis: 128px; margin: 0 auto;
          }

          .gauge .center .g-num {
            font-size: 27px;
          }

          .gauge-side {
            grid-template-columns: repeat(2, minmax(0, 1fr)); gap: 9px;
          }

          .metrics-row {
            grid-template-columns: repeat(2, minmax(0, 1fr)); gap: 9px; margin-top: 12px;
          }

          .temperature-wrap {
            align-items: center; justify-content: center;
          }

          .flow {
            grid-template-columns: 1fr auto 1fr auto 1fr; gap: 5px; padding: 12px 8px;
          }

          .flow-node .fn-ico {
            width: 36px; height: 36px; margin-bottom: 6px;
          }

          .flow-node .fn-val {
            font-size: 13px;
          }

          .flow-node .fn-lbl {
            font-size: 10px;
          }

          .flow-arrow svg {
            width: 18px;
          }

          .value-grid {
            grid-template-columns: repeat(2, minmax(0, 1fr)); gap: 9px;
          }

          .value-box {
            min-width: 0; padding: 11px;
          }

          .value-box .value {
            font-size: 17px;
          }

          .collapsible-content {
            padding: 0 14px 16px;
          }

          details summary {
            padding: 15px 16px;
          }

          .flag {
            padding: 7px 10px; font-size: 12px;
          }
        }

        @media (max-width: 380px) {
          .header-controls {
            grid-template-columns: 1fr;
          }

          .gauge {
            width: 120px; height: 120px; flex-basis: 120px;
          }

          .gauge .center .g-num {
            font-size: 25px;
          }

          .gauge-side {
            grid-template-columns: 1fr;
          }

          .metrics-row {
            grid-template-columns: 1fr;
          }

          .metrics {
            gap: 8px;
          }

          .metric .num {
            font-size: 18px;
          }

          .flow-node .fn-lbl {
            display: none;
          }

          .value-grid {
            grid-template-columns: 1fr;
          }
        }

        @media (prefers-reduced-motion: reduce) {
          .status-dot.online,
          .flow-node.source-active .fn-ico {
            animation: none !important;
          }
        }

        @media (max-width: 700px) {
          .flow {
            padding: 17px 10px; min-height: 160px;
          }

          .flow-node .fn-ico {
            width: 46px; height: 46px;
          }

          .flow-node .fn-ico svg {
            width: 24px; height: 24px;
          }

          .flow-node .fn-val {
            font-size: 19px;
          }

          .flow-node .fn-lbl {
            font-size: 11px;
          }

          .flow-arrow svg {
            width: 27px;
          }
        }
    </style>
  </head>
  <body>
    <!-- ======================================================
       HEADER — sticky glass bar (lives outside .wrap so it
       spans full viewport width with no side-padding gap)
       ====================================================== -->
    <header>
      <div class="brand">
        <div class="logo" aria-hidden="true">
          <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round">
            <path d="M13 2 3 14h7l-1 8 10-12h-7l1-8z" />
          </svg>
        </div>
        <div>
          <h1>Inverter Monitor - InfiniSolar VII 5KW</h1>
          <p>Real-time power system telemetry</p>
        </div>
      </div>
      <div class="header-controls">
        <div class="pill">
          <span id="systemDot" class="status-dot"></span>
          <span id="systemStatus">Connecting…</span>
        </div>
        <div class="ctrl">
          <label for="refreshRate">Refresh</label>
          <select id="refreshRate" aria-label="Refresh rate">
            <option value="1000" selected>1s</option>
            <option value="2000">2s</option>
            <option value="5000">5s</option>
            <option value="10000">10s</option>
          </select>
        </div>
        <a class="tbtn" href="./serial">
          <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
            <rect x="3" y="4" width="18" height="16" rx="2" />
            <path d="m7 9 3 3-3 3M13 15h4" />
          </svg>
          <span>Serial</span>
        </a>
        <a class="tbtn" href="./settings">
          <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
            <path d="M12.22 2h-.44a2 2 0 0 0-2 2v.18a2 2 0 0 1-1 1.73l-.43.25a2 2 0 0 1-2 0l-.15-.08a2 2 0 0 0-2.73.73l-.22.38a2 2 0 0 0 .73 2.73l.15.1a2 2 0 0 1 1 1.72v.51a2 2 0 0 1-1 1.74l-.15.09a2 2 0 0 0-.73 2.73l.22.38a2 2 0 0 0 2.73.73l.15-.08a2 2 0 0 1 2 0l.43.25a2 2 0 0 1 1 1.73V20a2 2 0 0 0 2 2h.44a2 2 0 0 0 2-2v-.18a2 2 0 0 1 1-1.73l.43-.25a2 2 0 0 1 2 0l.15.08a2 2 0 0 0 2.73-.73l.22-.39a2 2 0 0 0-.73-2.73l-.15-.08a2 2 0 0 1-1-1.74v-.5a2 2 0 0 1 1-1.74l.15-.09a2 2 0 0 0 .73-2.73l-.22-.38a2 2 0 0 0-2.73-.73l-.15.08a2 2 0 0 1-2 0l-.43-.25a2 2 0 0 1-1-1.73V4a2 2 0 0 0-2-2z" />
            <circle cx="12" cy="12" r="3" />
          </svg>
          <span>Settings</span>
        </a>
        <a class="tbtn" href="./history">
          <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
            <polyline points="23 6 13.5 15.5 8.5 10.5 1 18" />
            <polyline points="17 6 23 6 23 12" />
          </svg>
          <span>History</span>
        </a>
        <button class="tbtn" id="installBtn" style="display:none" aria-label="Install app">
          <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
            <path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4"/>
            <polyline points="7 10 12 15 17 10"/>
            <line x1="12" y1="15" x2="12" y2="3"/>
          </svg>
          <span>Install</span>
        </button>
        <button class="tbtn" id="themeToggle" aria-label="Toggle theme">
          <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
            <circle cx="12" cy="12" r="4" />
            <path d="M12 2v2M12 20v2M4.9 4.9l1.4 1.4M17.7 17.7l1.4 1.4M2 12h2M20 12h2M4.9 19.1l1.4-1.4M17.7 6.3l1.4-1.4" />
          </svg>
          <span id="themeLabel">Light</span>
        </button>
      </div>
    </header>
    <div class="wrap">
      <!-- ======================================================
         OFFLINE BANNER
         ====================================================== -->
      <div id="offlineBanner" role="alert">
        <div class="ob-ico" aria-hidden="true">
          <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
            <path d="M12 9v4M12 17h.01" />
            <path d="M10.3 3.9 1.8 18a2 2 0 0 0 1.7 3h17a2 2 0 0 0 1.7-3L13.7 3.9a2 2 0 0 0-3.4 0z" />
          </svg>
        </div>
        <div>
          <strong id="offlineTitle">No connection to inverter</strong>
          <span id="offlineDetail">Waiting to reach <code>/api/data</code>. Values below are the last known readings. </span>
        </div>
      </div>
      <!-- ======================================================
         XL SPLIT: FLOW (left) + SUMMARY STRIP (right)
         On ≥1280 px these sit side-by-side; below they stack.
         ====================================================== -->
      <div class="xl-split">
      <!-- ── FLOW DIAGRAM ── -->
      <div class="card" style="--accent: var(--amber)">
          <div class="card-head">
            <div class="card-title">
              <span class="icon">
                <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                  <path d="M3 12h4l3-9 4 18 3-9h4" />
                </svg>
              </span> Energy Flow
            </div>
            <span id="flowModeBadge" class="status-badge">—</span>
          </div>

          <svg id="flowSvg" viewBox="0 0 480 308" width="100%" style="display:block;overflow:visible;max-height:320px" aria-label="Energy flow diagram">
            <defs>
              <!-- Glow filter -->
              <filter id="fng" x="-40%" y="-40%" width="180%" height="180%">
                <feGaussianBlur stdDeviation="4" result="b"/>
                <feMerge><feMergeNode in="b"/><feMergeNode in="SourceGraphic"/></feMerge>
              </filter>
              <!-- Arrow markers -->
              <marker id="mAmber" markerWidth="7" markerHeight="7" refX="6" refY="3.5" orient="auto">
                <polygon points="0 0, 7 3.5, 0 7" fill="#ffb648"/>
              </marker>
              <marker id="mGreen" markerWidth="7" markerHeight="7" refX="6" refY="3.5" orient="auto">
                <polygon points="0 0, 7 3.5, 0 7" fill="#3ee08f"/>
              </marker>
              <marker id="mBlue" markerWidth="7" markerHeight="7" refX="6" refY="3.5" orient="auto">
                <polygon points="0 0, 7 3.5, 0 7" fill="#4fc3ff"/>
              </marker>
              <marker id="mGrey" markerWidth="7" markerHeight="7" refX="6" refY="3.5" orient="auto">
                <polygon points="0 0, 7 3.5, 0 7" fill="#3a4560"/>
              </marker>
              <marker id="mGreyB" markerWidth="7" markerHeight="7" refX="1" refY="3.5" orient="auto">
                <polygon points="7 0, 0 3.5, 7 7" fill="#3a4560"/>
              </marker>
              <marker id="mAmberB" markerWidth="7" markerHeight="7" refX="1" refY="3.5" orient="auto">
                <polygon points="7 0, 0 3.5, 7 7" fill="#ffb648"/>
              </marker>
              <marker id="mGreenB" markerWidth="7" markerHeight="7" refX="1" refY="3.5" orient="auto">
                <polygon points="7 0, 0 3.5, 7 7" fill="#3ee08f"/>
              </marker>
              <marker id="mBlueB" markerWidth="7" markerHeight="7" refX="1" refY="3.5" orient="auto">
                <polygon points="7 0, 0 3.5, 7 7" fill="#4fc3ff"/>
              </marker>
            </defs>

            <!-- ── BASE LINES (always dim) ── -->
            <!-- Solar → Inverter (unidirectional) -->
            <path class="flow-line-idle" d="M107,72 H234 V114" fill="none" stroke="#3a4560" stroke-width="2" stroke-dasharray="6 4" stroke-linecap="round" stroke-linejoin="round"/>
            <!-- Battery ↔ Inverter: charge (Inv→Batt) -->
            <path class="flow-line-idle" d="M245,114 V72 H373" fill="none" stroke="#3a4560" stroke-width="2" stroke-dasharray="6 4" stroke-linecap="round" stroke-linejoin="round"/>
            <!-- Battery ↔ Inverter: discharge (Batt→Inv) -->
            <path class="flow-line-idle" d="M373,72 H245 V114" fill="none" stroke="#3a4560" stroke-width="2" stroke-dasharray="6 4" stroke-linecap="round" stroke-linejoin="round"/>
            <!-- Grid ↔ Inverter: import (Grid→Inv) -->
            <path class="flow-line-idle" d="M107,228 H234 V186" fill="none" stroke="#3a4560" stroke-width="2" stroke-dasharray="6 4" stroke-linecap="round" stroke-linejoin="round"/>
            <!-- Grid ↔ Inverter: export (Inv→Grid) -->
            <path class="flow-line-idle" d="M234,186 V228 H107" fill="none" stroke="#3a4560" stroke-width="2" stroke-dasharray="6 4" stroke-linecap="round" stroke-linejoin="round"/>
            <!-- Inverter → Load (unidirectional) -->
            <path class="flow-line-idle" d="M245,186 V228 H373" fill="none" stroke="#3a4560" stroke-width="2" stroke-dasharray="6 4" stroke-linecap="round" stroke-linejoin="round"/>
            
            <!-- ── ACTIVE ANIMATED LINES ── -->
            <path id="flSolar"      opacity="0" d="M107,72 H234 V114"  fill="none" stroke="#ffb648" stroke-width="2.5" stroke-dasharray="8 5" stroke-linecap="round" stroke-linejoin="round"/>
            <path id="flBattCharge" opacity="0" d="M245,114 V72 H373"  fill="none" stroke="#3ee08f" stroke-width="2.5" stroke-dasharray="8 5" stroke-linecap="round" stroke-linejoin="round"/>
            <path id="flBattDisch"  opacity="0" d="M373,72 H245 V114"  fill="none" stroke="#3ee08f" stroke-width="2.5" stroke-dasharray="8 5" stroke-linecap="round" stroke-linejoin="round"/>
            <path id="flGridImport" opacity="0" d="M107,228 H234 V186" fill="none" stroke="#4fc3ff" stroke-width="2.5" stroke-dasharray="8 5" stroke-linecap="round" stroke-linejoin="round"/>
            <path id="flGridExport" opacity="0" d="M234,186 V228 H107" fill="none" stroke="#4fc3ff" stroke-width="2.5" stroke-dasharray="8 5" stroke-linecap="round" stroke-linejoin="round"/>
            <path id="flLoad"       opacity="0" d="M245,186 V228 H373" fill="none" stroke="#4fc3ff" stroke-width="2.5" stroke-dasharray="8 5" stroke-linecap="round" stroke-linejoin="round"/>

            <!-- ── SOLAR NODE (top-left) ── -->
            <g id="fnodeSolar" class="fnode-idle">
              <circle cx="72" cy="72" r="42" fill="rgba(255,182,72,.07)" stroke="rgba(255,182,72,.25)" stroke-width="1"/>
              <circle class="outer" cx="72" cy="72" r="30" fill="rgba(255,182,72,.12)" stroke="#ffb648" stroke-width="1.8"/>
              <g transform="translate(61,61)" fill="none" stroke="#ffb648" stroke-width="1.7" stroke-linecap="round">
                <circle cx="11" cy="11" r="4"/>
                <path d="M11 2v2M11 18v2M4.2 4.2l1.4 1.4M16.4 16.4l1.4 1.4M2 11h2M18 11h2M4.2 17.8l1.4-1.4M16.4 5.6l1.4-1.4"/>
              </g>
              <text x="72" y="20" text-anchor="middle" fill="#ffb648" font-size="11" font-weight="700" letter-spacing=".05em">SOLAR</text>
              <text x="72" y="128" text-anchor="middle" font-size="13" font-weight="800" font-family="ui-monospace,monospace" fill="var(--text)">
                <tspan id="flowSolar">--</tspan><tspan fill="var(--muted)" font-size="11"> W</tspan>
              </text>
            </g>

            <!-- ── BATTERY NODE (top-right) ── -->
            <g id="fnodeBatt" class="fnode-active">
              <circle cx="408" cy="72" r="42" fill="rgba(62,224,143,.07)" stroke="rgba(62,224,143,.25)" stroke-width="1"/>
              <circle class="outer" cx="408" cy="72" r="30" fill="rgba(62,224,143,.12)" stroke="#3ee08f" stroke-width="1.8"/>
              <g transform="translate(397,64)" fill="none" stroke="#3ee08f" stroke-width="1.7" stroke-linecap="round">
                <rect x="1" y="2" width="16" height="11" rx="2"/>
                <path d="M19 7v4M5 8v5M9 7v6"/>
              </g>
              <text x="408" y="20" text-anchor="middle" fill="#3ee08f" font-size="11" font-weight="700" letter-spacing=".05em">BATTERY</text>
              <text x="408" y="128" text-anchor="middle" font-size="13" font-weight="800" font-family="ui-monospace,monospace" fill="var(--text)">
                <tspan id="flowBatt">--</tspan><tspan fill="var(--muted)" font-size="11">%</tspan>
              </text>
              <text x="408" y="142" text-anchor="middle" font-size="10.5" fill="var(--muted)" font-weight="600" id="flowBattState">—</text>
            </g>

            <!-- ── INVERTER NODE (center) ── -->
            <g id="fnodeInv" class="fnode-active">
              <circle cx="240" cy="150" r="65" fill="rgba(255,182,72,.07)" stroke="rgba(255,182,72,.25)" stroke-width="1"/>
              <circle class="outer" cx="240" cy="150" r="50" fill="rgba(255,182,72,.12)" stroke="#ffb648" stroke-width="1.8"/>
              <g transform="translate(215,118)" fill="none" stroke="#ffb648" stroke-linecap="round" stroke-linejoin="round">
                <!-- body -->
                <rect x="0" y="0" width="50" height="64" rx="4" stroke-width="1.8"/>
                <!-- fan grille -->
                <line x1="8" y1="6"  x2="42" y2="6"  stroke-width="1.1"/>
                <line x1="8" y1="11" x2="42" y2="11" stroke-width="1.1"/>
                <line x1="8" y1="16" x2="42" y2="16" stroke-width="1.1"/>
                <!-- display -->
                <rect x="6" y="21" width="38" height="20" rx="2" stroke-width="1.3"/>
                <!-- sine wave on display -->
                <path d="M9 31 C13 24,17 24,21 31 C25 38,29 38,33 31 C37 24,40 24,43 31" stroke-width="1.3" stroke-linecap="round"/>
                <!-- DC terminal -->
                <rect x="8"  y="46" width="10" height="6" rx="1" stroke-width="1.1"/>
                <!-- AC terminal -->
                <rect x="32" y="46" width="10" height="6" rx="1" stroke-width="1.1"/>
                <!-- labels below terminals -->
                <text x="13" y="60" text-anchor="middle" fill="#ffb648" font-size="6" font-weight="700" stroke="none">DC</text>
                <text x="37" y="60" text-anchor="middle" fill="#ffb648" font-size="6" font-weight="700" stroke="none">AC</text>
                <!-- status LED -->
                <circle cx="25" cy="57" r="2.5" stroke-width="1.1"/>
              </g>
            </g>

            <!-- ── GRID NODE (bottom-left) ── -->
            <g id="fnodeGrid" class="fnode-idle">
              <circle cx="72" cy="228" r="42" fill="rgba(79,195,255,.07)" stroke="rgba(79,195,255,.25)" stroke-width="1"/>
              <circle class="outer" cx="72" cy="228" r="30" fill="rgba(79,195,255,.12)" stroke="#4fc3ff" stroke-width="1.8"/>
              <g transform="translate(60,217)" fill="none" stroke="#4fc3ff" stroke-width="1.6" stroke-linecap="round">
                <path d="M2 0h20M4 0v22M18 0v22M2 22h20M7 0v22M13 0v22M2 7h20M2 15h20"/>
              </g>
              <text x="72" y="284" text-anchor="middle" fill="#4fc3ff" font-size="11" font-weight="700" letter-spacing=".05em">GRID</text>
              <text x="72" y="298" text-anchor="middle" font-size="13" font-weight="800" font-family="ui-monospace,monospace" fill="var(--text)">
                <tspan id="flowGrid">--</tspan><tspan fill="var(--muted)" font-size="11"> V</tspan>
              </text>
            </g>

            <!-- ── LOAD NODE (bottom-right) ── -->
            <g id="fnodeLoad" class="fnode-idle">
              <circle cx="408" cy="228" r="42" fill="rgba(79,195,255,.07)" stroke="rgba(79,195,255,.25)" stroke-width="1"/>
              <circle class="outer" cx="408" cy="228" r="30" fill="rgba(79,195,255,.12)" stroke="#4fc3ff" stroke-width="1.8"/>
              <g transform="translate(397,216)" fill="none" stroke="#4fc3ff" stroke-width="1.7" stroke-linecap="round" stroke-linejoin="round">
                <path d="M4 14a1 1 0 0 1-.8-1.6l9.4-12.6c.3-.5 1.1-.2 1 .4L11.6 9H20a1 1 0 0 1 .8 1.6l-9.4 12.6c-.3.5-1.1.2-1-.4L13.4 15H4z"/>
              </g>
              <text x="408" y="284" text-anchor="middle" fill="#4fc3ff" font-size="11" font-weight="700" letter-spacing=".05em">LOAD</text>
              <text x="408" y="298" text-anchor="middle" font-size="13" font-weight="800" font-family="ui-monospace,monospace" fill="var(--text)">
                <tspan id="flowLoad">--</tspan><tspan fill="var(--muted)" font-size="11"> W</tspan>
              </text>
            </g>
          </svg>
        </div>
      <!-- ── SUMMARY STRIP (right column on xl) ── -->
      <section class="summary" id="summaryStrip">
        <div class="stat" style="--accent: var(--charge)">
          <div class="k">Battery</div>
          <div class="v">
            <span id="sumBattery">--</span>
            <small>%</small>
          </div>
          <div class="sub" id="sumBatteryV">-- V</div>
        </div>
        <div class="stat" style="--accent: var(--amber)">
          <div class="k">Solar Input</div>
          <div class="v">
            <span id="sumSolar">--</span>
            <small>W</small>
          </div>
          <div class="sub" id="sumSolarV">-- V</div>
        </div>
        <div class="stat" style="--accent: var(--grid)">
          <div class="k">Load</div>
          <div class="v">
            <span id="sumLoad">--</span>
            <small>%</small>
          </div>
          <div class="sub" id="sumLoadW">-- W</div>
        </div>
        <div class="stat" style="--accent: var(--discharge)">
          <div class="k">Inverter Temp</div>
          <div class="v">
            <span id="sumTemp">--</span>
            <small>°C</small>
          </div>
          <div class="sub" id="sumTempState">--</div>
        </div>
      </section>
      </div><!-- /.xl-split -->
      <!-- ======================================================
         MAIN CARD GRID
         ====================================================== -->
      <div class="grid">
        <!-- BATTERY -->
        <div class="card col-4" style="--accent: var(--charge)">
          <div class="card-head">
            <div class="card-title">
              <span class="icon">
                <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                  <rect x="2" y="7" width="16" height="10" rx="2" />
                  <path d="M22 11v2M6 11v2M10 10v4" />
                </svg>
              </span> Battery
            </div>
            <span class="badge" id="batteryStatus">--</span>
          </div>
          <div class="gauge-wrap">
            <div class="gauge gauge-new" style="width:152px;height:152px;flex:0 0 152px">
              <svg viewBox="0 0 160 160" style="width:100%;height:100%">
                <defs>
                  <filter id="bglow" x="-25%" y="-25%" width="150%" height="150%">
                    <feGaussianBlur stdDeviation="3.5" result="b" />
                    <feMerge>
                      <feMergeNode in="b" />
                      <feMergeNode in="SourceGraphic" />
                    </feMerge>
                  </filter>
                </defs>
                <circle fill="none" stroke="var(--border)" stroke-width="9" stroke-linecap="round" cx="80" cy="80" r="62" stroke-dasharray="292.17 97.39" transform="rotate(135 80 80)" />
                <circle fill="none" id="battGauge" stroke="#34d399" stroke-width="9" stroke-linecap="round" cx="80" cy="80" r="62" stroke-dasharray="292.17 389.56" stroke-dashoffset="292.17" transform="rotate(135 80 80)" filter="url(#bglow)" style="transition:stroke-dashoffset .7s cubic-bezier(.22,1,.36,1),stroke .4s" />
              </svg>
              <div class="center">
                <div class="g-num">
                  <span id="battPercent">--</span>%
                </div>
                <div class="g-lbl">state of charge</div>
              </div>
            </div>
            <div class="gauge-side">
              <div class="metric metric-wide">
                <div class="lbl">Voltage</div>
                <div class="num">
                  <span id="batteryVoltage">--</span>
                  <small>V</small>
                </div>
              </div>
              <div class="metric">
                <div class="lbl">Charge Current</div>
                <div class="num">
                  <span id="batteryChargeAmps">--</span>
                  <small>A</small>
                </div>
              </div>
              <div class="metric">
                <div class="lbl">Discharge Current</div>
                <div class="num">
                  <span id="batteryDischargeAmps">--</span>
                  <small>A</small>
                </div>
              </div>
              <div class="metric">
                <div class="lbl">Charged Today</div>
                <div class="num">
                  <span id="batteryChargeKWh">--</span>
                  <small>kWh</small>
                </div>
              </div>
              <div class="metric">
                <div class="lbl">Discharged Today</div>
                <div class="num">
                  <span id="batteryDischargeKWh">--</span>
                  <small>kWh</small>
                </div>
              </div>
            </div>
          </div>
        </div>
        <!-- SOLAR / PV -->
        <div class="card col-4" style="--accent: var(--amber)">
          <div class="card-head">
            <div class="card-title">
              <span class="icon">
                <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                  <circle cx="12" cy="12" r="4" />
                  <path d="M12 2v2M12 20v2M4.9 4.9l1.4 1.4M17.7 17.7l1.4 1.4M2 12h2M20 12h2M4.9 19.1l1.4-1.4M17.7 6.3l1.4-1.4" />
                </svg>
              </span> Solar (PV)
            </div>
          </div>
          <div class="metrics">
            <div class="metric">
              <div class="lbl">Voltage</div>
              <div class="num">
                <span id="pvInputVoltage">--</span>
                <small>V</small>
              </div>
            </div>
            <div class="metric">
              <div class="lbl">Current</div>
              <div class="num">
                <span id="pvInputAmps">--</span>
                <small>A</small>
              </div>
            </div>
            <div class="metric">
              <div class="lbl">Power</div>
              <div class="num">
                <span id="pvInputWatts">--</span>
                <small>W</small>
              </div>
            </div>
            <div class="metric">
              <div class="lbl">Generation Today</div>
              <div class="num">
                <span id="pvInputKWh">--</span>
                <small>kWh</small>
              </div>
            </div>
          </div>
          <div style="margin-top:16px">
            <div style="display:flex;justify-content:space-between;font-size:12px;color:var(--muted);margin-bottom:6px">
              <span>Generation</span>
              <span id="pvBarLabel">0 W</span>
            </div>
            <div class="bar">
              <span id="pvBar"></span>
            </div>
          </div>
        </div>
        <!-- UTILITY GRID -->
        <div class="card col-4" style="--accent: var(--grid)">
          <div class="card-head">
            <div class="card-title">
              <span class="icon">
                <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                  <path d="M4 3h16M6 3v18M18 3v18M4 21h16M9 3v18M15 3v18M4 9h16M4 15h16" />
                </svg>
              </span> Utility Grid
            </div>
            <span class="badge" id="gridStatus">--</span>
          </div>
          <div class="metrics">
            <div class="metric">
              <div class="lbl">Voltage</div>
              <div class="num">
                <span id="gridVoltage">--</span>
                <small>V</small>
              </div>
            </div>
            <div class="metric">
              <div class="lbl">Frequency</div>
              <div class="num">
                <span id="gridFrequency">--</span>
                <small>Hz</small>
              </div>
            </div>
            <div class="metric">
              <div class="lbl">Power</div>
              <div class="num">
                <span id="estimatedGridPower">--</span>
                <small>W</small>
              </div>
            </div>
            <div class="metric">
              <div class="lbl">Energy</div>
              <div class="num">
                <span id="estimatedGridKWh">--</span>
                <small>kWh</small>
              </div>
            </div>
          </div>
        </div>
        <!-- OUTPUT / LOAD -->
        <div class="card col-8" style="--accent: var(--grid)">
          <div class="card-head">
            <div class="card-title">
              <span class="icon">
                <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                  <path d="M4 14a1 1 0 0 1-.8-1.6l9.4-12.6c.3-.5 1.1-.2 1 .4L11.6 9H20a1 1 0 0 1 .8 1.6l-9.4 12.6c-.3.5-1.1.2-1-.4L13.4 15H4z" />
                </svg>
              </span> Output &amp; Load
            </div>
          </div>
          <div class="metrics metrics-4">
            <div class="metric">
              <div class="lbl">Voltage</div>
              <div class="num">
                <span id="outputVoltage">--</span>
                <small>V</small>
              </div>
            </div>
            <div class="metric">
              <div class="lbl">Frequency</div>
              <div class="num">
                <span id="outputFrequency">--</span>
                <small>Hz</small>
              </div>
            </div>
            <div class="metric">
              <div class="lbl">Active</div>
              <div class="num">
                <span id="outputWatts">--</span>
                <small>W</small>
              </div>
            </div>
            <div class="metric">
              <div class="lbl">Apparent</div>
              <div class="num">
                <span id="outputVA">--</span>
                <small>VA</small>
              </div>
            </div>
          </div>
          <div style="margin-top:18px">
            <div style="display:flex;justify-content:space-between;font-size:12.5px;margin-bottom:7px">
              <span style="color:var(--muted)">Load level</span>
              <span style="font-family:var(--font-num);font-weight:700">
                <span id="loadPercentLabel">--</span> %</span>
            </div>
            <div class="bar">
              <span id="loadBar"></span>
            </div>
          </div>
        </div>
        <!-- TEMPERATURE -->
        <div class="card col-4" style="--accent: var(--discharge)">
          <div class="card-head">
            <div class="card-title">
              <span class="icon">
                <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                  <path d="M14 14.76V4a2 2 0 0 0-4 0v10.76a4 4 0 1 0 4 0z" />
                </svg>
              </span> Temperature
            </div>
            <span class="badge" id="tempBadge">--</span>
          </div>
          <div class="gauge-wrap temperature-wrap">
            <div class="gauge gauge-new" style="width:132px;height:132px;flex:0 0 132px">
              <svg viewBox="0 0 140 140" style="width:100%;height:100%">
                <defs>
                  <filter id="tglow" x="-25%" y="-25%" width="150%" height="150%">
                    <feGaussianBlur stdDeviation="3" result="b" />
                    <feMerge>
                      <feMergeNode in="b" />
                      <feMergeNode in="SourceGraphic" />
                    </feMerge>
                  </filter>
                </defs>
                <circle fill="none" stroke="var(--border)" stroke-width="8" stroke-linecap="round" cx="70" cy="70" r="52" stroke-dasharray="245.04 81.69" transform="rotate(135 70 70)" />
                <circle fill="none" id="tempGauge" stroke="#34d399" stroke-width="8" stroke-linecap="round" cx="70" cy="70" r="52" stroke-dasharray="245.04 326.73" stroke-dashoffset="245.04" transform="rotate(135 70 70)" filter="url(#tglow)" style="transition:stroke-dashoffset .7s cubic-bezier(.22,1,.36,1),stroke .4s" />
              </svg>
              <div class="center">
                <div class="g-num">
                  <span id="inverterTemp">--</span>°
                </div>
                <div class="g-lbl">Inverter</div>
              </div>
            </div>
          </div>
        </div>
      </div>
      <!-- ======================================================
         ROLLING CHARTS
         ====================================================== -->
      <div class="grid" style="margin-top: var(--gap)">
        <div class="card col-6" style="--accent: var(--amber)">
          <div class="card-head">
            <div class="card-title">Power – Last 1 minute timeline</div>
          </div>
          <div style="display:flex;flex-wrap:wrap;gap:12px;margin-bottom:10px;font-size:12px;color:var(--muted)">
            <span style="display:flex;align-items:center;gap:5px">
              <span style="width:10px;height:3px;border-radius:2px;background:#f5a623;display:inline-block"></span>Solar </span>
            <span style="display:flex;align-items:center;gap:5px">
              <span style="width:10px;height:3px;border-radius:2px;background:#34d399;display:inline-block"></span>Battery </span>
            <span style="display:flex;align-items:center;gap:5px">
              <span style="width:10px;height:3px;border-radius:2px;background:#38bdf8;display:inline-block"></span>Load </span>
          </div>
          <div style="position:relative;width:100%;height:180px">
            <canvas id="chartPwr" role="img" aria-label="Rolling power chart">Power chart</canvas>
          </div>
        </div>
        <div class="card col-6" style="--accent: var(--charge)">
          <div class="card-head">
            <div class="card-title">Battery SOC – Last 1 minute timeline</div>
          </div>
          <div style="position:relative;width:100%;height:180px">
            <canvas id="chartSOC" role="img" aria-label="Battery SOC chart">SOC chart</canvas>
          </div>
        </div>
      </div>
      <!-- ======================================================
         FOOTER
         ====================================================== -->
      <div class="footer"> Last update: <span id="lastUpdate">never</span> · polling <code>/api/data</code>
      </div>
    </div>
    <script src="https://cdnjs.cloudflare.com/ajax/libs/Chart.js/4.4.1/chart.umd.js"></script>
    <script>
      "use strict";
      /* ============================================================
         THEME
         ============================================================ */
      (function() {
        const root = document.documentElement;
        const btn = document.getElementById("themeToggle");
        const label = document.getElementById("themeLabel");
        const meta = document.querySelector('meta[name="theme-color"]');
        try {
          const saved = localStorage.getItem("inv-theme");
          if (saved) root.setAttribute("data-theme", saved);
        } catch (e) {}

        function sync() {
          const dark = root.getAttribute("data-theme") === "dark";
          label.textContent = dark ? "Light" : "Dark";
          if (meta) meta.setAttribute("content", dark ? "#0b1220" : "#eef2f8");
        }
        sync();
        btn.addEventListener("click", function() {
          const next = root.getAttribute("data-theme") === "dark" ? "light" : "dark";
          root.setAttribute("data-theme", next);
          try {
            localStorage.setItem("inv-theme", next);
          } catch (e) {}
          sync();
        });
        new MutationObserver(sync).observe(root, {
          attributes: true,
          attributeFilter: ["data-theme"]
        });
      })();
      /* ============================================================
         ANIMATED COLLAPSIBLES
         ============================================================ */
      (function() {
        document.querySelectorAll("details").forEach(function(details) {
          const summary = details.querySelector("summary");
          const content = details.querySelector(".collapsible-content");
          const chev = summary ? summary.querySelector(".chev") : null;
          if (!summary || !content) return;
          let animation = null;
          let isClosing = false;
          let isExpanding = false;
          summary.addEventListener("click", function(e) {
            e.preventDefault();
            details.style.overflow = "hidden";
            if (isClosing || !details.open) openPanel();
            else if (isExpanding || details.open) closePanel();
          });

          function openPanel() {
            details.style.height = details.offsetHeight + "px";
            details.open = true;
            if (chev) chev.style.transform = "rotate(180deg)";
            requestAnimationFrame(function() {
              isExpanding = true;
              runAnimation(details.offsetHeight, summary.offsetHeight + content.offsetHeight, true);
            });
          }

          function closePanel() {
            isClosing = true;
            if (chev) chev.style.transform = "rotate(0deg)";
            runAnimation(details.offsetHeight, summary.offsetHeight, false);
          }

          function runAnimation(startH, endH, opening) {
            if (animation) animation.cancel();
            animation = details.animate({
              height: [startH + "px", endH + "px"]
            }, {
              duration: 320,
              easing: "cubic-bezier(0.22, 1, 0.36, 1)"
            });
            content.animate(opening ? [{
              opacity: 0,
              transform: "translateY(-6px)"
            }, {
              opacity: 1,
              transform: "translateY(0)"
            }] : [{
              opacity: 1,
              transform: "translateY(0)"
            }, {
              opacity: 0,
              transform: "translateY(-6px)"
            }], {
              duration: opening ? 260 : 180,
              easing: "ease",
              fill: "forwards"
            });
            animation.onfinish = function() {
              finish(opening);
            };
            animation.oncancel = function() {
              isClosing = false;
              isExpanding = false;
            };
          }

          function finish(open) {
            details.open = open;
            animation = null;
            isClosing = false;
            isExpanding = false;
            details.style.height = "";
            details.style.overflow = "";
          }
        });
      })();
      /* ============================================================
         HELPERS
         ============================================================ */
      const $ = (id) => document.getElementById(id);
      const tweens = {};

      function setNum(id, to, decimals) {
        const el = $(id);
        if (!el) return;
        to = Number(to);
        if (!isFinite(to)) {
          el.textContent = "--";
          return;
        }
        decimals = decimals || 0;
        const from = tweens[id] !== undefined ? tweens[id] : to;
        const start = performance.now();
        const dur = 500;

        function step(now) {
          const t = Math.min(1, (now - start) / dur);
          const eased = 1 - Math.pow(1 - t, 3);
          el.textContent = (from + (to - from) * eased).toFixed(decimals);
          if (t < 1) requestAnimationFrame(step);
          else tweens[id] = to;
        }
        tweens[id] = from;
        requestAnimationFrame(step);
      }

      function setText(id, val) {
        const el = $(id);
        if (el && val !== undefined && val !== null) el.textContent = val;
      }

      function setGauge(id, pct, r) {
        const el = $(id);
        if (!el) return;
        pct = Math.max(0, Math.min(100, pct));
        el.style.strokeDashoffset = 2 * Math.PI * (r || 52) * 0.75 * (1 - pct / 100);
      }

      function setBar(id, pct) {
        const el = $(id);
        if (el) el.style.width = Math.max(0, Math.min(100, pct)) + "%";
      }

      function setStale(stale) {
        document.querySelectorAll(".card, .summary, details").forEach(function(c) {
          c.classList.toggle("stale", stale);
        });
      }
      /* ============================================================
         STATUS
         ============================================================ */
      function setStatus(text, online, kind) {
        setText("systemStatus", text);
        const dot = $("systemDot");
        const banner = $("offlineBanner");
        if (dot) dot.className = "status-dot " + (online ? "online" : "offline");
        if (banner) banner.classList.toggle("show", !online);
        if (!online) {
          const titles = {
            network:  "Can't reach the ESP32",
            wifi:     "ESP32 not on WiFi",
            inverter: "No connection to inverter",
            stale:    "Inverter data is stale"
          };
          const details = {
            network:  "No response from the device at all - check it's powered on and you're on the same network, or connect to its setup WiFi (InverterMonitor-Setup) if it's showing one.",
            wifi:     "The ESP32 is up but not connected to a WiFi network. Connect to its open setup network and use the home page to add one.",
            inverter: "The ESP32 is online but isn't getting a response from the inverter. Check the UART wiring.",
            stale:    "Readings below are the last known values."
          };
          setText("offlineTitle", titles[kind] || titles.inverter);
          setText("offlineDetail", details[kind] || details.inverter);
        }
      }
      /* ============================================================
         RENDER
         ============================================================ */
      function render(data) {
        const G = data.GSX || {};
        const M = data.MOD || {};
        const F = data.FLAG || {};
        const P = data.PIRI || {};

        // The device answered (we're inside render(), so the fetch succeeded)
        // but it still might have no WiFi (reachable only via its setup AP)
        // or a silent inverter - two very different problems from "can't
        // reach the ESP32 at all", which is handled in updateData()'s catch.
        if (data.wifiConnected === false) {
          setStatus("ESP32 Not On WiFi", false, "wifi");
          setStale(true);
          return;
        }
        if (!G.valid || data.inverterConnected === false) {
          setStatus("Inverter Not Responding", false, "inverter");
          setStale(true);
          return;
        }
        const age = Number(data.inverterAge);
        if (!isFinite(age) || age <= 10) {
          setStatus("Inverter Connected", true);
          setStale(false);
        } else {
          setStatus("Inverter Data Stale", false, "stale");
          setStale(true);
        }
        /* GRID */
        setNum("gridVoltage", G.gridVoltage, 1);
        setNum("gridFrequency", G.gridFrequency, 1);
        setNum("estimatedGridPower", G.estimatedGridPower, 0);
        setNum("estimatedGridKWh", G.dailyEstimatedGridKWh, 3);
        const gridStatusEl = $("gridStatus");
        if (gridStatusEl) {
          gridStatusEl.textContent = G.gridAvailable ? "ONLINE" : "OFFLINE";
          gridStatusEl.className = "badge " + (G.gridAvailable ? "good" : "bad");
        }
        /* OUTPUT / LOAD */
        setNum("outputVoltage", G.outputVoltage, 1);
        setNum("outputFrequency", G.outputFrequency, 1);
        setNum("outputWatts", G.outputWatts, 0);
        setNum("outputVA", G.outputVA, 0);
        setNum("dailyOutputEnergyKWh", G.dailyOutputEnergyKWh, 3);
        const load = Math.max(0, Math.min(100, Number(G.loadPercent) || 0));
        setNum("loadPercentLabel", load, 0);
        setBar("loadBar", load);
        /* BATTERY */
        const batteryVoltage = Number(G.batteryVoltage) || 0;
        const batteryChargeAmps = Number(G.batteryChargeAmps) || 0;
        const batteryDischargeAmps = Number(G.batteryDischargeAmps) || 0;
        const battPct = Math.max(0, Math.min(100, Number(G.batteryPercentage) || 0));
        setNum("batteryVoltage", batteryVoltage, 1);
        setNum("batteryChargeAmps", batteryChargeAmps, 1);
        setNum("batteryDischargeAmps", batteryDischargeAmps, 1);
        setNum("batteryChargeKWh", G.dailyBatteryChargeKWh, 3);
        setNum("batteryDischargeKWh", G.dailyBatteryDischargeKWh, 3);
        setNum("battPercent", battPct, 0);
        setGauge("battGauge", battPct, 62);
        const bg = $("battGauge");
        if (bg) bg.style.stroke = battPct < 20 ? "var(--discharge)" : battPct < 50 ? "var(--amber)" : "var(--charge)";
        const bstat = $("batteryStatus");
        if (bstat) {
          if (batteryChargeAmps > 0.5) {
            bstat.textContent = "CHARGING";
            bstat.className = "badge good";
          } else if (batteryDischargeAmps > 0.5) {
            bstat.textContent = "DISCHARGING";
            bstat.className = "badge bad";
          } else {
            bstat.textContent = "IDLE";
            bstat.className = "badge";
          }
        }
        /* SOLAR / PV */
        const pvV = Number(G.pv1InputVoltage) || 0;
        const pvW = Number(G.pv1InputWatts) || 0;
        setNum("pvInputVoltage", pvV, 1);
        setNum("pvInputWatts", pvW, 0);
        setNum("pvInputAmps", pvV > 0 ? pvW / pvV : 0, 1);
        setNum("pvInputKWh", G.dailyPVInputKWh, 3);
        setBar("pvBar", Math.min(100, (pvW / 5000) * 100));
        $("pvBarLabel").textContent = pvW.toFixed(0) + " W";
        /* TEMPERATURE */
        const temp = Number(G.inverterTemp) || 0;
        setNum("inverterTemp", temp, 0);
        setGauge("tempGauge", (temp / 90) * 100, 52);
        const tg = $("tempGauge");
        const tb = $("tempBadge");
        if (tg && tb) {
          if (temp >= 70) {
            tg.style.stroke = "var(--discharge)";
            tb.textContent = "HOT";
            tb.className = "badge bad";
          } else if (temp >= 50) {
            tg.style.stroke = "var(--amber)";
            tb.textContent = "WARM";
            tb.className = "badge";
          } else {
            tg.style.stroke = "var(--charge)";
            tb.textContent = "NORMAL";
            tb.className = "badge good";
          }
        }
        /* SUMMARY STRIP */
        setNum("sumBattery", battPct, 0);
        setText("sumBatteryV", batteryVoltage.toFixed(1) + " V");
        setNum("sumSolar", pvW, 0);
        setText("sumSolarV", pvV.toFixed(1) + " V");
        setNum("sumLoad", load, 0);
        setText("sumLoadW", (Number(G.outputWatts) || 0).toFixed(0) + " W");
        setNum("sumTemp", temp, 0);
        setText("sumTempState", temp >= 70 ? "Running hot" : temp >= 51 ? "Warm" : "Nominal");
        /* ENERGY FLOW */
        const gridW = Number(G.estimatedGridPower) || 0;
        const solarLow = pvW < 50;
                /* ── Energy Flow Diagram ── */
        setNum("flowSolar", pvW, 0);
        setNum("flowBatt",  battPct, 0);
        setText("flowGrid", (Number(G.gridVoltage)||0).toFixed(1));
        setNum("flowLoad",  G.outputWatts, 0);

        const cA  = Number(G.batteryChargeAmps)    || 0;
        const dA  = Number(G.batteryDischargeAmps) || 0;
        const gW  = Number(G.estimatedGridPower)   || 0;
        const oW  = Number(G.outputWatts)          || 0;
        const gridAvail = !!G.gridAvailable;

        /* Active flags */
        const solarOn   = pvW > 50;
        const battChg   = cA  > 0.5;
        const battDisch = dA  > 0.5;
        const gridImp   = gridAvail && gW > 50;
        const gridExp   = gW < -50;
        const loadOn    = oW > 20;

        /* Flow line helper */
        function setFlow(id, active, cls) {
          const el = $(id);
          if (!el) return;
          el.setAttribute("opacity", active ? "1" : "0");
          el.setAttribute("class",   active ? "flow-line-active" : "");
          if (active) el.setAttribute("stroke", cls);
        }

        setFlow("flSolar",      solarOn,   "#ffb648");
        setFlow("flBattCharge", battChg,   "#3ee08f");
        setFlow("flBattDisch",  battDisch,  "#3ee08f");
        setFlow("flGridImport", gridImp,   "#4fc3ff");
        setFlow("flGridExport", gridExp,   "#4fc3ff");
        setFlow("flLoad",       loadOn,    "#4fc3ff");

        /* Node active/idle */
        function setNode(id, active) {
          const el = $(id);
          if (el) el.setAttribute("class", active ? "fnode-active" : "fnode-idle");
        }

        setNode("fnodeSolar", solarOn);
        setNode("fnodeBatt",  true);   /* always present */
        setNode("fnodeInv",   true);   /* always present */
        setNode("fnodeGrid",  gridAvail);
        setNode("fnodeLoad",  loadOn);

        /* Battery state label */
        setText("flowBattState", battChg ? "Charging" : battDisch ? "Discharging" : "Idle");

        /* Mode badge */
        const mb = $("flowModeBadge");
        if (mb) {
          if (solarOn && !gridImp)      { mb.textContent = "Solar"; mb.className = "status-badge good"; }
          else if (solarOn && gridImp)  { mb.textContent = "Solar + Grid"; mb.className = "status-badge good"; }
          else if (gridImp)             { mb.textContent = "Grid"; mb.className = "status-badge"; }
          else if (battDisch)           { mb.textContent = "Battery"; mb.className = "status-badge warn"; }
          else                          { mb.textContent = "Standby"; mb.className = "status-badge"; }
        }
                
        updateCharts(G);
        lastUpdateAt = Date.now();
        updateFooterTime();
      }

      

      /* ============================================================
         FOOTER TIME
         ============================================================ */
      let lastUpdateAt = null;

      function updateFooterTime() {
        if (lastUpdateAt === null) return;
        const secs = Math.round((Date.now() - lastUpdateAt) / 1000);
        setText("lastUpdate", secs < 2 ? "just now" : secs < 60 ? secs + "s ago" : Math.floor(secs / 60) + "m ago");
      }
      setInterval(updateFooterTime, 1000);
      /* ============================================================
         DATA POLLING
         ============================================================ */
      async function updateData() {
        try {
          const res = await fetch("./api/data", {
            cache: "no-store"
          });
          if (!res.ok) throw new Error("HTTP " + res.status);
          render(await res.json());
        } catch (err) {
          console.log("[v0] /api/data request failed:", err.message);
          // The request itself failed (timeout/no route) - a genuinely
          // different case from a successful response reporting no data.
          setStatus("Can't Reach Device", false, "network");
          setStale(true);
        }
      }
      /* ============================================================
         REFRESH CONTROL
         ============================================================ */
      let refreshInterval = 1000;
      let refreshTimer = null;

      function startTimer() {
        if (refreshTimer) clearInterval(refreshTimer);
        refreshTimer = setInterval(updateData, refreshInterval);
      }
      $("refreshRate").addEventListener("change", function() {
        refreshInterval = Number(this.value);
        startTimer();
      });
      updateData();
      startTimer();
      if ('serviceWorker' in navigator) { navigator.serviceWorker.register('./sw.js'); }
      (function() {
        let deferredPrompt;
        const btn = document.getElementById('installBtn');
        window.addEventListener('beforeinstallprompt', function(e) {
          e.preventDefault();
          deferredPrompt = e;
          if (btn) btn.style.display = '';
        });
        if (btn) btn.addEventListener('click', function() {
          if (!deferredPrompt) return;
          deferredPrompt.prompt();
          deferredPrompt.userChoice.then(function() {
            deferredPrompt = null;
            btn.style.display = 'none';
          });
        });
        window.addEventListener('appinstalled', function() {
          if (btn) btn.style.display = 'none';
        });
      })();
      
      /* ============================================================
         ROLLING CHARTS
         ============================================================ */
      const MAX_PTS = 60;
      const isDark = () => document.documentElement.getAttribute("data-theme") === "dark";
      const gridColor = () => isDark() ? "#1e2d45" : "#e4eaf5";
      const tickColor = "#90a2bd";

      function mkChart(id, datasets, ymax) {
        return new Chart(document.getElementById(id).getContext("2d"), {
          type: "line",
          data: {
            labels: [],
            datasets: datasets
          },
          options: {
            responsive: true,
            maintainAspectRatio: false,
            animation: false,
            interaction: {
              mode: "index",
              intersect: false
            },
            plugins: {
              legend: {
                display: false
              },
              tooltip: {
                backgroundColor: isDark() ? "#131d31" : "#fff",
                borderColor: isDark() ? "#24334f" : "#d7e0ee",
                borderWidth: 1,
                titleColor: isDark() ? "#e8eef8" : "#17233a",
                bodyColor: "#90a2bd",
                padding: 8
              }
            },
            scales: {
              x: {
                grid: {
                  color: gridColor(),
                  lineWidth: 0.5
                },
                ticks: {
                  color: tickColor,
                  font: {
                    size: 9
                  },
                  maxTicksLimit: 6,
                  maxRotation: 0
                },
                border: {
                  display: false
                }
              },
              y: {
                grid: {
                  color: gridColor(),
                  lineWidth: 0.5
                },
                ticks: {
                  color: tickColor,
                  font: {
                    size: 9
                  }
                },
                beginAtZero: true,
                border: {
                  display: false
                },
                max: ymax || undefined
              }
            }
          }
        });
      }

      function mkDs(color, fill) {
        return {
          data: [],
          borderColor: color,
          backgroundColor: color + (fill ? "22" : "00"),
          borderWidth: 1.5,
          pointRadius: 0,
          tension: 0.3,
          fill: fill || false
        };
      }
      const cPwr = mkChart("chartPwr", [mkDs("#f5a623"), mkDs("#34d399"), mkDs("#38bdf8")]);
      const cSOC = mkChart("chartSOC", [mkDs("#818cf8", true)], 100);

      function pushPt(chart, idx, v) {
        const d = chart.data.datasets[idx].data;
        d.push(v);
        if (d.length > MAX_PTS) d.shift();
      }

      function pushLbl(chart, l) {
        chart.data.labels.push(l);
        if (chart.data.labels.length > MAX_PTS) chart.data.labels.shift();
      }

      function nowLbl() {
        const d = new Date();
        return d.getHours().toString().padStart(2, "0") + ":" + d.getMinutes().toString().padStart(2, "0") + ":" + d.getSeconds().toString().padStart(2, "0");
      }

      function updateCharts(G) {
        const pvW = Number(G.pv1InputWatts) || 0;
        const battChargeW  = Number(G.batteryChargePower)    || 0;
        const battDischargeW = Number(G.batteryDischargePower) || 0;
        const battW = battDischargeW > 0 ? -battDischargeW : battChargeW;
        const outW = Number(G.outputWatts) || 0;
        const soc = Math.max(0, Math.min(100, Number(G.batteryPercentage) || 0));
        const lbl = nowLbl();
        pushLbl(cPwr, lbl);
        pushLbl(cSOC, lbl);
        pushPt(cPwr, 0, pvW);
        pushPt(cPwr, 1, battW);
        pushPt(cPwr, 2, outW);
        pushPt(cSOC, 0, soc);
        cPwr.update("none");
        cSOC.update("none");
      }
    </script>
  </body>
</html>
)rawliteral";

