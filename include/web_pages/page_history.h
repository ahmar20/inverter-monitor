#pragma once
// ============================================================================
// page_history.h — the /history energy charts page.
// Pure HTML/CSS/JS served as-is by web_server.h. No C++ logic here.
// ============================================================================
#include <Arduino.h>

const char history_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en" data-theme="dark">
  <head>
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width,initial-scale=1" />
    <title>Inverter – History</title>
    <style>
      /* ── TOKENS ─────────────────────────────────────────────────── */

       :root {
          --font-ui: system-ui, -apple-system, "Segoe UI", Roboto, sans-serif;
          --font-num: ui-monospace, "SFMono-Regular", "JetBrains Mono", monospace;
          --amber: #f5a623;
          --charge: #34d399;
          --discharge: #fb7185;
          --grid-c: #38bdf8;
          --purple: #818cf8;
          --radius: 16px;
          --gap: 16px;
          --ease: cubic-bezier(0.22, 1, 0.36, 1);
      }

      html[data-theme="dark"] {
          --bg: #0b1220;
          --bg-grad: radial-gradient(1200px 600px at 80% -10%, #16233b 0%, transparent 60%), radial-gradient(900px 500px at -10% 110%, #101a2e 0%, transparent 55%);
          --surface: #131d31;
          --surface-2: #182545;
          --border: #24334f;
          --text: #e8eef8;
          --muted: #90a2bd;
          --shadow: 0 18px 40px -24px rgba(0, 0, 0, 0.8);
          --glass-bg-strong: rgba(19, 29, 49, .90);
          --glass-bg-light: rgba(19, 29, 49, .45);
      }

      html[data-theme="light"] {
          --bg: #eef2f8;
          --bg-grad: radial-gradient(1200px 600px at 80% -10%, #dbe6f5 0%, transparent 60%), radial-gradient(900px 500px at -10% 110%, #e2ecf7 0%, transparent 55%);
          --surface: #ffffff;
          --surface-2: #f4f7fc;
          --border: #d7e0ee;
          --text: #17233a;
          --muted: #5c6b83;
          --shadow: 0 18px 40px -28px rgba(30, 50, 90, 0.4);
          --glass-bg-strong: rgba(255, 255, 255, .94);
          --glass-bg-light: rgba(255, 255, 255, .45);
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
          min-height: 100vh;
          -webkit-font-smoothing: antialiased;
          transition: background-color 0.3s, color 0.3s;
          position: relative;
      }

      /* ── AURORA ──────────────────────────────────────────────────── */
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

      .wrap {
          width: 100%;
          padding: 20px 18px 60px;
      }
      /* ── HEADER ──────────────────────────────────────────────────── */

      header {
          display: flex;
          flex-wrap: wrap;
          align-items: center;
          justify-content: space-between;
          gap: 14px;
          margin-bottom: 0;
          /* ── Sticky glass bar ── */
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
          gap: 12px;
      }

      .logo {
          width: 42px;
          height: 42px;
          border-radius: 12px;
          display: grid;
          place-items: center;
          background: linear-gradient(145deg, var(--amber), #d97706);
          color: #1a1206;
          flex-shrink: 0;
          box-shadow: 0 6px 16px -6px rgba(245, 166, 35, 0.65);
      }

      .logo svg {
          width: 22px;
          height: 22px;
      }

      .brand h1 {
          font-size: 18px;
          font-weight: 700;
      }

      .brand p {
          font-size: 12px;
          color: var(--muted);
          font-family: var(--font-num);
      }

      .hc {
          display: flex;
          align-items: center;
          gap: 8px;
          flex-wrap: wrap;
      }

      button.tbtn,
      a.tbtn {
          font-family: var(--font-ui);
          display: inline-flex;
          align-items: center;
          gap: 6px;
          padding: 7px 12px;
          border-radius: 10px;
          background: var(--surface-2);
          color: var(--text);
          border: 1px solid var(--border);
          font-size: 13px;
          font-weight: 600;
          cursor: pointer;
          text-decoration: none;
          transition: border-color 0.2s var(--ease);
          white-space: nowrap;
      }

      button.tbtn:hover,
      a.tbtn:hover {
          border-color: var(--amber);
      }
      /* ── RANGE CONTROLS ──────────────────────────────────────────── */

      .controls {
          background: var(--surface);
          border: 1px solid var(--border);
          border-radius: var(--radius);
          padding: 14px 18px;
          margin-bottom: var(--gap);
          box-shadow: var(--shadow);
          display: flex;
          flex-wrap: wrap;
          align-items: flex-end;
          gap: 14px;
      }

      .ctrl-group {
          display: flex;
          flex-direction: column;
          gap: 5px;
      }

      .ctrl-group label {
          font-size: 11px;
          font-weight: 600;
          color: var(--muted);
          text-transform: uppercase;
          letter-spacing: 0.06em;
      }

      .range-btns {
          display: flex;
          flex-wrap: wrap;
          gap: 6px;
      }

      .range-btn {
          font-family: var(--font-ui);
          padding: 6px 14px;
          border-radius: 8px;
          background: var(--surface-2);
          color: var(--muted);
          border: 1px solid var(--border);
          font-size: 12.5px;
          font-weight: 600;
          cursor: pointer;
          transition: all 0.2s var(--ease);
          white-space: nowrap;
      }

      .range-btn:hover {
          border-color: var(--amber);
          color: var(--text);
      }

      .range-btn.active {
          background: var(--amber);
          color: #1a1206;
          border-color: var(--amber);
          box-shadow: 0 4px 12px -4px rgba(245, 166, 35, 0.5);
      }

      .date-inputs {
          display: flex;
          align-items: center;
          gap: 8px;
          flex-wrap: wrap;
      }

      input[type="date"],
      input[type="time"] {
          font-family: var(--font-ui);
          font-size: 13px;
          font-weight: 600;
          background: var(--surface-2);
          color: var(--text);
          border: 1px solid var(--border);
          border-radius: 9px;
          padding: 6px 10px;
          cursor: pointer;
          color-scheme: inherit;
      }

      input[type="date"]:hover,
      input[type="time"]:hover {
          border-color: var(--amber);
      }

      .ctrl-sep {
          color: var(--muted);
          font-size: 13px;
      }

      #loadBtn {
          padding: 7px 18px;
          border-radius: 9px;
          background: var(--amber);
          color: #1a1206;
          border: none;
          font-family: var(--font-ui);
          font-size: 13px;
          font-weight: 700;
          cursor: pointer;
          box-shadow: 0 4px 12px -4px rgba(245, 166, 35, 0.5);
          transition: opacity 0.2s;
          white-space: nowrap;
      }

      #loadBtn:hover {
          opacity: 0.88;
      }

      #loadBtn:disabled {
          opacity: 0.5;
          cursor: not-allowed;
      }
      /* ── STATUS BAR ──────────────────────────────────────────────── */

      #statusBar {
          font-size: 12.5px;
          color: var(--muted);
          margin-left: auto;
          white-space: nowrap;
      }

      #errMsg {
          width: 100%;
          font-size: 13px;
          color: var(--discharge);
          display: none;
          padding-top: 4px;
      }

      #errMsg.show {
          display: block;
      }
      /* ── KPI STRIP ───────────────────────────────────────────────── */

      .kpi-strip {
          display: grid;
          grid-template-columns: repeat(6, 1fr);
          gap: var(--gap);
          margin-bottom: var(--gap);
      }

      .kpi {
          background: var(--surface);
          border: 1px solid var(--border);
          border-radius: var(--radius);
          padding: 14px 16px;
          box-shadow: var(--shadow);
          position: relative;
          overflow: hidden;
      }

      .kpi::before {
          content: "";
          position: absolute;
          left: 0;
          top: 0;
          bottom: 0;
          width: 3px;
          background: var(--ka, var(--amber));
      }

      .kpi .k {
          font-size: 10.5px;
          color: var(--muted);
          text-transform: uppercase;
          letter-spacing: 0.07em;
          font-weight: 600;
      }

      .kpi .v {
          font-family: var(--font-num);
          font-size: clamp(18px, 1.8vw, 24px);
          font-weight: 700;
          margin-top: 4px;
          letter-spacing: -0.02em;
      }

      .kpi .v small {
          font-size: 12px;
          color: var(--muted);
          font-weight: 500;
          margin-left: 2px;
      }
      /* ── CARDS ───────────────────────────────────────────────────── */

      .card {
          background: var(--surface);
          border: 1px solid var(--border);
          border-radius: var(--radius);
          padding: 20px;
          box-shadow: var(--shadow);
          margin-bottom: var(--gap);
          min-width: 0;
      }

      .card-head {
          display: flex;
          align-items: center;
          justify-content: space-between;
          margin-bottom: 14px;
          gap: 10px;
          flex-wrap: wrap;
      }

      .card-title {
          display: flex;
          align-items: center;
          gap: 10px;
          font-size: 14px;
          font-weight: 700;
      }

      .icon {
          width: 32px;
          height: 32px;
          border-radius: 9px;
          display: grid;
          place-items: center;
          background: color-mix(in srgb, var(--ic, var(--amber)) 16%, transparent);
          color: var(--ic, var(--amber));
          flex-shrink: 0;
      }

      .icon svg {
          width: 17px;
          height: 17px;
      }
      /* Legend */

      .legend {
          display: flex;
          flex-wrap: wrap;
          gap: 14px;
          font-size: 12px;
          color: var(--muted);
      }

      .legend span {
          display: flex;
          align-items: center;
          gap: 5px;
      }

      .legend i {
          width: 12px;
          height: 3px;
          border-radius: 2px;
          display: inline-block;
      }
      /* Two-col chart row */

      .chart-row {
          display: grid;
          grid-template-columns: 1fr 1fr;
          gap: var(--gap);
          margin-bottom: var(--gap);
      }

      .chart-row .card {
          margin-bottom: 0;
      }
      /* Footer */

      .footer {
          margin-top: 28px;
          text-align: center;
          font-size: 12px;
          color: var(--muted);
      }

      .footer a {
          color: var(--amber);
          text-decoration: none;
      }
      /* ── RESPONSIVE ──────────────────────────────────────────────── */

      @media (max-width: 1000px) {
          .kpi-strip {
              grid-template-columns: repeat(3, 1fr);
          }
      }

      @media (max-width: 700px) {
          .kpi-strip {
              grid-template-columns: repeat(2, 1fr);
          }
          .chart-row {
              grid-template-columns: 1fr;
          }
      }

      @media (max-width: 420px) {
          .kpi-strip {
              grid-template-columns: repeat(2, 1fr);
          }
      }
      
    </style>
  </head>

  <body>
    <header>
      <div class="brand">
        <div class="logo" aria-hidden="true">
          <svg
            viewBox="0 0 24 24"
            fill="none"
            stroke="currentColor"
            stroke-width="2.2"
            stroke-linecap="round"
            stroke-linejoin="round"
          >
            <polyline points="23 6 13.5 15.5 8.5 10.5 1 18" />
            <polyline points="17 6 23 6 23 12" />
          </svg>
        </div>
        <div>
          <h1>InfiniSolar VII 5KW – History</h1>
          <p id="rangeLabel">Select a range to load data</p>
        </div>
      </div>
      <div class="hc">
        <a class="tbtn" href="./dashboard">
          <svg
            width="14"
            height="14"
            viewBox="0 0 24 24"
            fill="none"
            stroke="currentColor"
            stroke-width="2"
            stroke-linecap="round"
            stroke-linejoin="round"
          >
            <path d="m3 9 9-7 9 7v11a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2z" />
            <polyline points="9 22 9 12 15 12 15 22" />
          </svg>
          Live
        </a>
        <button class="tbtn" id="themeToggle">
          <svg
            width="14"
            height="14"
            viewBox="0 0 24 24"
            fill="none"
            stroke="currentColor"
            stroke-width="2"
            stroke-linecap="round"
          >
            <circle cx="12" cy="12" r="4" />
            <path
              d="M12 2v2M12 20v2M4.9 4.9l1.4 1.4M17.7 17.7l1.4 1.4M2 12h2M20 12h2M4.9 19.1l1.4-1.4M17.7 6.3l1.4-1.4"
            />
          </svg>
          <span id="themeLabel">Light</span>
        </button>
      </div>
    </header>
    <div class="wrap">
      <!-- RANGE CONTROLS -->
      <div class="controls">
        <div class="ctrl-group">
          <label>Quick range</label>
          <div class="range-btns">
            <button class="range-btn" data-range="today">Today</button>
            <button class="range-btn" data-range="24h">Last 24 h</button>
            <button class="range-btn" data-range="7d">Last 7 days</button>
            <button class="range-btn" data-range="30d">Last 30 days</button>
          </div>
        </div>
        <div class="ctrl-group">
          <label>Custom range</label>
            <div class="date-inputs">
                <input type="date" id="fromDate"/>
                <input type="time" id="fromTime" value="00:00"/>
                <span class="ctrl-sep">→</span>
                <input type="date" id="toDate"/>
                <input type="time" id="toTime" value="23:59"/>
            </div>
        </div>
        <button id="loadBtn">Load</button> <span id="statusBar">—</span>
        <span id="errMsg"></span>
      </div>
      <!-- KPI STRIP -->
      <div class="kpi-strip">
        <div class="kpi" style="--ka:var(--amber)">
          <div class="k">PV yield</div>
          <div class="v"><span id="kPV">--</span><small>kWh</small></div>
        </div>
        <div class="kpi" style="--ka:var(--grid-c)">
          <div class="k">Grid import</div>
          <div class="v"><span id="kGrid">--</span><small>kWh</small></div>
        </div>
        <div class="kpi" style="--ka:var(--charge)">
          <div class="k">Batt. charged</div>
          <div class="v"><span id="kBattC">--</span><small>kWh</small></div>
        </div>
        <div class="kpi" style="--ka:var(--discharge)">
          <div class="k">Batt. discharged</div>
          <div class="v"><span id="kBattD">--</span><small>kWh</small></div>
        </div>
        <div class="kpi" style="--ka:var(--purple)">
          <div class="k">Load energy</div>
          <div class="v"><span id="kLoad">--</span><small>kWh</small></div>
        </div>
        <div class="kpi" style="--ka:var(--grid-c)">
          <div class="k">Grid export</div>
          <div class="v"><span id="kExport">--</span><small>kWh</small></div>
        </div>
      </div>
      <!-- POWER CHART (full width) -->
      <div class="card">
        <div class="card-head">
          <div class="card-title">
            <span class="icon" style="--ic:var(--amber)">
              <svg
                viewBox="0 0 24 24"
                fill="none"
                stroke="currentColor"
                stroke-width="2"
                stroke-linecap="round"
                stroke-linejoin="round">
                <path d="M3 12h4l3-9 4 18 3-9h4" />
              </svg>
            </span>
            Power overview
          </div>
          <div class="legend">
            <span><i style="background:#f5a623"></i>PV</span>
            <span><i style="background:#34d399"></i>Battery</span>
            <span><i style="background:#38bdf8"></i>Grid</span>
            <span><i style="background:#fb7185"></i>Load</span>
            <span><i style="background:#818cf8"></i>SOC %</span>
          </div>
        </div>
        <div style="position:relative;width:100%;height:260px">
          <canvas id="chartPwr" role="img" aria-label="Power overview"
            >Power chart</canvas
          >
        </div>
      </div>
      <!-- SOC & DAILY ENERGY -->
      <div class="chart-row">
        <!-- SOC CHART -->
        <div class="card">
          <div class="card-head">
            <div class="card-title">
              <span class="icon" style="--ic:var(--charge)">
                <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                  <rect x="2" y="7" width="16" height="10" rx="2"/>
                  <path d="M22 11v2"/>
                </svg>
              </span>
              Battery SOC (Estimated)
            </div>
          </div>
          <div style="position:relative;width:100%;height:220px">
            <canvas id="chartSOC" role="img" aria-label="Battery SOC">SOC chart</canvas>
          </div>
        </div>
        <!-- DAILY ENERGY BAR -->
        <div class="card">
          <div class="card-head">
            <div class="card-title">
              <span class="icon" style="--ic:var(--amber)">
                <svg
                  viewBox="0 0 24 24"
                  fill="none"
                  stroke="currentColor"
                  stroke-width="2"
                  stroke-linecap="round"
                  stroke-linejoin="round">
                  <rect x="3" y="3" width="18" height="18" rx="2" />
                  <path d="M3 9h18M9 21V9" />
                </svg>
              </span>
              Daily energy (kWh)
            </div>
            <div class="legend">
              <span><i style="background:#f5a623"></i>PV</span>
              <span><i style="background:#38bdf8"></i>Grid</span>
              <span><i style="background:#fb7185"></i>Load</span>
            </div>
          </div>
          <div style="position:relative;width:100%;height:220px">
            <canvas id="chartDaily" role="img" aria-label="Daily energy"
              >Daily chart</canvas
            >
          </div>
        </div>
      </div>
      <div class="footer">
        <a href="./dashboard">← Live dashboard</a> &nbsp;·&nbsp; <span id="dataSource">Data from Oracle Cloud</span> ·
        <span id="loadedAt">not loaded</span>
      </div>
    </div>
    <script src="https://cdnjs.cloudflare.com/ajax/libs/Chart.js/4.4.1/chart.umd.js"></script>
    <script>
      "use strict";
      /* ── CONFIG ─────────────────────────────────────────────────── */
      const CLOUD_API = "https://oracle-cloud-vm.chickenkiller.com";
      const DEVICE_ID = "infinisolar-5kw-01";
      const TOKEN = "c4beebf7478bb73915af4508d6802e152826d9f7fa21bc906e50638495e0cd7e";
      const HEADERS = {
          "Authorization": "Bearer " + TOKEN
      };
      const TZ_OFFSET = 5; // PKT = UTC+5
      let currentTimestamps = [];
      
      let cloudEnabled = null;
      async function checkCloudEnabled() {
          if (cloudEnabled !== null) return cloudEnabled;
          try {
              const r = await fetch("./api/system", { cache: "no-store" });
              const d = await r.json();
              cloudEnabled = !!d.cloudConfigured;
          } catch (e) {
              cloudEnabled = false;   // can't even ask - fall back to local
          }
          return cloudEnabled;
      }
      /* ── THEME ──────────────────────────────────────────────────── */
      (function() {
          const root = document.documentElement;
          const btn = document.getElementById("themeToggle");
          const lbl = document.getElementById("themeLabel");
          let saved;
          try {
              saved = localStorage.getItem("inv-theme");
          } catch (e) {}
          if (saved) root.setAttribute("data-theme", saved);

          function sync() {
              lbl.textContent = root.getAttribute("data-theme") === "dark" ? "Light" : "Dark";
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
      })();
      /* ── HELPERS ────────────────────────────────────────────────── */
      const $ = id => document.getElementById(id);
      const isDark = () => document.documentElement.getAttribute("data-theme") === "dark";
      const gc = () => isDark() ? "#1e2d45" : "#e4eaf5";
      const tc = "#90a2bd";

      function localMidnight(offsetDays = 0) {
          /* Returns ISO string for local midnight (PKT) adjusted by offsetDays */
          const now = new Date();
          // Strip to local date in PKT
          const localMs = now.getTime() + TZ_OFFSET * 3600000;
          const localDay = new Date(Math.floor(localMs / 86400000) * 86400000);
          localDay.setTime(localDay.getTime() + offsetDays * 86400000 - TZ_OFFSET * 3600000);
          return localDay.toISOString().replace(".000Z", "+00:00");
      }

      function nowISO() {
          return new Date().toISOString().replace(".000Z", "+00:00");
      }

      function fmtDate(isoStr, showDate) {
          if (!isoStr) return "";
          const d = new Date(isoStr);
          const pkt = new Date(d.getTime() + TZ_OFFSET * 3600000);
          const iso = pkt.toISOString();
          if (showDate) return iso.substring(5, 10); // MM-DD  for daily/weekly ranges
          return iso.substring(11, 16); // HH:MM  for same-day ranges
      }

      function fmtDateOnly(isoStr) {
          if (!isoStr) return "";
          const d = new Date(isoStr);
          const pkt = new Date(d.getTime() + TZ_OFFSET * 3600000);
          return pkt.toISOString().substring(5, 10); // MM-DD
      }

      function sumField(days, field) {
          return days.reduce((a, d) => a + (d[field] || 0), 0).toFixed(2);
      }

      function toDateInputVal(isoStr) {
          /* Convert ISO to YYYY-MM-DD for date input in local time */
          const d = new Date(isoStr);
          const pkt = new Date(d.getTime() + TZ_OFFSET * 3600000);
          return pkt.toISOString().substring(0, 10);
      }
      function toTimeInputVal(isoStr) {
          const d = new Date(isoStr);
          const pkt = new Date(d.getTime() + TZ_OFFSET * 3600000);
          return pkt.toISOString().substring(11, 16);
      }
      
      /* ── RESOLUTION PICKER ──────────────────────────────────────── */
      function pickResolution(fromISO, toISO) {
          const diffMs = new Date(toISO) - new Date(fromISO);
          const diffHrs = diffMs / 3600000;
          if (diffHrs <= 48) return "1min";
          if (diffHrs <= 240) return "30min";
          return "6hour";
      }
      /* ── CHART SETUP ────────────────────────────────────────────── */
      function baseOpts(yLabel, yMin) {
          return {
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
                      bodyColor: tc,
                      padding: 9,
                      callbacks: {
                          title: function(items) {
                              if (!items.length) return "";
                              const idx = items[0].dataIndex;
                              const ts = currentTimestamps[idx];
                              if (!ts) return items[0].label;
                              const d = new Date(ts);
                              const pkt = new Date(d.getTime() + TZ_OFFSET * 3600000);
                              const iso = pkt.toISOString();
                              return iso.substring(0, 10) + "  " + iso.substring(11, 16);
                          }
                      }
                  }
              },
              scales: {
                  x: {
                      grid: {
                          color: gc(),
                          lineWidth: 0.5
                      },
                      ticks: {
                          color: tc,
                          font: {
                              size: 10
                          },
                          maxTicksLimit: 12,
                          maxRotation: 30
                      },
                      border: {
                          display: false
                      }
                  },
                  y: {
                      grid: {
                          color: gc(),
                          lineWidth: 0.5
                      },
                      ticks: {
                          color: tc,
                          font: {
                              size: 10
                          }
                      },
                      beginAtZero: !yMin,
                      suggestedMin: yMin || undefined,
                      border: {
                          display: false
                      },
                      title: {
                          display: !!yLabel,
                          text: yLabel || "",
                          color: tc,
                          font: {
                              size: 10
                          }
                      }
                  }
              }
          };
      }

      function lineDs(label, color, fill) {
          return {
              label,
              data: [],
              borderColor: color,
              backgroundColor: color + (fill ? "28" : "00"),
              borderWidth: 2,
              pointRadius: 0,
              pointHoverRadius: 4,
              tension: 0.3,
              fill: fill || false
          };
      }

      function barDs(label, color) {
          return {
              label,
              data: [],
              backgroundColor: color + "cc",
              borderColor: color,
              borderWidth: 1,
              borderRadius: 4
          };
      }
      /* Power chart */
      const chartPwr = new Chart($("chartPwr").getContext("2d"), {
          type: "line",
          data: {
              labels: [],
              datasets: [
                  lineDs("PV", "#f5a623"),
                  lineDs("Battery", "#34d399"),
                  lineDs("Grid", "#38bdf8"),
                  lineDs("Load", "#fb7185"),
                  {
                      label: "SOC %",
                      data: [],
                      borderColor: "#818cf8",
                      backgroundColor: "#818cf800",
                      borderWidth: 2,
                      pointRadius: 0,
                      pointHoverRadius: 4,
                      tension: 0.3,
                      yAxisID: "ySoc",
                      fill: false
                  }
              ]
          },
          options: {
              ...baseOpts("W", -3000),
              scales: {
                  ...baseOpts("W", -3000).scales,
                  ySoc: {
                      type: "linear",
                      position: "right",
                      min: 0, max: 100,
                      grid: { drawOnChartArea: false },
                      ticks: { color: "#818cf8", font: { size: 10 }, callback: v => v + "%" },
                      border: { display: false }
                  }
              }
          }
      });
      /* SOC chart */
      const chartSOC = new Chart($("chartSOC").getContext("2d"), {
          type: "line",
          data: { labels: [], datasets: [{ 
              label: "SOC %",
              data: [],
              borderColor: "#818cf8",
              backgroundColor: "#818cf822",
              borderWidth: 2,
              pointRadius: 0,
              pointHoverRadius: 4,
              tension: 0.3,
              fill: true
          }]},
          options: {
              responsive: true, maintainAspectRatio: false, animation: false,
              interaction: { mode: "index", intersect: false },
              plugins: {
                  legend: { display: false },
                  tooltip: {
                      backgroundColor: isDark() ? "#131d31" : "#fff",
                      borderColor: isDark() ? "#24334f" : "#d7e0ee", borderWidth: 1,
                      titleColor: isDark() ? "#e8eef8" : "#17233a",
                      bodyColor: tc, padding: 9,
                      callbacks: {
                          title: function(items) {
                              if (!items.length) return "";
                              const idx = items[0].dataIndex;
                              const ts = currentTimestamps[idx];
                              if (!ts) return items[0].label;
                              const d = new Date(ts);
                              const pkt = new Date(d.getTime() + TZ_OFFSET * 3600000);
                              const iso = pkt.toISOString();
                              return iso.substring(0, 10) + "  " + iso.substring(11, 16);
                          }
                      }
                  }
              },
              scales: {
                  x: { grid: { color: gc(), lineWidth: 0.5 }, ticks: { color: tc, font: { size: 10 }, maxTicksLimit: 12, maxRotation: 30 }, border: { display: false } },
                  y: { grid: { color: gc(), lineWidth: 0.5 }, ticks: { color: tc, font: { size: 10 }, callback: v => v + "%" }, min: 0, max: 100, border: { display: false } }
              }
          }
      });
      /* Daily energy bar chart */
      const chartDaily = new Chart($("chartDaily").getContext("2d"), {
          type: "bar",
          data: {
              labels: [],
              datasets: [
                  barDs("PV", "#f5a623"),
                  barDs("Grid", "#38bdf8"),
                  barDs("Load", "#fb7185")
              ]
          },
          options: {...baseOpts("kWh"),
              plugins: {...baseOpts("kWh").plugins,
                  tooltip: {...baseOpts("kWh").plugins.tooltip,
                      callbacks: {
                          label: ctx => " " + ctx.dataset.label + ": " + ctx.parsed.y.toFixed(2) + " kWh"
                      }
                  }
              },
              scales: {
                  x: {...baseOpts("kWh").scales.x,
                      stacked: false
                  },
                  y: {...baseOpts("kWh").scales.y,
                      stacked: false
                  }
              }
          }
      });
      /* ── DATA LOADING ───────────────────────────────────────────── */
      async function loadData(fromISO, toISO, label) {
          const useCloud = await checkCloudEnabled();
          if (!useCloud) return loadLocalData(fromISO, toISO, label);
          return loadCloudData(fromISO, toISO, label);
      }

      async function loadCloudData(fromISO, toISO, label) {
          const btn = $("loadBtn");
          btn.disabled = true;
          $("statusBar").textContent = "Loading…";
          $("errMsg").classList.remove("show");
          $("dataSource").textContent = "Data from Oracle Cloud";
          // Work out from/to date strings for daily endpoint
          const fromDate = toDateInputVal(fromISO);
          const toDate = toDateInputVal(toISO);
          const res = pickResolution(fromISO, toISO);
          const is24h = (label === "Last 24 hours");
          try {
              /* Fetch power/SOC range data and daily totals in parallel */
              const [rangeResp, dailyResp, last24hResp, rangesumResp] = await Promise.all([
                fetch(CLOUD_API + "/api/telemetry/range?device_id=" + DEVICE_ID +
                      "&from_dt=" + encodeURIComponent(fromISO) +
                      "&to_dt="   + encodeURIComponent(toISO) +
                      "&resolution=" + res, { headers: HEADERS }),
                fetch(CLOUD_API + "/api/telemetry/daily?device_id=" + DEVICE_ID +
                      "&from_date=" + fromDate +
                      "&to_date="   + toDate, { headers: HEADERS }),
                is24h
                    ? fetch(CLOUD_API + "/api/telemetry/last24h?device_id=" + DEVICE_ID, { headers: HEADERS })
                    : Promise.resolve(null),
                fetch(CLOUD_API + "/api/telemetry/rangesum?device_id=" + DEVICE_ID +
                      "&from_dt=" + encodeURIComponent(fromISO) +
                      "&to_dt="   + encodeURIComponent(toISO), { headers: HEADERS })
              ]);
              if (!rangeResp.ok) throw new Error("Range API: HTTP " + rangeResp.status);
              if (!dailyResp.ok) throw new Error("Daily API: HTTP " + dailyResp.status);
              const range = await rangeResp.json();
              const daily = await dailyResp.json();
              const last24h = (is24h && last24hResp) ? await last24hResp.json() : null;
              const rangesum = rangesumResp ? await rangesumResp.json() : null;
              /* ── Power chart ── */
              const diffHrs = (new Date(toISO) - new Date(fromISO)) / 3600000;
              const showDate = diffHrs > 24;
              const pwrLabels = range.timestamps.map(ts => fmtDate(ts, showDate));
              currentTimestamps = range.timestamps;
              chartPwr.data.labels = pwrLabels;
              chartPwr.data.datasets[0].data = range.pv_power;
              chartPwr.data.datasets[1].data = range.battery_power;
              chartPwr.data.datasets[2].data = range.grid_power;
              chartPwr.data.datasets[3].data = range.load_power;
              chartPwr.data.datasets[4].data = range.battery_soc.map(v => v === null ? NaN : v);
              chartPwr.update();
              chartSOC.data.labels = pwrLabels;
              chartSOC.data.datasets[0].data = range.calculated_soc.map(v => v === null ? NaN : v);
              chartSOC.update();
              /* ── Daily energy bar chart ── */
              const days = daily.days;
              if (diffHrs <= 24) {
                  // Sub-day / 24 h range: show one bar from rangesum or last24h
                  const src = (is24h && last24h?.status === "ok") ? last24h
                            : (rangesum?.status  === "ok") ? rangesum
                            : null;
                  chartDaily.data.labels             = src ? [label]                  : [];
                  chartDaily.data.datasets[0].data   = src ? [src.pv_energy_kwh]      : [];
                  chartDaily.data.datasets[1].data   = src ? [src.grid_import_kwh]    : [];
                  chartDaily.data.datasets[2].data   = src ? [src.load_energy_kwh]    : [];
              } else {
                  // Multi-day range: one bar per day (existing behaviour)
                  chartDaily.data.labels             = days.map(d => d.date.substring(5));
                  chartDaily.data.datasets[0].data   = days.map(d => d.pv_energy_kwh);
                  chartDaily.data.datasets[1].data   = days.map(d => d.grid_import_kwh);
                  chartDaily.data.datasets[2].data   = days.map(d => d.load_energy_kwh);
              }
              chartDaily.update();
              /* ── KPI totals ── */
              if (last24h && last24h.status === "ok") {
                  $("kPV").textContent    = last24h.pv_energy_kwh.toFixed(2);
                  $("kGrid").textContent  = last24h.grid_import_kwh.toFixed(2);
                  $("kBattC").textContent = last24h.battery_charge_kwh.toFixed(2);
                  $("kBattD").textContent = last24h.battery_discharge_kwh.toFixed(2);
                  $("kLoad").textContent  = last24h.load_energy_kwh.toFixed(2);
                  $("kExport").textContent= last24h.grid_export_kwh.toFixed(2);
              } else if (rangesum && rangesum.status === "ok") {
                  $("kPV").textContent    = rangesum.pv_energy_kwh.toFixed(2);
                  $("kGrid").textContent  = rangesum.grid_import_kwh.toFixed(2);
                  $("kBattC").textContent = rangesum.battery_charge_kwh.toFixed(2);
                  $("kBattD").textContent = rangesum.battery_discharge_kwh.toFixed(2);
                  $("kLoad").textContent  = rangesum.load_energy_kwh.toFixed(2);
                  $("kExport").textContent= rangesum.grid_export_kwh.toFixed(2);
              } else {
                  $("kPV").textContent    = sumField(days, "pv_energy_kwh");
                  $("kGrid").textContent  = sumField(days, "grid_import_kwh");
                  $("kBattC").textContent = sumField(days, "battery_charge_kwh");
                  $("kBattD").textContent = sumField(days, "battery_discharge_kwh");
                  $("kLoad").textContent  = sumField(days, "load_energy_kwh");
                  $("kExport").textContent= sumField(days, "grid_export_kwh");
              }
              /* ── Status ── */
              $("rangeLabel").textContent = label + " · " + range.count + " points · " + res + " resolution";
              $("statusBar").textContent = "Loaded " + new Date().toLocaleTimeString();
              $("loadedAt").textContent = new Date().toLocaleString();
          } catch (err) {
              $("errMsg").textContent = "Error: " + err.message;
              $("errMsg").classList.add("show");
              $("statusBar").textContent = "Failed";
          }
        btn.disabled = false;
      }
      /* ── LOCAL FALLBACK (no cloud configured) ────────────────────── */
      const LOCAL_MAX_POINTS = 1200; // downsample the chart lines beyond this many raw samples

      function bucketAverage(records, n) {
          // Splits records into n ~equal buckets and averages each numeric
          // field, keeping the bucket's LAST timestamp as its label. Used
          // only to keep the line charts readable over long local ranges -
          // the KPI totals below always sum every raw record, never the
          // downsampled buckets.
          if (records.length <= n) return records;
          const out = [];
          const size = records.length / n;
          for (let b = 0; b < n; b++) {
              const start = Math.floor(b * size);
              const end = Math.floor((b + 1) * size);
              const slice = records.slice(start, end);
              if (!slice.length) continue;
              const avg = (f) => slice.reduce((a, r) => a + (r[f] || 0), 0) / slice.length;
              out.push({
                  recorded_at: slice[slice.length - 1].recorded_at,
                  pv_power: avg("pv_power"), battery_power: avg("battery_power"),
                  grid_power: avg("grid_power"), load_power: avg("load_power"),
                  battery_soc: avg("battery_soc"), calculated_soc: avg("calculated_soc")
              });
          }
          return out;
      }

      async function loadLocalData(fromISO, toISO, label) {
          const btn = $("loadBtn");
          btn.disabled = true;
          $("statusBar").textContent = "Loading (local)…";
          $("errMsg").classList.remove("show");
          $("dataSource").textContent = "Data from on-device flash (cloud not configured)";
          try {
              const fromEpoch = Math.floor(new Date(fromISO).getTime() / 1000);
              const toEpoch   = Math.floor(new Date(toISO).getTime()   / 1000);
              const resp = await fetch("./api/history/local?from=" + fromEpoch + "&to=" + toEpoch);
              if (!resp.ok) throw new Error("Local history: HTTP " + resp.status);
              const records = await resp.json();

              const diffHrs = (new Date(toISO) - new Date(fromISO)) / 3600000;
              const showDate = diffHrs > 24;

              /* ── Power + SOC charts (downsampled if long range) ── */
              const plotRecords = bucketAverage(records, LOCAL_MAX_POINTS);
              currentTimestamps = plotRecords.map(r => r.recorded_at);
              const pwrLabels = plotRecords.map(r => fmtDate(r.recorded_at, showDate));
              chartPwr.data.labels = pwrLabels;
              chartPwr.data.datasets[0].data = plotRecords.map(r => r.pv_power);
              chartPwr.data.datasets[1].data = plotRecords.map(r => r.battery_power);
              chartPwr.data.datasets[2].data = plotRecords.map(r => r.grid_power);
              chartPwr.data.datasets[3].data = plotRecords.map(r => r.load_power);
              chartPwr.data.datasets[4].data = plotRecords.map(r => r.battery_soc);
              chartPwr.update();
              chartSOC.data.labels = pwrLabels;
              chartSOC.data.datasets[0].data = plotRecords.map(r => r.calculated_soc);
              chartSOC.update();

              /* ── KPI totals - always summed from every raw record, never the downsampled buckets ── */
              const sum = (f) => records.reduce((a, r) => a + (r[f] || 0), 0);
              const kpi = {
                  pv: sum("pv_energy_kwh"), grid: sum("grid_import_kwh"),
                  battC: sum("battery_charge_kwh"), battD: sum("battery_discharge_kwh"),
                  load: sum("load_energy_kwh"), gridExp: sum("grid_export_kwh")
              };
              $("kPV").textContent     = kpi.pv.toFixed(2);
              $("kGrid").textContent   = kpi.grid.toFixed(2);
              $("kBattC").textContent  = kpi.battC.toFixed(2);
              $("kBattD").textContent  = kpi.battD.toFixed(2);
              $("kLoad").textContent   = kpi.load.toFixed(2);
              $("kExport").textContent = kpi.gridExp.toFixed(2);

              /* ── Daily energy bar chart ── */
              if (diffHrs <= 24) {
                  chartDaily.data.labels           = records.length ? [label] : [];
                  chartDaily.data.datasets[0].data = records.length ? [kpi.pv]   : [];
                  chartDaily.data.datasets[1].data = records.length ? [kpi.grid] : [];
                  chartDaily.data.datasets[2].data = records.length ? [kpi.load] : [];
              } else {
                  const byDay = {};
                  for (const r of records) {
                      const day = fmtDateOnly(r.recorded_at);
                      if (!byDay[day]) byDay[day] = { pv: 0, grid: 0, load: 0 };
                      byDay[day].pv   += r.pv_energy_kwh    || 0;
                      byDay[day].grid += r.grid_import_kwh  || 0;
                      byDay[day].load += r.load_energy_kwh  || 0;
                  }
                  const dayKeys = Object.keys(byDay).sort();
                  chartDaily.data.labels           = dayKeys;
                  chartDaily.data.datasets[0].data = dayKeys.map(d => byDay[d].pv);
                  chartDaily.data.datasets[1].data = dayKeys.map(d => byDay[d].grid);
                  chartDaily.data.datasets[2].data = dayKeys.map(d => byDay[d].load);
              }
              chartDaily.update();

              /* ── Status ── */
              $("rangeLabel").textContent = label + " · " + records.length + " points (local, no cloud)";
              $("statusBar").textContent = "Loaded " + new Date().toLocaleTimeString();
              $("loadedAt").textContent = new Date().toLocaleString();
          } catch (err) {
              $("errMsg").textContent = "Error: " + err.message;
              $("errMsg").classList.add("show");
              $("statusBar").textContent = "Failed";
          }
          btn.disabled = false;
      }
      /* ── RANGE BUTTONS ──────────────────────────────────────────── */
      const RANGES = {
          today: () => ({
              from: localMidnight(0),
              to: nowISO(),
              label: "Today (midnight–now)"
          }),
          "24h": () => ({
              from: new Date(Date.now() - 86400000).toISOString(),
              to: nowISO(),
              label: "Last 24 hours"
          }),
          "7d": () => ({
              from: localMidnight(-6),
              to: nowISO(),
              label: "Last 7 days"
          }),
          "30d": () => ({
              from: localMidnight(-29),
              to: nowISO(),
              label: "Last 30 days"
          }),
      };
      document.querySelectorAll(".range-btn").forEach(btn => {
          btn.addEventListener("click", function() {
              document.querySelectorAll(".range-btn").forEach(b => b.classList.remove("active"));
              this.classList.add("active");
              const r = RANGES[this.dataset.range]();
              /* Sync date and time inputs */
              $("fromDate").value = toDateInputVal(r.from);
              $("toDate").value   = toDateInputVal(r.to);
              $("fromTime").value = toTimeInputVal(r.from);
              $("toTime").value   = toTimeInputVal(r.to);
              loadData(r.from, r.to, r.label);
          });
      });
      /* ── LOAD BUTTON (custom range) ─────────────────────────────── */
      $("loadBtn").addEventListener("click", function () {
          const from     = $("fromDate").value;
          const to       = $("toDate").value;
          const fromTime = $("fromTime").value || "00:00";
          const toTime   = $("toTime").value   || "23:59";
          if (!from || !to) { alert("Please select both dates."); return; }
          document.querySelectorAll(".range-btn").forEach(b => b.classList.remove("active"));
          const fromISO = from + "T" + fromTime + ":00+05:00";
          const toISO   = to   + "T" + toTime   + ":59+05:00";
          loadData(fromISO, toISO, from + " " + fromTime + " → " + to + " " + toTime);
      });
      /* ── INITIALISE date inputs to today ─────────────────────────── */
      (function() {
          const today = toDateInputVal(nowISO());
          $("fromDate").value = today;
          $("toDate").value = today;
      })();
      let autoTimer = null;

      function startAutoRefresh() {
          clearInterval(autoTimer);
          autoTimer = setInterval(() => {
          const activeBtn = document.querySelector(".range-btn.active");
          if (activeBtn) {
              activeBtn.click();
          } else {
              const from     = $("fromDate").value;
              const to       = $("toDate").value;
              const fromTime = $("fromTime").value || "00:00";
              const toTime   = $("toTime").value   || "23:59";
              if (from && to) {
                  loadData(
                      from + "T" + fromTime + ":00+05:00",
                      to   + "T" + toTime   + ":59+05:00",
                      from + " " + fromTime + " → " + to + " " + toTime
                  );
              }
            }
          }, 60000);
      }
      /* ── AUTO-LOAD today on page open ───────────────────────────── */
      document.querySelector('[data-range="today"]').click();
      /* ── Auto Refresh Page ───────────────────────────────────────── */
      startAutoRefresh();
    </script>
  </body>
</html>
)rawliteral";

