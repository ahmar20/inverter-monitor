#pragma once
// ============================================================================
// page_settings.h — the authenticated /settings schedule-editor page.
// Pure HTML/CSS/JS served as-is by web_server.h. No C++ logic here.
// ============================================================================
#include <Arduino.h>

const char settings_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en" data-theme="dark">
  <head>
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width,initial-scale=1" />
    <title>Inverter – Settings</title>
    <style>
       :root {
          --font-ui: system-ui, -apple-system, "Segoe UI", Roboto, sans-serif;
          --font-num: ui-monospace, "SFMono-Regular", "JetBrains Mono", monospace;
          --amber: #f5a623;
          --charge: #34d399;
          --discharge: #fb7185;
          --gridc: #38bdf8;
          --purple: #818cf8;
          --r: 14px;
          --g: 14px;
          --ease: cubic-bezier(.22, 1, .36, 1);
      }

      html[data-theme="dark"] {
          --bg: #0b1220;
          --sf: #131d31;
          --sf2: #182545;
          --bd: #24334f;
          --tx: #e8eef8;
          --mu: #90a2bd;
          --sh: 0 8px 24px -12px rgba(0, 0, 0, .75);
          --glass-bg-strong: rgba(19, 29, 49, .90);
          --glass-bg-light: rgba(19, 29, 49, .45);
      }

      html[data-theme="light"] {
          --bg: #eef2f8;
          --sf: #fff;
          --sf2: #f4f7fc;
          --bd: #d7e0ee;
          --tx: #17233a;
          --mu: #5c6b83;
          --sh: 0 8px 24px -16px rgba(30, 50, 90, .3);
          --glass-bg-strong: rgba(255, 255, 255, .94);
          --glass-bg-light: rgba(255, 255, 255, .45);
      }

      * {
          box-sizing: border-box;
          margin: 0;
          padding: 0
      }

      body {
          font-family: var(--font-ui);
          background: var(--bg);
          color: var(--tx);
          min-height: 100vh;
          -webkit-font-smoothing: antialiased;
          transition: background .3s, color .3s;
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
          padding: 16px 16px 52px
      }

      header {
          display: flex;
          flex-wrap: wrap;
          align-items: center;
          justify-content: space-between;
          gap: 10px;
          margin-bottom: 0;
          /* ── Sticky glass bar ── */
          position: sticky;
          top: 0;
          z-index: 100;
          background: var(--glass-bg-light);
          backdrop-filter: blur(20px);
          -webkit-backdrop-filter: blur(20px);
          border-bottom: 1px solid var(--bd);
          padding: 12px 16px;
          transition: background .4s var(--ease);
      }

      .brand {
          display: flex;
          align-items: center;
          gap: 11px
      }

      .logo {
          width: 40px;
          height: 40px;
          border-radius: 11px;
          display: grid;
          place-items: center;
          background: linear-gradient(145deg, var(--amber), #d97706);
          color: #1a1206;
          flex-shrink: 0;
          box-shadow: 0 5px 14px -5px rgba(245, 166, 35, .6)
      }

      .logo svg {
          width: 20px;
          height: 20px
      }

      .brand h1 {
          font-size: 16px;
          font-weight: 700
      }

      .brand p {
          font-size: 11.5px;
          color: var(--mu)
      }

      .hc {
          display: flex;
          align-items: center;
          gap: 7px;
          flex-wrap: wrap
      }

      .pill {
          display: inline-flex;
          align-items: center;
          gap: 7px;
          padding: 5px 12px;
          border-radius: 999px;
          background: var(--sf);
          border: 1px solid var(--bd);
          font-size: 12px;
          font-weight: 600;
          box-shadow: var(--sh)
      }

      .dot {
          width: 8px;
          height: 8px;
          border-radius: 50%;
          background: var(--mu);
          flex-shrink: 0;
          position: relative
      }

      .dot.on {
          background: var(--charge)
      }

      .dot.on::after {
          content: "";
          position: absolute;
          inset: -4px;
          border-radius: 50%;
          background: var(--charge);
          opacity: .35;
          animation: ping 1.6s var(--ease) infinite
      }

      .dot.off {
          background: var(--discharge)
      }

      @keyframes ping {
          0% {
              transform: scale(1);
              opacity: .35
          }
          75%,
          100% {
              transform: scale(2.4);
              opacity: 0
          }
      }

      .tbtn {
          font-family: var(--font-ui);
          display: inline-flex;
          align-items: center;
          gap: 5px;
          padding: 6px 11px;
          border-radius: 9px;
          background: var(--sf2);
          color: var(--tx);
          border: 1px solid var(--bd);
          font-size: 12.5px;
          font-weight: 600;
          cursor: pointer;
          text-decoration: none;
          white-space: nowrap;
          transition: border-color .2s
      }

      .tbtn:hover {
          border-color: var(--amber)
      }
      /* ============================================================
       UNSAVED CHANGES BANNER
       ============================================================ */

      #unsavedBanner {
          display: none;
          align-items: center;
          justify-content: space-between;
          gap: 12px;
          background: color-mix(in srgb, var(--amber) 12%, var(--sf));
          border: 1px solid color-mix(in srgb, var(--amber) 45%, transparent);
          color: var(--amber);
          border-radius: 10px;
          padding: 10px 13px;
          margin-bottom: var(--g);
          font-size: 12.5px;
          font-weight: 600;
          animation: bannerIn .25s var(--ease)
      }

      #unsavedBanner.show {
          display: flex
      }

      .unsaved-left {
          display: flex;
          align-items: center;
          gap: 8px;
          min-width: 0
      }

      .unsaved-left svg {
          flex-shrink: 0
      }

      .unsaved-text {
          overflow: hidden;
          text-overflow: ellipsis
      }

      .unsaved-hint {
          font-size: 11px;
          font-weight: 500;
          color: var(--mu);
          white-space: nowrap
      }

      @keyframes bannerIn {
          from {
              opacity: 0;
              transform: translateY(-5px)
          }
          to {
              opacity: 1;
              transform: none
          }
      }
      /* ============================================================
       SECTIONS
       ============================================================ */

      .section {
          background: var(--sf);
          border: 1px solid var(--bd);
          border-radius: var(--r);
          padding: 18px;
          box-shadow: var(--sh);
          margin-bottom: var(--g)
      }

      .sec-head {
          display: flex;
          align-items: center;
          gap: 10px;
          margin-bottom: 16px;
          padding-bottom: 13px;
          border-bottom: 1px solid var(--bd)
      }

      .sec-icon {
          width: 32px;
          height: 32px;
          border-radius: 9px;
          display: grid;
          place-items: center;
          background: color-mix(in srgb, var(--ic, var(--amber)) 15%, transparent);
          color: var(--ic, var(--amber));
          flex-shrink: 0
      }

      .sec-icon svg {
          width: 17px;
          height: 17px
      }

      .sec-title {
          font-size: 15px;
          font-weight: 700
      }

      .sec-sub {
          font-size: 12px;
          color: var(--mu);
          margin-top: 1px
      }

      .ctrl-grid {
          display: grid;
          grid-template-columns: repeat(auto-fill, minmax(240px, 1fr));
          gap: 12px
      }

      .ctrl-box {
          background: var(--sf2);
          border-radius: 10px;
          padding: 13px 14px;
          transition: opacity .3s, border-color .2s;
          border: 1px solid transparent
      }

      .ctrl-box.dirty {
          border-color: color-mix(in srgb, var(--amber) 50%, transparent);
      }

      .ctrl-box.locked {
          opacity: .45;
          pointer-events: none
      }

      .ctrl-label {
          font-size: 10.5px;
          color: var(--mu);
          text-transform: uppercase;
          letter-spacing: .06em;
          font-weight: 600;
          margin-bottom: 3px
      }

      .ctrl-current {
          font-family: var(--font-num);
          font-size: 12.5px;
          color: var(--amber);
          font-weight: 600;
          margin-bottom: 9px;
          min-height: 16px
      }

      .ctrl-row {
          display: flex;
          align-items: center;
          gap: 7px;
          flex-wrap: wrap
      }

      .ctrl-select,
      .ctrl-input {
          font-family: var(--font-ui);
          font-size: 13px;
          font-weight: 600;
          background: var(--sf);
          color: var(--tx);
          border: 1px solid var(--bd);
          border-radius: 8px;
          padding: 7px 9px;
          flex: 1;
          min-width: 0;
          transition: border-color .2s
      }

      .ctrl-select:hover,
      .ctrl-input:hover,
      .ctrl-select:focus,
      .ctrl-input:focus {
          border-color: var(--amber);
          outline: none
      }

      .ctrl-unit {
          font-size: 12px;
          color: var(--mu);
          font-weight: 600;
          flex-shrink: 0
      }

      .ctrl-btn {
          font-family: var(--font-ui);
          font-size: 12px;
          font-weight: 700;
          padding: 7px 15px;
          border-radius: 8px;
          border: none;
          cursor: pointer;
          background: var(--amber);
          color: #1a1206;
          box-shadow: 0 3px 8px -3px rgba(245, 166, 35, .5);
          transition: opacity .2s;
          white-space: nowrap;
          flex-shrink: 0
      }

      .ctrl-btn:hover {
          opacity: .85
      }

      .ctrl-btn:disabled {
          opacity: .4;
          cursor: not-allowed
      }

      .ctrl-fb {
          font-size: 11px;
          font-weight: 600;
          padding: 5px 9px;
          border-radius: 6px;
          margin-top: 7px;
          display: none
      }

      .ctrl-fb.ok {
          display: block;
          background: color-mix(in srgb, var(--charge) 14%, transparent);
          color: var(--charge)
      }

      .ctrl-fb.err {
          display: block;
          background: color-mix(in srgb, var(--discharge) 14%, transparent);
          color: var(--discharge)
      }

      .ctrl-fb.busy {
          display: block;
          background: color-mix(in srgb, var(--amber) 14%, transparent);
          color: var(--amber)
      }

      .warn-banner {
          background: color-mix(in srgb, var(--amber) 12%, transparent);
          border: 1px solid color-mix(in srgb, var(--amber) 35%, transparent);
          border-radius: 9px;
          padding: 10px 13px;
          font-size: 12.5px;
          color: var(--amber);
          display: flex;
          align-items: flex-start;
          gap: 8px;
          margin-bottom: 13px
      }

      .warn-banner svg {
          flex-shrink: 0;
          margin-top: 1px
      }

      .flags-grid {
          display: flex;
          flex-wrap: wrap;
          gap: 8px
      }

      .flag-pill {
          display: inline-flex;
          align-items: center;
          gap: 7px;
          padding: 7px 13px;
          border-radius: 999px;
          font-size: 12.5px;
          font-weight: 600;
          border: 1px solid var(--bd);
          background: var(--sf2);
          cursor: pointer;
          transition: all .2s;
          user-select: none;
          -webkit-tap-highlight-color: transparent
      }

      .flag-pill:hover {
          border-color: var(--amber)
      }

      .flag-pill.on {
          border-color: color-mix(in srgb, var(--charge) 45%, transparent);
          background: color-mix(in srgb, var(--charge) 10%, transparent)
      }

      .flag-pill.sending {
          opacity: .6;
          cursor: wait
      }

      .fdot {
          width: 7px;
          height: 7px;
          border-radius: 50%;
          flex-shrink: 0;
          transition: background .2s
      }

      .flag-pill.on .fdot {
          background: var(--charge)
      }

      .flag-pill.off .fdot {
          background: var(--mu)
      }

      .time-row {
          display: flex;
          align-items: center;
          gap: 8px;
          flex-wrap: wrap
      }

      .time-row input[type="date"],
      .time-row input[type="time"] {
          font-family: var(--font-ui);
          font-size: 13px;
          font-weight: 600;
          background: var(--sf);
          color: var(--tx);
          border: 1px solid var(--bd);
          border-radius: 8px;
          padding: 7px 9px;
          width: auto;
          transition: border-color .2s
      }

      .time-row input:hover,
      .time-row input:focus {
          border-color: var(--amber);
          outline: none
      }

      .time-sep {
          font-size: 13px;
          color: var(--mu);
          font-weight: 600
      }

      .hour-select {
          font-family: var(--font-ui);
          font-size: 13px;
          font-weight: 600;
          background: var(--sf);
          color: var(--tx);
          border: 1px solid var(--bd);
          border-radius: 8px;
          padding: 7px 9px;
          width: 110px;
          transition: border-color .2s;
          cursor: pointer
      }

      .hour-select:hover,
      .hour-select:focus {
          border-color: var(--amber);
          outline: none
      }

      .footer {
          margin-top: 24px;
          text-align: center;
          font-size: 12px;
          color: var(--mu)
      }

      .footer a {
          color: var(--amber);
          text-decoration: none
      }

            @media(max-width:600px) {
          .ctrl-grid {
              grid-template-columns: 1fr
          }
          #unsavedBanner {
              align-items: flex-start;
              flex-direction: column
          }
          .unsaved-hint {
              white-space: normal
          }
      }

      /* ── Schedules ── */
      .sched-item {
          display: flex;
          align-items: center;
          gap: 10px;
          padding: 10px 12px;
          background: var(--sf2);
          border-radius: 9px;
          margin-bottom: 6px;
          border: 1px solid var(--bd)
      }
      .sched-item-info { flex: 1; min-width: 0 }
      .sched-item-name { font-size: 14px; font-weight: 600 }
      .sched-item-meta { font-size: 11px; color: var(--mu); margin-top: 2px }
      .sched-tog {
          background: none;
          border: 1px solid var(--bd);
          border-radius: 6px;
          padding: 3px 9px;
          cursor: pointer;
          font-size: 12px;
          font-weight: 700;
          transition: color .2s, border-color .2s
      }
      .sched-tog.on  { color: var(--charge); border-color: color-mix(in srgb,var(--charge) 40%,transparent) }
      .sched-tog.off { color: var(--mu) }
      .sched-del {
          background: color-mix(in srgb, var(--discharge) 12%, transparent);
          color: var(--discharge);
          border-color: color-mix(in srgb, var(--discharge) 30%, transparent)
      }
      .sched-run {
          background: color-mix(in srgb, var(--charge) 14%, transparent);
          color: var(--charge);
          border-color: color-mix(in srgb, var(--charge) 35%, transparent)
      }
      .sched-run:disabled { opacity: .45; cursor: default }
      .sched-last-run { font-size: 10px; color: var(--mu); margin-top: 3px }

      /* modal */
      .sched-modal-wrap {
          display: none;
          position: fixed;
          inset: 0;
          background: rgba(0,0,0,.65);
          z-index: 999;
          overflow-y: auto
      }
      .sched-modal-center {
          min-height: 100%;
          display: flex;
          align-items: flex-start;
          justify-content: center;
          padding: 24px 12px
      }
      .sched-modal-box {
          background: var(--sf);
          border-radius: 14px;
          padding: 22px;
          width: min(560px, 100%);
          box-shadow: 0 20px 60px rgba(0,0,0,.5)
      }
      .sched-modal-head {
          display: flex;
          align-items: center;
          justify-content: space-between;
          margin-bottom: 16px;
          border-bottom: 1px solid var(--bd);
          padding-bottom: 12px
      }
      .sched-modal-title { font-size: 15px; font-weight: 700 }
      .sched-modal-close {
          background: none;
          border: none;
          color: var(--mu);
          font-size: 20px;
          cursor: pointer;
          line-height: 1
      }
      .sched-modal-footer {
          display: flex;
          justify-content: flex-end;
          gap: 10px;
          margin-top: 16px
      }
      .sched-cancel {
          background: var(--sf2);
          color: var(--tx);
          border: 1px solid var(--bd);
          box-shadow: none
      }
      .day-pills { display: flex; gap: 5px; flex-wrap: wrap; margin-bottom: 14px }
      .day-pill {
          padding: 4px 9px;
          border-radius: 6px;
          cursor: pointer;
          border: 1px solid var(--bd);
          font-size: 12px;
          font-weight: 600;
          background: transparent;
          color: var(--mu);
          transition: background .15s, color .15s
      }
      .day-pill.on { background: var(--charge); color: #0b1a11; border-color: var(--charge) }
      .cmd-row {
          display: flex;
          align-items: center;
          gap: 7px;
          margin-bottom: 8px;
          flex-wrap: wrap
      }
      .cmd-setting-sel { flex: 0 0 168px; min-width: 0 }
      .cmd-val-area { display: flex; align-items: center; gap: 6px; flex: 1; flex-wrap: wrap; min-width: 0 }
      .cmd-remove {
          background: none;
          border: none;
          color: var(--mu);
          font-size: 18px;
          cursor: pointer;
          padding: 0 3px;
          flex-shrink: 0
      }
    </style>
  </head>

  <body>
    <header>
      <div class="brand">
        <div class="logo">
          <svg
            viewBox="0 0 24 24"
            fill="none"
            stroke="currentColor"
            stroke-width="2.2"
            stroke-linecap="round"
            stroke-linejoin="round"
          >
            <path
              d="M12.22 2h-.44a2 2 0 0 0-2 2v.18a2 2 0 0 1-1 1.73l-.43.25a2 2 0 0 1-2 0l-.15-.08a2 2 0 0 0-2.73.73l-.22.38a2 2 0 0 0 .73 2.73l.15.1a2 2 0 0 1 1 1.72v.51a2 2 0 0 1-1 1.74l-.15.09a2 2 0 0 0-.73 2.73l.22.38a2 2 0 0 0 2.73.73l.15-.08a2 2 0 0 1 2 0l.43.25a2 2 0 0 1 1 1.73V20a2 2 0 0 0 2 2h.44a2 2 0 0 0 2-2v-.18a2 2 0 0 1 1-1.73l.43-.25a2 2 0 0 1 2 0l.15.08a2 2 0 0 0 2.73-.73l.22-.39a2 2 0 0 0-.73-2.73l-.15-.08a2 2 0 0 1-1-1.74v-.5a2 2 0 0 1 1-1.74l.15-.09a2 2 0 0 0 .73-2.73l-.22-.38a2 2 0 0 0-2.73-.73l-.15.08a2 2 0 0 1-2 0l-.43-.25a2 2 0 0 1-1-1.73V4a2 2 0 0 0-2-2z"
            />
            <circle cx="12" cy="12" r="3" />
          </svg>
        </div>
        <div>
          <h1>InfiniSolar VII 5KW – Settings</h1>
          <p>Updated <span id="lastUpdate">—</span></p>
        </div>
      </div>
      <div class="hc">
        <div class="pill">
          <span id="sysDot" class="dot"></span>
          <span id="sysStatus">Connecting…</span>
        </div>
        <a class="tbtn" href="./dashboard">
          <svg
            width="13"
            height="13"
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
            width="13"
            height="13"
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
      <!-- ============================================================
     UNSAVED CHANGES BANNER
     ============================================================ -->
      <div id="unsavedBanner">
        <div class="unsaved-left">
          <svg
            width="16"
            height="16"
            viewBox="0 0 24 24"
            fill="none"
            stroke="currentColor"
            stroke-width="2"
            stroke-linecap="round"
            stroke-linejoin="round"
          >
            <path d="M12 9v4" />
            <path d="M12 17h.01" />
            <path
              d="M10.3 3.9 1.8 18a2 2 0 0 0 1.7 3h17a2 2 0 0 0 1.7-3L13.7 3.9a2 2 0 0 0-3.4 0z"
            />
          </svg>
          <span class="unsaved-text">
            Unsaved changes — automatic refresh is paused
          </span>
        </div>
        <span class="unsaved-hint">
          Press Set / Apply to save your change
        </span>
      </div>
      <!-- ============================================================
     SECTION 1: SOURCE & PRIORITY
     ============================================================ -->
      <div class="section">
        <div class="sec-head">
          <span class="sec-icon" style="--ic:var(--amber)">
            <svg
              viewBox="0 0 24 24"
              fill="none"
              stroke="currentColor"
              stroke-width="2"
              stroke-linecap="round"
              stroke-linejoin="round"
            >
              <path d="M13 2 3 14h7l-1 8 10-12h-7l1-8z" />
            </svg>
          </span>
          <div>
            <div class="sec-title">Source &amp; Priority</div>
            <div class="sec-sub">Programs 01, 05, 10</div>
          </div>
        </div>
        <div class="ctrl-grid">
          <div class="ctrl-box" id="boxPOP">
            <div class="ctrl-label">Output Source Priority (Prog 01)</div>
            <div class="ctrl-current" id="curPOP">—</div>
            <div class="ctrl-row">
              <select class="ctrl-select" id="ctrlPOP">
                <option value="0">SUB – Solar first, then Utility</option>
                <option value="1">SBU – Solar → Battery → Grid</option>
              </select>
              <button
                class="ctrl-btn"
                onclick="sendSelect('POP','ctrlPOP','fbPOP','POP')"
              >
                Set
              </button>
            </div>
            <div class="ctrl-fb" id="fbPOP"></div>
          </div>
          <div class="ctrl-box" id="boxPSP">
            <div class="ctrl-label">Solar Supply Priority (Prog 05)</div>
            <div class="ctrl-current" id="curPSP">—</div>
            <div class="ctrl-row">
              <select class="ctrl-select" id="ctrlPSP">
                <option value="0">BLU – Charge battery first</option>
                <option value="1">LBU – Supply load first</option>
              </select>
              <button
                class="ctrl-btn"
                onclick="sendSelect('PSP','ctrlPSP','fbPSP','PSP')">
                Set
              </button>
            </div>
            <div class="ctrl-fb" id="fbPSP"></div>
          </div>
          <div class="ctrl-box" id="boxPCP">
            <div class="ctrl-label">Charger Source Priority (Prog 10)</div>
            <div class="ctrl-current" id="curPCP">—</div>
            <div class="ctrl-row">
              <select
                class="ctrl-select"
                id="ctrlPCP"
              >
                <option value="0">CSO – Solar first</option>
                <option value="1">SNU – Solar + Utility</option>
                <option value="2">OSO – Solar only</option>
              </select>
              <button
                class="ctrl-btn"
                onclick="sendSelect('PCP','ctrlPCP','fbPCP','PCP');updateACTimerVisibility()"
              >
                Set
              </button>
            </div>
            <div class="ctrl-fb" id="fbPCP"></div>
          </div>
        </div>
      </div>
      <!-- ============================================================
     SECTION 2: CHARGE CURRENT
     ============================================================ -->
      <div class="section">
        <div class="sec-head">
          <span class="sec-icon" style="--ic:var(--charge)">
            <svg
              viewBox="0 0 24 24"
              fill="none"
              stroke="currentColor"
              stroke-width="2"
              stroke-linecap="round"
              stroke-linejoin="round"
            >
              <path d="M5 12h14M12 5l7 7-7 7" />
            </svg>
          </span>
          <div>
            <div class="sec-title">Charge Current Limits</div>
            <div class="sec-sub">Programs 11, 13</div>
          </div>
        </div>
        <div class="ctrl-grid">
          <div class="ctrl-box" id="boxMCHGC">
            <div class="ctrl-label">Max Total Charge Current (Prog 11)</div>
            <div class="ctrl-current" id="curMCHGC">—</div>
            <div class="ctrl-row">
              <select class="ctrl-select" id="ctrlMCHGC">
                <option value="010">10 A</option>
                <option value="020">20 A</option>
                <option value="030">30 A</option>
                <option value="040">40 A</option>
                <option value="050">50 A</option>
                <option value="060">60 A</option>
                <option value="070">70 A</option>
                <option value="080">80 A</option>
                <option value="090">90 A</option>
                <option value="100">100 A</option>
              </select>
              <button
                class="ctrl-btn"
                onclick="sendSelect('MCHGC','ctrlMCHGC','fbMCHGC','MCHGC')"
              >
                Set
              </button>
            </div>
            <div class="ctrl-fb" id="fbMCHGC"></div>
          </div>
          <div class="ctrl-box" id="boxMUCHGC">
            <div class="ctrl-label">Max AC Charge Current (Prog 13)</div>
            <div class="ctrl-current" id="curMUCHGC">—</div>
            <div class="ctrl-row">
              <select class="ctrl-select" id="ctrlMUCHGC">
                <option value="002">2 A</option>
                <option value="010">10 A</option>
                <option value="020">20 A</option>
                <option value="030">30 A</option>
                <option value="040">40 A</option>
                <option value="050">50 A</option>
                <option value="060">60 A</option>
                <option value="070">70 A</option>
                <option value="080">80 A</option>
                <option value="090">90 A</option>
                <option value="100">100 A</option>
              </select>
              <button
                class="ctrl-btn"
                onclick="sendSelect('MUCHGC','ctrlMUCHGC','fbMUCHGC','MUCHGC')"
              >
                Set
              </button>
            </div>
            <div class="ctrl-fb" id="fbMUCHGC"></div>
          </div>
        </div>
      </div>
      <!-- ============================================================
     SECTION 3: BATTERY
     ============================================================ -->
      <div class="section">
        <div class="sec-head">
          <span class="sec-icon" style="--ic:var(--charge)">
            <svg
              viewBox="0 0 24 24"
              fill="none"
              stroke="currentColor"
              stroke-width="2"
              stroke-linecap="round"
              stroke-linejoin="round"
            >
              <rect x="2" y="7" width="16" height="10" rx="2" />
              <path d="M22 11v2M6 11v2M10 10v4" />
            </svg>
          </span>
          <div>
            <div class="sec-title">Battery Settings</div>
            <div class="sec-sub">Programs 14, 17, 18, 19, 20, 21</div>
          </div>
        </div>
        <div class="ctrl-grid" style="margin-bottom:12px">
          <div class="ctrl-box" id="boxPBT">
            <div class="ctrl-label">Battery Type (Prog 14)</div>
            <div class="ctrl-current" id="curPBT">—</div>
            <div class="ctrl-row">
              <select
                class="ctrl-select"
                id="ctrlPBT"
                onchange="settingChanged('PBT');updateBattTypeVisibility()"
              >
                <option value="0">AGM</option>
                <option value="1">Flooded</option>
                <option value="2">User-Defined</option>
              </select>
              <button
                class="ctrl-btn"
                onclick="sendSelect('PBT','ctrlPBT','fbPBT','PBT');setTimeout(updateBattTypeVisibility,200)"
              >
                Set
              </button>
            </div>
            <div class="ctrl-fb" id="fbPBT"></div>
          </div>
        </div>
        <div class="warn-banner" id="userVoltWarn">
          <svg
            width="15"
            height="15"
            viewBox="0 0 24 24"
            fill="none"
            stroke="currentColor"
            stroke-width="2"
            stroke-linecap="round"
            stroke-linejoin="round"
          >
            <path
              d="M10.3 3.9 1.8 18a2 2 0 0 0 1.7 3h17a2 2 0 0 0 1.7-3L13.7 3.9a2 2 0 0 0-3.4 0z"
            />
            <path d="M12 9v4M12 17h.01" />
          </svg>
          Programs 17, 18 and 19 are only editable when Battery Type is set to
          <strong>&nbsp;User-Defined</strong>.
        </div>
        <div class="ctrl-grid">
          <div class="ctrl-box" id="boxBulk">
            <div class="ctrl-label">Bulk Charge Voltage (Prog 17)</div>
            <div class="ctrl-current" id="curBulk">—</div>
            <div class="ctrl-row">
              <input
                class="ctrl-input"
                type="number"
                id="ctrlBulk"
                step="0.1"
                min="48.0"
                max="64.0"
                placeholder="e.g. 56.4"
              />
              <span class="ctrl-unit">V</span>
              <button class="ctrl-btn" onclick="sendMCHGV()">Set</button>
            </div>
            <div class="ctrl-fb" id="fbMCHGV"></div>
          </div>
          <div class="ctrl-box" id="boxFloat">
            <div class="ctrl-label">Float Charge Voltage (Prog 18)</div>
            <div class="ctrl-current" id="curFloat">—</div>
            <div class="ctrl-row">
              <input
                class="ctrl-input"
                type="number"
                id="ctrlFloat"
                step="0.1"
                min="48.0"
                max="60.0"
                placeholder="e.g. 54.0"
              />
              <span class="ctrl-unit">V</span>
              <button class="ctrl-btn" onclick="sendMCHGV()">Set</button>
            </div>
          </div>
          <div class="ctrl-box" id="boxCutoff">
            <div class="ctrl-label">Low DC Cutoff Voltage (Prog 19)</div>
            <div class="ctrl-current" id="curPSDV">—</div>
            <div class="ctrl-row">
              <input
                class="ctrl-input"
                type="number"
                id="ctrlPSDV"
                step="0.1"
                min="40.8"
                max="48.0"
                placeholder="e.g. 40.8"
              />
              <span class="ctrl-unit">V</span>
              <button
                class="ctrl-btn"
                onclick="sendVoltage('PSDV','ctrlPSDV','fbPSDV','PSDV')"
              >
                Set
              </button>
            </div>
            <div class="ctrl-fb" id="fbPSDV"></div>
          </div>
          <div class="ctrl-box" id="boxStopDisch">
            <div class="ctrl-label">
              Stop Discharging Voltage – Grid (Prog 20)
            </div>
            <div class="ctrl-current" id="curStopDisch">—</div>
            <div class="ctrl-row">
              <select class="ctrl-select" id="ctrlStopDisch">
                <option value="440">44.0 V</option>
                <option value="450">45.0 V</option>
                <option value="460">46.0 V</option>
                <option value="470">47.0 V</option>
                <option value="480">48.0 V</option>
                <option value="490">49.0 V</option>
                <option value="500">50.0 V</option>
                <option value="510">51.0 V</option>
              </select>
              <button class="ctrl-btn" onclick="sendBUCD()">Set</button>
            </div>
            <div class="ctrl-fb" id="fbBUCD"></div>
          </div>
          <div class="ctrl-box" id="boxStopChg">
            <div class="ctrl-label">Stop Charging Voltage – Grid (Prog 21)</div>
            <div class="ctrl-current" id="curStopChg">—</div>
            <div class="ctrl-row">
              <select class="ctrl-select" id="ctrlStopChg">
                <option value="480">48.0 V</option>
                <option value="490">49.0 V</option>
                <option value="500">50.0 V</option>
                <option value="510">51.0 V</option>
                <option value="520">52.0 V</option>
                <option value="530">53.0 V</option>
                <option value="540">54.0 V</option>
                <option value="550">55.0 V</option>
                <option value="560">56.0 V</option>
                <option value="570">57.0 V</option>
                <option value="580">58.0 V</option>
              </select>
              <button class="ctrl-btn" onclick="sendBUCD()">Set</button>
            </div>
          </div>
        </div>
      </div>
      <!-- ============================================================
     SECTION 4: AC INPUT / OUTPUT
     ============================================================ -->
      <div class="section">
        <div class="sec-head">
          <span class="sec-icon" style="--ic:var(--gridc)">
            <svg
              viewBox="0 0 24 24"
              fill="none"
              stroke="currentColor"
              stroke-width="2"
              stroke-linecap="round"
              stroke-linejoin="round"
            >
              <path d="M12 2v20M2 12h20M4.9 4.9l14.2 14.2M19.1 4.9 4.9 19.1" />
            </svg>
          </span>
          <div>
            <div class="sec-title">AC Input / Output</div>
            <div class="sec-sub">Programs 02, 03, 04</div>
          </div>
        </div>
        <div class="ctrl-grid">
          <div class="ctrl-box" id="boxPGR">
            <div class="ctrl-label">AC Input Voltage Range (Prog 02)</div>
            <div class="ctrl-current" id="curPGR">—</div>
            <div class="ctrl-row">
              <select class="ctrl-select" id="ctrlPGR">
                <option value="0">Appliance – 90~280 VAC</option>
                <option value="1">UPS – 170~280 VAC</option>
              </select>
              <button
                class="ctrl-btn"
                onclick="sendSelect('PGR','ctrlPGR','fbPGR','PGR')"
              >
                Set
              </button>
            </div>
            <div class="ctrl-fb" id="fbPGR"></div>
          </div>
          <div class="ctrl-box" id="boxV">
            <div class="ctrl-label">Output Voltage (Prog 03)</div>
            <div class="ctrl-current" id="curV">—</div>
            <div class="ctrl-row">
              <select class="ctrl-select" id="ctrlV">
                <option value="220">220 VAC</option>
                <option value="230">230 VAC</option>
                <option value="240">240 VAC</option>
              </select>
              <button
                class="ctrl-btn"
                onclick="sendRaw('V',document.getElementById('ctrlV').value,'fbV','V')"
              >
                Set
              </button>
            </div>
            <div class="ctrl-fb" id="fbV"></div>
          </div>
          <div class="ctrl-box" id="boxF">
            <div class="ctrl-label">Output Frequency (Prog 04)</div>
            <div class="ctrl-current" id="curF">—</div>
            <div class="ctrl-row">
              <select class="ctrl-select" id="ctrlF">
                <option value="50">50 Hz</option>
                <option value="60">60 Hz</option>
              </select>
              <button
                class="ctrl-btn"
                onclick="sendRaw('F',document.getElementById('ctrlF').value,'fbF','F')"
              >
                Set
              </button>
            </div>
            <div class="ctrl-fb" id="fbF"></div>
          </div>
        </div>
      </div>
      <!-- ============================================================
     SECTION 5: AC TIMERS
     ============================================================ -->
      <div class="section">
        <div class="sec-head">
          <span class="sec-icon" style="--ic:var(--purple)">
            <svg
              viewBox="0 0 24 24"
              fill="none"
              stroke="currentColor"
              stroke-width="2"
              stroke-linecap="round"
              stroke-linejoin="round"
            >
              <circle cx="12" cy="12" r="10" />
              <polyline points="12 6 12 12 16 14" />
            </svg>
          </span>
          <div>
            <div class="sec-title">AC Timers</div>
            <div class="sec-sub">
              Programs 30–31 (charge) · 32–33 (load output)
            </div>
          </div>
        </div>
        <div id="acChargeTimerWarnDiv" style="display:none">
          <div class="warn-banner">
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
              <path
                d="M10.3 3.9 1.8 18a2 2 0 0 0 1.7 3h17a2 2 0 0 0 1.7-3L13.7 3.9a2 2 0 0 0-3.4 0z"
              />
              <path d="M12 9v4M12 17h.01" />
            </svg>
            AC charge timer has no effect when Charger Priority is Solar only
            (OSO).
          </div>
        </div>
        <div
          style="font-size:12px;color:var(--mu);margin-bottom:8px;font-weight:600"
        >
          AC Charge Timer (Prog 30–31)
        </div>
        <div
          class="ctrl-box"
          id="boxACCT"
          style="max-width:420px;margin-bottom:14px"
        >
          <div class="ctrl-label">Start time → Stop time</div>
          <div class="ctrl-current" id="curACCT">—</div>
          <div class="time-row">
            <select class="hour-select" id="ctrlACCTstart">
              <option value="00">00:00</option>
              <option value="01">01:00</option>
              <option value="02">02:00</option>
              <option value="03">03:00</option>
              <option value="04">04:00</option>
              <option value="05">05:00</option>
              <option value="06">06:00</option>
              <option value="07">07:00</option>
              <option value="08">08:00</option>
              <option value="09">09:00</option>
              <option value="10">10:00</option>
              <option value="11">11:00</option>
              <option value="12">12:00</option>
              <option value="13">13:00</option>
              <option value="14">14:00</option>
              <option value="15">15:00</option>
              <option value="16">16:00</option>
              <option value="17">17:00</option>
              <option value="18">18:00</option>
              <option value="19">19:00</option>
              <option value="20">20:00</option>
              <option value="21">21:00</option>
              <option value="22">22:00</option>
              <option value="23">23:00</option>
            </select>
            <span class="time-sep">→</span>
            <select class="hour-select" id="ctrlACCTstop">
              <option value="00">00:00</option>
              <option value="01">01:00</option>
              <option value="02">02:00</option>
              <option value="03">03:00</option>
              <option value="04">04:00</option>
              <option value="05">05:00</option>
              <option value="06">06:00</option>
              <option value="07">07:00</option>
              <option value="08">08:00</option>
              <option value="09">09:00</option>
              <option value="10">10:00</option>
              <option value="11">11:00</option>
              <option value="12">12:00</option>
              <option value="13">13:00</option>
              <option value="14">14:00</option>
              <option value="15">15:00</option>
              <option value="16">16:00</option>
              <option value="17">17:00</option>
              <option value="18">18:00</option>
              <option value="19">19:00</option>
              <option value="20">20:00</option>
              <option value="21">21:00</option>
              <option value="22">22:00</option>
              <option value="23">23:00</option>
            </select>
            <button class="ctrl-btn" onclick="sendACCT()">Set</button>
          </div>
          <div class="ctrl-fb" id="fbACCT"></div>
        </div>
        <div
          style="font-size:12px;color:var(--mu);margin-bottom:8px;font-weight:600"
        >
          AC Load Output Timer (Prog 32–33)
        </div>
        <div class="ctrl-box" id="boxACLT" style="max-width:420px">
          <div class="ctrl-label">Output on time → Output off time</div>
          <div class="ctrl-current" id="curACLT">—</div>
          <div class="time-row">
            <select class="hour-select" id="ctrlACLTstart">
              <option value="00">00:00</option>
              <option value="01">01:00</option>
              <option value="02">02:00</option>
              <option value="03">03:00</option>
              <option value="04">04:00</option>
              <option value="05">05:00</option>
              <option value="06">06:00</option>
              <option value="07">07:00</option>
              <option value="08">08:00</option>
              <option value="09">09:00</option>
              <option value="10">10:00</option>
              <option value="11">11:00</option>
              <option value="12">12:00</option>
              <option value="13">13:00</option>
              <option value="14">14:00</option>
              <option value="15">15:00</option>
              <option value="16">16:00</option>
              <option value="17">17:00</option>
              <option value="18">18:00</option>
              <option value="19">19:00</option>
              <option value="20">20:00</option>
              <option value="21">21:00</option>
              <option value="22">22:00</option>
              <option value="23">23:00</option>
            </select>
            <span class="time-sep">→</span>
            <select class="hour-select" id="ctrlACLTstop">
              <option value="00">00:00</option>
              <option value="01">01:00</option>
              <option value="02">02:00</option>
              <option value="03">03:00</option>
              <option value="04">04:00</option>
              <option value="05">05:00</option>
              <option value="06">06:00</option>
              <option value="07">07:00</option>
              <option value="08">08:00</option>
              <option value="09">09:00</option>
              <option value="10">10:00</option>
              <option value="11">11:00</option>
              <option value="12">12:00</option>
              <option value="13">13:00</option>
              <option value="14">14:00</option>
              <option value="15">15:00</option>
              <option value="16">16:00</option>
              <option value="17">17:00</option>
              <option value="18">18:00</option>
              <option value="19">19:00</option>
              <option value="20">20:00</option>
              <option value="21">21:00</option>
              <option value="22">22:00</option>
              <option value="23">23:00</option>
            </select>
            <button class="ctrl-btn" onclick="sendACLT()">Set</button>
          </div>
          <div class="ctrl-fb" id="fbACLT"></div>
        </div>
      </div>
      <!-- ============================================================
     SECTION 6: FLAGS
     ============================================================ -->
      <div class="section">
        <div class="sec-head">
          <span class="sec-icon" style="--ic:var(--amber)">
            <svg
              viewBox="0 0 24 24"
              fill="none"
              stroke="currentColor"
              stroke-width="2"
              stroke-linecap="round"
              stroke-linejoin="round"
            >
              <path
                d="M4 15s1-1 4-1 5 2 8 2 4-1 4-1V3s-1 1-4 1-5-2-8-2-4 1-4 1z"
              />
              <line x1="4" y1="22" x2="4" y2="15" />
            </svg>
          </span>
          <div>
            <div class="sec-title">Feature Flags</div>
            <div class="sec-sub">Tap to toggle on / off instantly</div>
          </div>
        </div>
        <div class="flags-grid" id="flagsGrid">
          <span style="color:var(--mu);font-size:13px">
            Waiting for data…
          </span>
        </div>
        <div class="ctrl-fb" id="fbFlag" style="margin-top:10px"></div>
      </div>
      <!-- ============================================================
     SECTION 7: DATE & TIME
     ============================================================ -->
      <div class="section">
        <div class="sec-head">
          <span class="sec-icon" style="--ic:var(--purple)">
            <svg
              viewBox="0 0 24 24"
              fill="none"
              stroke="currentColor"
              stroke-width="2"
              stroke-linecap="round"
              stroke-linejoin="round"
            >
              <rect x="3" y="4" width="18" height="18" rx="2" />
              <line x1="16" y1="2" x2="16" y2="6" />
              <line x1="8" y1="2" x2="8" y2="6" />
              <line x1="3" y1="10" x2="21" y2="10" />
            </svg>
          </span>
          <div>
            <div class="sec-title">Date &amp; Time</div>
            <div class="sec-sub">Programs 95–99 · Syncs inverter clock</div>
          </div>
        </div>
        <div class="ctrl-box" id="boxDAT" style="max-width:460px">
          <div class="ctrl-label">Inverter clock</div>
          <div class="ctrl-current" id="curDAT">—</div>
          <div class="time-row" style="margin-bottom:8px">
            <input type="date" id="ctrlDate" />
            <input type="time" id="ctrlTime" />
            <button class="ctrl-btn" onclick="sendDAT()">Set</button>
          </div>
          <div style="font-size:11px;color:var(--mu)">
            Sets the inverter's internal real-time clock to the selected date
            and time.
          </div>
          <div class="ctrl-fb" id="fbDAT"></div>
        </div>
      </div>

      <!-- ============================================================
         SECTION 8: AUTOMATED SCHEDULES
         ============================================================ -->
      <div class="section">
        <div class="sec-head">
          <span class="sec-icon" style="--ic:var(--charge)">
            <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
              <circle cx="12" cy="12" r="10"/>
              <polyline points="12 6 12 12 16 14"/>
            </svg>
          </span>
          <div>
            <div class="sec-title">Automated Schedules</div>
            <div class="sec-sub">Fire inverter settings at set times</div>
          </div>
          <button class="ctrl-btn" style="margin-left:auto;flex-shrink:0" onclick="openSchedModal(-1)">+ Add</button>
        </div>
        <div id="scheduleList">
          <span style="color:var(--mu);font-size:13px">Loading…</span>
        </div>
        <div class="ctrl-fb" id="fbSched" style="margin-top:10px"></div>
      </div>
      <!-- ============================================================
         SECTION 9: CLOUD UPLOAD
         ============================================================ -->
      <div class="section">
        <div class="sec-head">
          <span class="sec-icon" style="--ic:var(--gridc)">
            <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
              <path d="M18 10h-1.26A8 8 0 1 0 9 20h9a5 5 0 0 0 0-10z"/>
            </svg>
          </span>
          <div>
            <div class="sec-title">Cloud Upload</div>
            <div class="sec-sub" id="cloudSub">—</div>
          </div>
        </div>
        <div class="ctrl-box" id="boxCloud" style="max-width:520px">
          <div class="ctrl-label">Backend URL</div>
          <div class="ctrl-row" style="margin-bottom:10px">
            <input class="ctrl-input" type="text" id="ctrlCloudUrl" placeholder="https://your-backend.example.com/api/telemetry">
          </div>
          <div class="ctrl-label">API Key</div>
          <div class="ctrl-row">
            <input class="ctrl-input" type="password" id="ctrlCloudKey" placeholder="Required to save">
            <button class="ctrl-btn" onclick="sendCloudConfig()">Save</button>
          </div>
          <div class="ctrl-fb" id="fbCloud"></div>
          <div style="margin-top:10px">
            <a href="#" style="font-size:11px;color:var(--mu)" onclick="disableCloudConfig();return false">Disable cloud upload</a>
          </div>
        </div>
      </div>
    
      <!-- SCHEDULE MODAL -->
      <div class="sched-modal-wrap" id="schedModal">
        <div class="sched-modal-center">
          <div class="sched-modal-box">
            <div class="sched-modal-head">
              <span class="sched-modal-title" id="schedModalTitle">Add Schedule</span>
              <button class="sched-modal-close" onclick="closeSchedModal()">✕</button>
            </div>
            <div class="ctrl-label" style="margin-bottom:4px">Label</div>
            <input type="text" id="schedLabel" class="ctrl-input" placeholder="e.g. Morning" style="width:100%;margin-bottom:14px">
            <div class="ctrl-label" style="margin-bottom:4px">Fire at</div>
            <input type="time" id="schedTime" class="ctrl-input" style="width:140px;margin-bottom:14px">
            <div class="ctrl-label" style="margin-bottom:6px">Days</div>
            <div class="day-pills" id="schedDayPills"></div>
            <div class="ctrl-label" style="margin-bottom:8px">Settings to apply</div>
            <div id="schedCmdsContainer" style="margin-bottom:12px"></div>
            <div class="ctrl-fb" id="fbSchedModal" style="margin-bottom:10px"></div>
            <div class="sched-modal-footer">
              <button class="ctrl-btn sched-cancel" onclick="closeSchedModal()">Cancel</button>
              <button class="ctrl-btn" onclick="saveSchedModal()">Save Schedule</button>
            </div>
          </div>
        </div>
      </div>

      <div class="footer">
        <a href="./dashboard">← Live dashboard</a> &nbsp;&middot;&nbsp;
        <a href="history">History</a>
      </div>
    </div>
    <script>
      "use strict";
      /* ============================================================
         THEME
         ============================================================ */
      (function() {
          const root = document.documentElement;
          const btn = document.getElementById("themeToggle");
          const lbl = document.getElementById("themeLabel");
          let saved;
          try {
              saved = localStorage.getItem("inv-theme");
          } catch (e) {}
          if (saved) {
              root.setAttribute("data-theme", saved);
          }

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
      /* ============================================================
         HELPERS
         ============================================================ */
      const $ = id => document.getElementById(id);

      function setText(id, v) {
          const el = $(id);
          if (el && v != null) {
              el.textContent = v;
          }
      }

      function setStatus(text, online) {
          setText("sysStatus", text);
          const d = $("sysDot");
          if (d) {
              d.className = "dot" + (online ? " on" : " off");
          }
      }

      function showFb(id, type, msg) {
          const el = $(id);
          if (!el) return;
          el.className = "ctrl-fb " + type;
          el.textContent = msg;
          if (type === "ok") {
              setTimeout(function() {
                  el.className = "ctrl-fb";
              }, 5000);
          }
      }
      /* ============================================================
         UNSAVED / POLLING CONTROL
         ============================================================ */
      /*
         Each dirty key represents one setting or a group of settings.

         Examples:

         POP       = Output Source Priority
         PCP       = Charger Priority
         MCHGV     = Bulk + Float together
         BUCD      = Stop Discharge + Stop Charge together
         ACCT      = AC charge timer
         ACLT      = AC load timer
      */
      const dirtySettings = new Set();
      let refreshTimer = null;
      let refreshInterval = 5000;
      let requestInProgress = false;

      function isDirty() {
          return dirtySettings.size > 0;
      }

      function updateUnsavedBanner() {
          const banner = $("unsavedBanner");
          if (!banner) return;
          banner.classList.toggle("show", isDirty());
      }

      function pausePolling() {
          if (refreshTimer !== null) {
              clearInterval(refreshTimer);
              refreshTimer = null;
          }
      }

      function resumePolling() {
          if (isDirty()) {
              return;
          }
          if (refreshTimer !== null) {
              clearInterval(refreshTimer);
          }
          refreshTimer = setInterval(fetchData, refreshInterval);
      }

      function settingChanged(key) {
          dirtySettings.add(key);
          updateUnsavedBanner();
          pausePolling();
          /*
             Visually mark the relevant control box.
          */
          const map = {
              POP: "boxPOP",
              PSP: "boxPSP",
              PCP: "boxPCP",
              MCHGC: "boxMCHGC",
              MUCHGC: "boxMUCHGC",
              PBT: "boxPBT",
              MCHGV: ["boxBulk", "boxFloat"],
              PSDV: "boxCutoff",
              BUCD: ["boxStopDisch", "boxStopChg"],
              PGR: "boxPGR",
              V: "boxV",
              F: "boxF",
              ACCT: "boxACCT",
              ACLT: "boxACLT",
              DAT: "boxDAT",
          };
          const boxes = Array.isArray(map[key]) ? map[key] : [map[key]];
          boxes.forEach(function(id) {
              const box = $(id);
              if (box) {
                  box.classList.add("dirty");
              }
          });
      }

      function clearDirty(key) {
          dirtySettings.delete(key);
          if (key.startsWith("FLAG_")) {
              updateUnsavedBanner();
              if (!isDirty()) {
                  setTimeout(function() { fetchData(); resumePolling(); }, 3000);
              }
              return;
          }
          const map = {
              POP: "boxPOP",
              PSP: "boxPSP",
              PCP: "boxPCP",
              MCHGC: "boxMCHGC",
              MUCHGC: "boxMUCHGC",
              PBT: "boxPBT",
              MCHGV: ["boxBulk", "boxFloat"],
              PSDV: "boxCutoff",
              BUCD: ["boxStopDisch", "boxStopChg"],
              PGR: "boxPGR",
              V: "boxV",
              F: "boxF",
              ACCT: "boxACCT",
              ACLT: "boxACLT",
              DAT: "boxDAT",
          };
          const boxes = Array.isArray(map[key]) ? map[key] : [map[key]];
          boxes.forEach(function(id) {
              const box = $(id);
              if (box) {
                  box.classList.remove("dirty");
              }
          });
          updateUnsavedBanner();
          if (!isDirty()) {
              /*
                 Give the inverter a moment to process the command
                 before asking for fresh data.
              */
              setTimeout(function() {
                  fetchData();
                  resumePolling();
              }, 3000);
          }
      }

      /* ============================================================
         COMMAND BUILD
         ============================================================ */
        function buildCmd(cmd, value) {
          let param;
          if (cmd === "PCP") {
              // PCAP confirmed: PCP0,1 format (program 0, value)
              param = "0," + value;
          } else if (cmd === "MCHGC") {
              // PCAP confirmed: MCHGC0,020 (program 0, 3-digit amps)
              param = "0," + String(parseInt(value)).padStart(3, "0");
          } else if (cmd === "MUCHGC") {
              // PCAP confirmed: MUCHGC0,002
              param = "0," + String(parseInt(value)).padStart(3, "0");
          } else if (cmd === "MCHGV") {
              // format: bulk*10,float*10 e.g. 572,560
              param = value.replace("-", ",");
          } else if (cmd === "BUCD") {
              // PCAP confirmed: BUCD450,560 (comma separator)
              param = value.replace("-", ",");
          } else if (cmd === "V") {
              // Output voltage is volts*10, 4-digit, matching PIRI field2 encoding
              param = String(parseInt(value) * 10).padStart(4, "0");
          //} else if (cmd === "F") {
              // Output frequency is Hz*10, 3-digit, matching PIRI field3 encoding
              //param = String(parseInt(value) * 10).padStart(3, "0");
          } else if (cmd === "ACCT" || cmd === "ACLT") {
              // PCAP confirmed: ACCT0001,0100 (comma separator)
              param = value.replace("-", ",");
          } else {
              param = value;
          }
          const body = cmd + param;
          return "^S" + String(body.length + 3).padStart(3, "0") + body;
      }
      /* ============================================================
         COMMAND DISPATCH
         ============================================================ */
      async function dispatch(fullCmd, fbId, dirtyKey) {
          requestInProgress = true;
          pausePolling();
          showFb(fbId, "busy", "Sending…");
          try {
              const res = await fetch("./api/command", {
                  method: "POST",
                  headers: {
                      "Content-Type": "application/json"
                  },
                  body: JSON.stringify({
                      command: fullCmd
                  })
              });
              if (!res.ok) {
                  throw new Error("HTTP " + res.status);
              }
              const d = await res.json();
              if (d.status === "ok") {
                  showFb(fbId, "ok", "✓ " + (d.message || "Saved"));
                  /*
                     Only this setting/group is now considered saved.
                  */
                  clearDirty(dirtyKey);
              } else {
                  showFb(fbId, "err", "✗ " + (d.message || "Command failed"));
                  /*
                     IMPORTANT:
                     Keep the setting dirty.
                     Polling remains paused.
                  */
              }
          } catch (e) {
              console.log("[SETTINGS] command failed:", e);
              showFb(fbId, "err", "✗ Network error");
              /*
                 Keep dirty state.
                 User's value must not be overwritten.
              */
          } finally {
              requestInProgress = false;
          }
      }
      /* ============================================================
         SEND SELECT
         ============================================================ */
      function sendSelect(cmd, sid, fbId, dirtyKey) {
          /*
             If no dirtyKey was explicitly supplied,
             use the command itself.
          */
          dirtyKey = dirtyKey || cmd;
          dispatch(buildCmd(cmd, $(sid).value), fbId, dirtyKey);
      }
      /* ============================================================
         SEND RAW
         ============================================================ */
      function sendRaw(cmd, value, fbId, dirtyKey) {
          dirtyKey = dirtyKey || cmd;
          dispatch(buildCmd(cmd, value), fbId, dirtyKey);
      }
      /* ============================================================
         SEND VOLTAGE
         ============================================================ */
      function sendVoltage(cmd, inputId, fbId, dirtyKey) {
          dirtyKey = dirtyKey || cmd;
          const raw = parseFloat($(inputId).value);
          if (isNaN(raw) || raw <= 0) {
              showFb(fbId, "err", "Enter a valid voltage");
              return;
          }
          dispatch(buildCmd(cmd, String(Math.round(raw * 10)).padStart(3, "0")), fbId, dirtyKey);
      }
      /* ============================================================
         BULK + FLOAT VOLTAGES
         ============================================================ */
      function sendMCHGV() {
          const bulk = parseFloat($("ctrlBulk").value);
          const fl = parseFloat($("ctrlFloat").value);
          if (isNaN(bulk) || isNaN(fl)) {
              showFb("fbMCHGV", "err", "Enter both bulk and float voltages");
              return;
          }
          if (fl >= bulk) {
              showFb("fbMCHGV", "err", "Float must be lower than bulk voltage");
              return;
          }
          dispatch(buildCmd("MCHGV", String(Math.round(bulk * 10)) + "-" + String(Math.round(fl * 10))), "fbMCHGV", "MCHGV");
      }
      /* ============================================================
         STOP DISCHARGE + STOP CHARGE
         ============================================================ */
      function sendBUCD() {
          const disch = $("ctrlStopDisch").value;
          const chg = $("ctrlStopChg").value;
          if (parseInt(disch) >= parseInt(chg)) {
              showFb("fbBUCD", "err", "Stop-discharge must be lower than stop-charge");
              return;
          }
          dispatch(buildCmd("BUCD", disch + "-" + chg), "fbBUCD", "BUCD");
      }
      /* ============================================================
         AC CHARGE TIMER
         ============================================================ */
      function sendACCT() {
          const s = String($("ctrlACCTstart").value).padStart(2, "0") + "00";
          const e = String($("ctrlACCTstop").value).padStart(2, "0") + "00";
          dispatch(buildCmd("ACCT", s + "-" + e), "fbACCT", "ACCT");
      }
      /* ============================================================
         AC LOAD TIMER
         ============================================================ */
      function sendACLT() {
          const s = String($("ctrlACLTstart").value).padStart(2, "0") + "00";
          const e = String($("ctrlACLTstop").value).padStart(2, "0") + "00";
          dispatch(buildCmd("ACLT", s + "-" + e), "fbACLT", "ACLT");
      }
      /* ============================================================
         DATE / TIME
         ============================================================ */
      function sendDAT() {
          const dateVal = $("ctrlDate").value;
          const timeVal = $("ctrlTime").value;
          if (!dateVal || !timeVal) {
              showFb("fbDAT", "err", "Select date and time first");
              return;
          }
          const p = dateVal.split("-");
          const yy = p[0].substring(2);
          const mm = p[1];
          const dd = p[2];
          const parts = timeVal.split(":");
          const hh = parts[0];
          const min = parts[1];
          dispatch(buildCmd("DAT", yy + mm + dd + hh + min + "00"), "fbDAT", "DAT");
      }
      /* ============================================================
         FLAGS
         ============================================================ */
      const FLAG_DEFS = [{
          key: "buzzerEnabled",
          label: "Buzzer",
          letter: "A"
      }, {
          key: "overloadBypass",
          label: "OVL Bypass",
          letter: "B"
      }, {
          key: "lcdTimeout",
          label: "LCD Timeout",
          letter: "C"
      }, {
          key: "overloadRestart",
          label: "OVL Restart",
          letter: "D"
      }, {
          key: "overTempRestart",
          label: "Temp Restart",
          letter: "E"
      }, {
          key: "backlightOn",
          label: "Backlight",
          letter: "F"
      }, {
          key: "alarmPrimaryInput",
          label: "Primary Alarm",
          letter: "G"
      }, {
          key: "faultCodeRecord",
          label: "Fault Record",
          letter: "H"
      }];

      function buildFlagGrid(F) {
          const grid = $("flagsGrid");
          if (!grid || F.buzzerEnabled === undefined) {
              return;
          }
          grid.innerHTML = FLAG_DEFS.map(function(f) {
              const on = !!F[f.key];
              return `
                    <div
                        class="flag-pill ${on ? "on" : "off"}"
                        data-letter="${f.letter}"
                        data-on="${on}"
                        onclick="toggleFlag(this)"
                    >
                        <span class="fdot"></span>
                        ${f.label}
                    </div>
                `;
          }).join("");
      }

      function toggleFlag(pill) {
          if (pill.classList.contains("sending")) {
              return;
          }
          const letter = pill.dataset.letter;
          const currentlyOn = pill.dataset.on === "true";
          const newOn = !currentlyOn;
          /*
             Flags are immediately applied,
             but polling is still paused while
             the command is being processed.
          */
          const dirtyKey = "FLAG_" + letter;
          settingChanged(dirtyKey);
          pill.classList.add("sending");
          showFb("fbFlag", "busy", "Toggling " + pill.textContent.trim() + "…");
          fetch("./api/command", {
              method: "POST",
              headers: {
                  "Content-Type": "application/json"
              },
              body: JSON.stringify({
                  command: buildCmd("P", (newOn ? "E" : "D") + letter)
              })
          }).then(function(r) {
              if (!r.ok) {
                  throw new Error("HTTP " + r.status);
              }
              return r.json();
          }).then(function(d) {
              if (d.status === "ok") {
                  showFb("fbFlag", "ok", "✓ " + pill.textContent.trim() + " " + (newOn ? "enabled" : "disabled"));
                  clearDirty(dirtyKey);
              } else {
                  showFb("fbFlag", "err", "✗ " + (d.message || "Command failed"));
              }
          }).catch(function() {
              showFb("fbFlag", "err", "✗ Network error");
          }).finally(function() {
              pill.classList.remove("sending");
          });
      }
      /* ============================================================
         SMART VISIBILITY
         ============================================================ */
      function updateBattTypeVisibility() {
          const isUser = $("ctrlPBT") && $("ctrlPBT").value === "2";
          $("userVoltWarn").style.display = isUser ? "none" : "flex";
          ["boxBulk", "boxFloat", "boxCutoff"].forEach(function(id) {
              const el = $(id);
              if (el) {
                  el.classList.toggle("locked", !isUser);
              }
          });
      }

      function updateACTimerVisibility() {
          const isSolarOnly = $("ctrlPCP") && $("ctrlPCP").value === "2";
          $("acChargeTimerWarnDiv").style.display = isSolarOnly ? "block" : "none";
      }
      /* ============================================================
         RENDER
         ============================================================ */
      function render(data) {
          const P = data.PIRI || {};
          const M = data.MOD || {};
          const F = data.FLAG || {};
          const AT = data.ACCT || {};
          const AL = data.ACLT || {};
          const G = data.GSX || {};
          if (!P.valid) {
              setStatus("Waiting for inverter", false);
              return;
          }
          setStatus("Connected", true);
          /* ========================================================
             CURRENT VALUES
             ======================================================== */
          setText("curPOP", P.outputSourcePriority || "—");
          setText("curPSP", P.solarPowerPriority || "—");
          setText("curPCP", P.chargerSourcePriority || "—");
          setText("curMCHGC", P.maxTotalChargeCurrent != null ? P.maxTotalChargeCurrent + " A" : "—");
          setText("curMUCHGC", P.maxACChargeCurrent != null ? P.maxACChargeCurrent + " A" : "—");
          setText("curPBT", P.batteryConfigType || "—");
          setText("curBulk", P.battBulkChargingVoltage != null ? Number(P.battBulkChargingVoltage).toFixed(1) + " V" : "—");
          setText("curFloat", P.battFloatVoltage != null ? Number(P.battFloatVoltage).toFixed(1) + " V" : "—");
          setText("curPSDV", P.battCutoffVoltage != null ? Number(P.battCutoffVoltage).toFixed(1) + " V" : "—");
          setText("curStopDisch", P.battStopDischargeVoltageOnGrid != null ? Number(P.battStopDischargeVoltageOnGrid).toFixed(1) + " V" : "—");
          setText("curStopChg", P.battStopChargeVoltageOnGrid != null ? Number(P.battStopChargeVoltageOnGrid).toFixed(1) + " V" : "—");
          setText("curPGR", P.inputVoltageRange || "—");
          setText("curV", P.outputVoltageRating != null ? P.outputVoltageRating + " VAC" : "—");
          setText("curF", P.outputFreqRating != null ? P.outputFreqRating + " Hz" : "—");
          setText("curACCT", AT.startTime && AT.endTime ? AT.startTime + " → " + AT.endTime : "—");
          setText("curACLT", AL.startTime && AL.endTime ? AL.startTime + " → " + AL.endTime : "—");
          setText("curDAT", P.currentDateTime || P.dateTime || P.datetime || "—");
          setText("lastUpdate", new Date().toLocaleTimeString());
          /*
             IMPORTANT:

             Current values are always updated.

             Editable controls are only prefilled when there are
             NO unsaved changes.

             This is the main protection against the problem you
             were experiencing.
          */
          if (!isDirty()) {
              prefill(P, AT, AL);
          }
          buildFlagGrid(F);
          /*
             Do not let live data change visibility based on a
             temporary unsaved selection.
          */
          if (!isDirty()) {
              updateBattTypeVisibility();
              updateACTimerVisibility();
          }
      }
      /* ============================================================
         PREFILL
         ============================================================ */
      function pfs(id, val, encoder) {
          if (val == null) {
              return;
          }
          const el = $(id);
          if (!el) {
              return;
          }
          const v = encoder(val);
          const o = el.querySelector(`option[value="${v}"]`);
          if (o) {
              el.value = v;
          }
      }

      function prefill(P, AT, AL) {
          /*
             IMPORTANT:
             This function is ONLY called when dirtySettings is empty.
          */
          /* Output priority */
          const popEl = $("ctrlPOP");
          if (popEl && P.outputSourcePriority !== undefined) {
              const s = String(P.outputSourcePriority).toLowerCase();
              // Parser produces: "s->u->b"=0, "s->b->u"=1, or raw digit
              if (s.includes("u->b") || s === "0") popEl.value = "0";
              else if (s.includes("b->u") || s === "1") popEl.value = "1";
              else if (s.includes("uti") || s === "2") popEl.value = "2";
          }
          /* Solar priority */
          const pspEl = $("ctrlPSP");
          if (pspEl && P.solarPowerPriority !== undefined) {
              const s = String(P.solarPowerPriority).toLowerCase();
              // Parser produces: "b->l->u"=0, "l->b->u"=1, or raw digit
              if (s.includes("b->l") || s === "0") pspEl.value = "0";
              else if (s.includes("l->b") || s === "1") pspEl.value = "1";
          }
          /* Charger priority */
          const pcpEl = $("ctrlPCP");
          if (pcpEl && P.chargerSourcePriority !== undefined) {
              const s = String(P.chargerSourcePriority).toLowerCase();
              // Parser produces: "solar-first"=0, "solar + grid"=1, "only solar"=2, or raw digit
              if (s.includes("solar-first") || s === "0") pcpEl.value = "0";
              else if (s.includes("solar + grid") || s === "1") pcpEl.value = "1";
              else if (s.includes("only solar") || s === "2") pcpEl.value = "2";
              else if (s === "3") pcpEl.value = "3";
          }
          /* Currents */
          pfs("ctrlMCHGC", P.maxTotalChargeCurrent, v => String(v).padStart(3, "0"));
          pfs("ctrlMUCHGC", P.maxACChargeCurrent, v => String(v).padStart(3, "0"));
          /* Battery type */
          const pbtEl = $("ctrlPBT");
          if (pbtEl && P.batteryConfigType) {
              const s = P.batteryConfigType.toLowerCase();
              if (s.includes("agm")) {
                  pbtEl.value = "0";
              } else if (s.includes("flo")) {
                  pbtEl.value = "1";
              } else if (s.includes("user")) {
                  pbtEl.value = "2";
              }
          }
          /* Voltages */
          if (P.battBulkChargingVoltage != null) {
              $("ctrlBulk").value = Number(P.battBulkChargingVoltage).toFixed(1);
          }
          if (P.battFloatVoltage != null) {
              $("ctrlFloat").value = Number(P.battFloatVoltage).toFixed(1);
          }
          if (P.battCutoffVoltage != null) {
              $("ctrlPSDV").value = Number(P.battCutoffVoltage).toFixed(1);
          }
          pfs("ctrlStopDisch", P.battStopDischargeVoltageOnGrid, v => String(Math.round(v * 10)));
          pfs("ctrlStopChg", P.battStopChargeVoltageOnGrid, v => String(Math.round(v * 10)));
          /* AC range */
          const pgrEl = $("ctrlPGR");
          if (pgrEl && P.inputVoltageRange) {
              const s = P.inputVoltageRange.toLowerCase();
              if (s.includes("apl") || s.includes("app")) {
                  pgrEl.value = "0";
              } else if (s.includes("ups")) {
                  pgrEl.value = "1";
              }
          }
          /* Voltage / frequency */
          pfs("ctrlV", P.outputVoltageRating, v => String(Math.round(v)));
          pfs("ctrlF", P.outputFreqRating, v => String(Math.round(v)));
          /* Timers */
          if (AT.startTime) {
              pfs("ctrlACCTstart", parseInt(AT.startTime.split(":")[0]), v => String(v).padStart(2, "0"));
          }
          if (AT.endTime) {
              pfs("ctrlACCTstop", parseInt(AT.endTime.split(":")[0]), v => String(v).padStart(2, "0"));
          }
          if (AL.startTime) {
              pfs("ctrlACLTstart", parseInt(AL.startTime.split(":")[0]), v => String(v).padStart(2, "0"));
          }
          if (AL.endTime) {
              pfs("ctrlACLTstop", parseInt(AL.endTime.split(":")[0]), v => String(v).padStart(2, "0"));
          }
          /*
             Date/time.

             Only prefill when the user is NOT editing it.
          */
          if (!dirtySettings.has("DAT")) {
              const now = new Date();
              $("ctrlDate").value = now.toISOString().substring(0, 10);
              $("ctrlTime").value = now.toTimeString().substring(0, 5);
          }
      }
      /* ============================================================
         AUTOMATICALLY MARK ALL EDITABLE CONTROLS AS DIRTY
         ============================================================ */
      /*
         This is intentionally event-driven.

         It means if another setting is added to the page later,
         it can simply be added to this list without changing the
         polling logic.
      */
      $("ctrlPOP").addEventListener("change", () => settingChanged("POP"));
      $("ctrlPSP").addEventListener("change", () => settingChanged("PSP"));
      $("ctrlPCP").addEventListener("change", () => { settingChanged("PCP"); updateACTimerVisibility(); });
      $("ctrlMCHGC").addEventListener("change", () => settingChanged("MCHGC"));
      $("ctrlMUCHGC").addEventListener("change", () => settingChanged("MUCHGC"));
      $("ctrlPBT").addEventListener("change", () => settingChanged("PBT"));
      $("ctrlBulk").addEventListener("input", () => settingChanged("MCHGV"));
      $("ctrlFloat").addEventListener("input", () => settingChanged("MCHGV"));
      $("ctrlPSDV").addEventListener("input", () => settingChanged("PSDV"));
      $("ctrlStopDisch").addEventListener("change", () => settingChanged("BUCD"));
      $("ctrlStopChg").addEventListener("change", () => settingChanged("BUCD"));
      $("ctrlPGR").addEventListener("change", () => settingChanged("PGR"));
      $("ctrlV").addEventListener("change", () => settingChanged("V"));
      $("ctrlF").addEventListener("change", () => settingChanged("F"));
      $("ctrlACCTstart").addEventListener("change", () => settingChanged("ACCT"));
      $("ctrlACCTstop").addEventListener("change", () => settingChanged("ACCT"));
      $("ctrlACLTstart").addEventListener("change", () => settingChanged("ACLT"));
      $("ctrlACLTstop").addEventListener("change", () => settingChanged("ACLT"));
      $("ctrlDate").addEventListener("change", () => settingChanged("DAT"));
      $("ctrlTime").addEventListener("change", () => settingChanged("DAT"));
      /* ============================================================
         DATA POLLING
         ============================================================ */
      async function fetchData() {
          /*
             Never start another request while a command is being
             sent.
          */
          if (requestInProgress) {
              return;
          }
          /*
             This second check is important.

             The user can change a setting while an already-running
             fetch request is waiting for the ESP32.

             We therefore don't even start another fetch while dirty.
          */
          if (isDirty()) {
              return;
          }
          try {
              const res = await fetch("./api/data", {
                  cache: "no-store"
              });
              if (!res.ok) {
                  throw new Error("HTTP " + res.status);
              }
              const data = await res.json();
              /*
                 The user might have changed a setting while the
                 request was in progress.

                 DO NOT render/prefill in that situation.
              */
              if (isDirty()) {
                  return;
              }
              render(data);
          } catch (e) {
              setStatus("Connection issue", false);
              console.log("[SETTINGS] /api/data failed:", e);
          }
      }
      /* ============================================================
         START POLLING
         ============================================================ */
      function startPolling() {
          if (refreshTimer !== null) {
              clearInterval(refreshTimer);
          }
          refreshTimer = setInterval(fetchData, refreshInterval);
      }
      fetchData();
      startPolling();

      /* ============================================================
         CLOUD UPLOAD
         ============================================================ */
      async function fetchCloudConfig() {
          try {
              const res = await fetch("./api/cloud/config", { cache: "no-store" });
              if (!res.ok) throw new Error("HTTP " + res.status);
              const d = await res.json();
              $("cloudSub").textContent = d.configured
                  ? "Configured — uploading enabled"
                  : "Not configured — samples are queued locally only, nothing is sent";
              if (document.activeElement !== $("ctrlCloudUrl")) {
                  $("ctrlCloudUrl").value = d.url || "";
              }
          } catch (e) {
              $("cloudSub").textContent = "Could not load status";
          }
      }
      async function sendCloudConfig() {
          const url = $("ctrlCloudUrl").value.trim();
          const key = $("ctrlCloudKey").value;
          if (!url || !key) {
              showFb("fbCloud", "err", "Enter both URL and key, or use Disable instead");
              return;
          }
          showFb("fbCloud", "busy", "Saving…");
          try {
              const res = await fetch("./api/cloud/config", {
                  method: "POST",
                  headers: { "Content-Type": "application/json" },
                  body: JSON.stringify({ url: url, key: key })
              });
              const d = await res.json();
              if (res.ok && d.status === "ok") {
                  showFb("fbCloud", "ok", "✓ Saved");
                  $("ctrlCloudKey").value = "";
                  fetchCloudConfig();
              } else {
                  showFb("fbCloud", "err", "✗ " + (d.message || "Failed"));
              }
          } catch (e) {
              showFb("fbCloud", "err", "✗ Network error");
          }
      }
      async function disableCloudConfig() {
          if (!confirm("Disable cloud upload? Samples will keep queuing locally but nothing will be sent until credentials are re-added.")) return;
          showFb("fbCloud", "busy", "Disabling…");
          try {
              const res = await fetch("./api/cloud/config", {
                  method: "POST",
                  headers: { "Content-Type": "application/json" },
                  body: JSON.stringify({ url: "", key: "" })
              });
              const d = await res.json();
              if (res.ok && d.status === "ok") {
                  showFb("fbCloud", "ok", "✓ Disabled");
                  $("ctrlCloudUrl").value = "";
                  $("ctrlCloudKey").value = "";
                  fetchCloudConfig();
              } else {
                  showFb("fbCloud", "err", "✗ " + (d.message || "Failed"));
              }
          } catch (e) {
              showFb("fbCloud", "err", "✗ Network error");
          }
      }
      fetchCloudConfig();

      /* ============================================================
         SCHEDULES
         ============================================================ */
      // Setting definitions — mirrors C++ buildScheduledCmd().
      // Types:
      //   select  → single dropdown; stored value = option value string
      //   volt1   → single number input (step 0.1); stored as raw ×10 int string e.g. "440"
      //   volt2v  → two number inputs for MCHGV; stored as "bulk×10,float×10" e.g. "572,560"
      //   select2 → two dropdowns for BUCD; stored as "val_a,val_b" e.g. "440,520"
      //   hour2   → two hour dropdowns (00–23) for ACCT/ACLT; stored as "HHmm,HHmm"
      //   flag    → flag letter + action selects; stored as action+letter e.g. "EA","DA"
      const SCHED_DEFS = [
          {cmd:"POP",   label:"Output Priority",       type:"select",
           opts:[["0","SUB (Solar→Utility→Bat)"],["1","SBU (Solar→Bat→Utility)"]]},
          {cmd:"PCP",   label:"Charger Priority",      type:"select",
           opts:[["0","CSO (Solar-first)"],["1","SNU (Solar+Grid)"],["2","OSO (Only Solar)"]]},
          {cmd:"PSP",   label:"Solar Supply Priority", type:"select",
           opts:[["0","BLU (Bat→Load→Utility)"],["1","LBU (Load→Bat→Utility)"]]},
          {cmd:"PBT",   label:"Battery Type",          type:"select",
           opts:[["0","AGM"],["1","Flooded"],["2","User-defined"]]},
          {cmd:"PGR",   label:"Grid Voltage Range",    type:"select",
           opts:[["0","Appliance (90–280V)"],["1","UPS (170–280V)"]]},
          {cmd:"V",     label:"Output Voltage",        type:"select",
           opts:[["220","220 VAC"],["230","230 VAC"],["240","240 VAC"]]},
          {cmd:"F",     label:"Output Frequency",      type:"select",
           opts:[["50","50 Hz"],["60","60 Hz"]]},
          {cmd:"MCHGC", label:"Max Total Charge",      type:"select",
           opts:[["010","10 A"],["020","20 A"],["030","30 A"],["040","40 A"],["050","50 A"],
                 ["060","60 A"],["070","70 A"],["080","80 A"],["090","90 A"],["100","100 A"]]},
          {cmd:"MUCHGC",label:"Max AC Charge",         type:"select",
           opts:[["002","2 A"],["010","10 A"],["020","20 A"],["030","30 A"],["040","40 A"],
                 ["050","50 A"],["060","60 A"],["070","70 A"],["080","80 A"],["090","90 A"],["100","100 A"]]},
          {cmd:"PSDV",  label:"Cutoff Voltage",        type:"volt1",
           min:40.8, max:48.0},
          {cmd:"BUCD",  label:"Grid Off / On Voltage", type:"select2",
           labels:["Grid Off","Grid On"],
           opts_a:[["440","44.0 V"],["450","45.0 V"],["460","46.0 V"],["470","47.0 V"],
                   ["480","48.0 V"],["490","49.0 V"],["500","50.0 V"],["510","51.0 V"]],
           opts_b:[["480","48.0 V"],["490","49.0 V"],["500","50.0 V"],["510","51.0 V"],
                   ["520","52.0 V"],["530","53.0 V"],["540","54.0 V"],["550","55.0 V"],
                   ["560","56.0 V"],["570","57.0 V"],["580","58.0 V"]]},
          {cmd:"MCHGV", label:"Bulk / Float Voltage",  type:"volt2v",
           labels:["Bulk","Float"], min_a:48.0, max_a:64.0, min_b:48.0, max_b:60.0},
          {cmd:"ACCT",  label:"AC Charge Timer",       type:"hour2",
           labels:["Start","End"]},
          {cmd:"ACLT",  label:"AC Load Timer",         type:"hour2",
           labels:["Start","End"]},
          {cmd:"FLAG_A", label:"Flag: Buzzer",         type:"flag_toggle"},
          {cmd:"FLAG_B", label:"Flag: OVL Bypass",     type:"flag_toggle"},
          {cmd:"FLAG_C", label:"Flag: LCD Timeout",    type:"flag_toggle"},
          {cmd:"FLAG_D", label:"Flag: OVL Restart",    type:"flag_toggle"},
          {cmd:"FLAG_E", label:"Flag: Temp Restart",   type:"flag_toggle"},
          {cmd:"FLAG_F", label:"Flag: Backlight",      type:"flag_toggle"},
          {cmd:"FLAG_G", label:"Flag: Primary Alarm",  type:"flag_toggle"},
          {cmd:"FLAG_H", label:"Flag: Fault Record",   type:"flag_toggle"},
      ];
      const DAYS = ["Sun","Mon","Tue","Wed","Thu","Fri","Sat"];

      let schedules   = [];
      let editSchedIdx = -1;   // -1 = new schedule
      let modalCmds   = [];    // [{setting, value}, ...]
      let modalDays   = [0,1,2,3,4,5,6]; // selected days (default: all)

      // ── Value HTML for one cmd row ──────────────────────────────────
      function schedValHtml(def, value, rowIdx) {
          const vid = "sv_" + rowIdx;

          // Single dropdown — POP, PCP, PBT, PGR, V, F, MCHGC, MUCHGC
          if (def.type === "select") {
              const opts = def.opts.map(([v,l]) =>
                  `<option value="${v}"${value===v?" selected":""}>${l}</option>`).join("");
              return `<select class="ctrl-select" id="${vid}" style="flex:1">${opts}</select>`;
          }

          // Single free voltage input — PSDV (matches the settings page number input)
          if (def.type === "volt1") {
              const dv = value ? (parseInt(value)/10).toFixed(1) : "";
              return `<input type="number" class="ctrl-input" id="${vid}" value="${dv}"
                          step="0.1" min="${def.min}" max="${def.max}"
                          placeholder="e.g. ${def.min}" style="width:90px;flex:none">
                      <span class="ctrl-unit">V</span>`;
          }

          // Two free voltage inputs — MCHGV bulk+float (matches settings page number inputs)
          if (def.type === "volt2v") {
              const parts = value ? value.split(",") : ["",""];
              const d1 = parts[0] ? (parseInt(parts[0])/10).toFixed(1) : "";
              const d2 = parts[1] ? (parseInt(parts[1])/10).toFixed(1) : "";
              return `<span style="font-size:11px;color:var(--mu)">${def.labels[0]}</span>
                      <input type="number" class="ctrl-input" id="${vid}_a" value="${d1}"
                          step="0.1" min="${def.min_a}" max="${def.max_a}" style="width:78px;flex:none">
                      <span style="font-size:11px;color:var(--mu)">${def.labels[1]}</span>
                      <input type="number" class="ctrl-input" id="${vid}_b" value="${d2}"
                          step="0.1" min="${def.min_b}" max="${def.max_b}" style="width:78px;flex:none">
                      <span class="ctrl-unit">V</span>`;
          }

          // Two dropdowns from specific option lists — BUCD (mirrors settings page selects)
          if (def.type === "select2") {
              const parts = value ? value.split(",") : ["",""];
              const v1 = parts[0]||"", v2 = parts[1]||"";
              const optsA = def.opts_a.map(([v,l]) =>
                  `<option value="${v}"${v1===v?" selected":""}>${l}</option>`).join("");
              const optsB = def.opts_b.map(([v,l]) =>
                  `<option value="${v}"${v2===v?" selected":""}>${l}</option>`).join("");
              return `<span style="font-size:11px;color:var(--mu)">${def.labels[0]}</span>
                      <select class="ctrl-select" id="${vid}_a" style="width:95px;flex:none">${optsA}</select>
                      <span style="font-size:11px;color:var(--mu)">${def.labels[1]}</span>
                      <select class="ctrl-select" id="${vid}_b" style="width:95px;flex:none">${optsB}</select>`;
          }

          // Two hour dropdowns 00–23 — ACCT, ACLT (mirrors settings page hour-selects)
          if (def.type === "hour2") {
              const parts = value ? value.split(",") : ["0000","0000"];
              const h1 = parts[0] ? parts[0].substring(0,2) : "00";
              const h2 = parts[1] ? parts[1].substring(0,2) : "00";
              const mkOpts = sel => Array.from({length:24}, (_,i) => {
                  const h = String(i).padStart(2,"0");
                  return `<option value="${h}"${sel===h?" selected":""}>${h}:00</option>`;
              }).join("");
              return `<span style="font-size:11px;color:var(--mu)">${def.labels[0]}</span>
                      <select class="ctrl-select hour-select" id="${vid}_a" style="width:88px;flex:none">${mkOpts(h1)}</select>
                      <span style="font-size:11px;color:var(--mu)">→</span>
                      <select class="ctrl-select hour-select" id="${vid}_b" style="width:88px;flex:none">${mkOpts(h2)}</select>`;
          }

          // Individual flag toggle — single Enable/Disable dropdown
          if (def.type === "flag_toggle") {
              const opts = [["E","Enable"],["D","Disable"]].map(([v,l]) =>
                  `<option value="${v}"${value===v?" selected":""}>${l}</option>`).join("");
              return `<select class="ctrl-select" id="${vid}" style="flex:1">${opts}</select>`;
          }

          return `<input type="text" class="ctrl-input" id="${vid}" value="${value||""}" style="flex:1">`;
      }

      // ── Read value back from DOM for a given row ─────────────────────
      function readSchedVal(def, rowIdx) {
          const vid = "sv_" + rowIdx;
          const g = id => (document.getElementById(id)||{}).value||"";
          if (def.type === "select") return g(vid);
          if (def.type === "volt1") {
              // stored as raw ×10 int string, 3 digits, e.g. "440"
              return String(Math.round(parseFloat(g(vid)||"0") * 10)).padStart(3,"0");
          }
          if (def.type === "volt2v") {
              // stored as "bulk×10,float×10"
              return String(Math.round(parseFloat(g(vid+"_a")||"0")*10)) + ","
                   + String(Math.round(parseFloat(g(vid+"_b")||"0")*10));
          }
          if (def.type === "select2") {
              // stored as raw "val_a,val_b" e.g. "440,520"
              return g(vid+"_a") + "," + g(vid+"_b");
          }
          if (def.type === "hour2") {
              // stored as "HHmm,HHmm" — minutes always "00"
              return g(vid+"_a") + "00," + g(vid+"_b") + "00";
          }
          if (def.type === "flag_toggle") {
              // stored as "E" or "D"
              return g(vid);
          }
          return g(vid);
      }

      // ── Render cmd rows inside modal ──────────────────────────────────
      function renderModalCmds() {
          const c = document.getElementById("schedCmdsContainer");
          if (!c) return;

          const usedCmds = new Set(modalCmds.map(mc => mc.setting));
          const available = SCHED_DEFS.filter(d => !usedCmds.has(d.cmd));

          // Existing command rows — setting name is a fixed label, not a dropdown
          let html = modalCmds.map((mc, i) => {
              const def = SCHED_DEFS.find(d => d.cmd === mc.setting) || SCHED_DEFS[0];
              return `<div class="cmd-row" id="cmdrow_${i}">
                  <div class="ctrl-select cmd-setting-sel" style="display:flex;align-items:center;font-size:13px;font-weight:600;background:var(--sf2);border:1px solid var(--bd);border-radius:8px;padding:7px 9px;color:var(--tx)">${def.label}</div>
                  <div class="cmd-val-area" id="cmdval_${i}">${schedValHtml(def, mc.value, i)}</div>
                  <button class="cmd-remove" onclick="removeSchedCmd(${i})">✕</button>
              </div>`;
          }).join("");

          // Picker — only shown when unused settings remain
          if (available.length > 0) {
              const pickerOpts = available.map(d =>
                  `<option value="${d.cmd}">${d.label}</option>`
              ).join("");
              html += `<div style="display:flex;gap:7px;align-items:center;margin-top:${modalCmds.length ? "10px" : "0"}">
                  <select class="ctrl-select" id="schedNewSettingPicker" style="flex:1">${pickerOpts}</select>
                  <button class="ctrl-btn" onclick="addSchedCmd()" style="padding:5px 12px;font-size:12px;flex-shrink:0">+ Add</button>
              </div>`;
          } else {
              html += `<p style="font-size:11px;color:var(--mu);margin-top:8px">All available settings added.</p>`;
          }

          c.innerHTML = html;
      }

      // ── Setting dropdown changed — refresh value area ─────────────────
      function onSchedCmdSettingChange(rowIdx, newCmd) {
          // Save current value first then reset for new setting
          const oldDef = SCHED_DEFS.find(d => d.cmd === modalCmds[rowIdx].setting) || SCHED_DEFS[0];
          modalCmds[rowIdx].value   = readSchedVal(oldDef, rowIdx);
          modalCmds[rowIdx].setting = newCmd;
          modalCmds[rowIdx].value   = "";  // reset value on setting change
          const newDef = SCHED_DEFS.find(d => d.cmd === newCmd) || SCHED_DEFS[0];
          const area = document.getElementById("cmdval_" + rowIdx);
          if (area) area.innerHTML = schedValHtml(newDef, "", rowIdx);
      }

      // ── Sync modalCmds values from DOM before reading ─────────────────
      function syncModalCmdsFromDOM() {
          modalCmds.forEach(function(mc, i) {
              const def = SCHED_DEFS.find(d => d.cmd === mc.setting) || SCHED_DEFS[0];
              mc.value = readSchedVal(def, i);
          });
      }

      // ── Day pills ────────────────────────────────────────────────────
      function renderDayPills() {
          const el = document.getElementById("schedDayPills");
          if (!el) return;
          el.innerHTML = DAYS.map((name, idx) =>
              `<button class="day-pill${modalDays.includes(idx)?" on":""}" onclick="toggleDay(${idx})">${name}</button>`
          ).join("");
      }

      function toggleDay(idx) {
          if (modalDays.includes(idx)) {
              if (modalDays.length === 1) return; // must keep at least one day
              modalDays = modalDays.filter(d => d !== idx);
          } else {
              modalDays.push(idx);
              modalDays.sort();
          }
          renderDayPills();
      }

      // ── Open modal ──────────────────────────────────────────────────
      function openSchedModal(idx) {
          editSchedIdx = idx;
          const modal = document.getElementById("schedModal");
          if (!modal) return;
          document.getElementById("fbSchedModal").className = "ctrl-fb";
          document.getElementById("fbSchedModal").textContent = "";
          if (idx === -1) {
              // New schedule
              document.getElementById("schedModalTitle").textContent = "Add Schedule";
              document.getElementById("schedLabel").value = "";
              document.getElementById("schedTime").value  = "07:00";
              modalDays = [0,1,2,3,4,5,6];
              modalCmds = [{setting:"POP", value:"1"}];
          } else {
              // Edit existing
              const s = schedules[idx];
              document.getElementById("schedModalTitle").textContent = "Edit Schedule";
              document.getElementById("schedLabel").value = s.label || "";
              document.getElementById("schedTime").value  = s.time  || "07:00";
              modalDays = s.days ? [...s.days] : [0,1,2,3,4,5,6];
              modalCmds = s.commands ? s.commands.map(c => ({setting:c.setting, value:c.value})) : [];
          }
          renderDayPills();
          renderModalCmds();
          modal.style.display = "block";
      }

      function closeSchedModal() {
          const modal = document.getElementById("schedModal");
          if (modal) modal.style.display = "none";
      }

      // ── Add / remove cmd rows ────────────────────────────────────────
      function addSchedCmd() {
          syncModalCmdsFromDOM();
          const picker = document.getElementById("schedNewSettingPicker");
          if (!picker || !picker.value) return;
          modalCmds.push({setting: picker.value, value: ""});
          renderModalCmds();
      }

      function removeSchedCmd(idx) {
          syncModalCmdsFromDOM();
          modalCmds.splice(idx, 1);
          renderModalCmds();
      }

      // ── Overlap validation ──────────────────────────────────────────
      function validateOverlap(newSched) {
          const errs = [];
          schedules.forEach(function(s, idx) {
              if (idx === editSchedIdx) return;
              if (!s.enabled) return;
              if (s.time !== newSched.time) return;
              const sharedDays = (s.days||[]).filter(d => newSched.days.includes(d));
              if (!sharedDays.length) return;
              const sKeys = (s.commands||[]).map(c => c.setting);
              const nKeys = newSched.commands.map(c => c.setting);
              const overlap = sKeys.filter(k => nKeys.includes(k));
              if (overlap.length)
                  errs.push(`"${s.label}" — same settings: ${overlap.join(", ")}`);
          });
          return errs;
      }

      // ── Save modal ──────────────────────────────────────────────────
      async function saveSchedModal() {
          syncModalCmdsFromDOM();

          const label = (document.getElementById("schedLabel").value||"").trim();
          const time  = document.getElementById("schedTime").value||"";
          if (!label) { showFb("fbSchedModal","err","Enter a label"); return; }
          if (!time)  { showFb("fbSchedModal","err","Select a time"); return; }
          if (modalDays.length === 0) { showFb("fbSchedModal","err","Select at least one day"); return; }
          if (modalCmds.length === 0) { showFb("fbSchedModal","err","Add at least one setting"); return; }

          const newSched = {
              id:       editSchedIdx >= 0 ? schedules[editSchedIdx].id : ("sched_" + Date.now()),
              label:    label,
              enabled:  editSchedIdx >= 0 ? schedules[editSchedIdx].enabled : true,
              time:     time,
              days:     [...modalDays],
              commands: modalCmds.map(c => ({setting:c.setting, value:c.value}))
          };

          // Overlap check
          const conflicts = validateOverlap(newSched);
          if (conflicts.length) {
              showFb("fbSchedModal","err","Conflict with: " + conflicts.join("; "));
              return;
          }

          // Save
          if (editSchedIdx === -1) {
              schedules.push(newSched);
          } else {
              schedules[editSchedIdx] = newSched;
          }

          showFb("fbSchedModal","busy","Saving…");
          const ok = await saveSchedules();
          if (ok) {
              closeSchedModal();
              renderScheduleList();
          } else {
              // Remove the optimistic add if save failed
              if (editSchedIdx === -1) schedules.pop();
          }
      }

      // ── Toggle schedule on/off ──────────────────────────────────────
      async function toggleSchedule(idx) {
          schedules[idx].enabled = !schedules[idx].enabled;
          renderScheduleList();
          await saveSchedules();
      }

      // ── Delete schedule ──────────────────────────────────────────────
      async function deleteSchedule(idx) {
          schedules.splice(idx, 1);
          renderScheduleList();
          await saveSchedules();
      }

      // ── Format lastFiredEpoch as human-readable ───────────────────────
      function fmtLastRun(epoch) {
          if (!epoch || epoch <= 0) return "Never";
          const d = new Date(epoch * 1000);
          const pad = n => String(n).padStart(2, "0");
          const dateStr = d.getFullYear() + "-" + pad(d.getMonth()+1) + "-" + pad(d.getDate());
          const timeStr = pad(d.getHours()) + ":" + pad(d.getMinutes());
          const diffSec = Math.floor(Date.now()/1000 - epoch);
          let ago;
          if (diffSec < 120)       ago = "just now";
          else if (diffSec < 3600) ago = Math.floor(diffSec/60) + "m ago";
          else if (diffSec < 86400) ago = Math.floor(diffSec/3600) + "h ago";
          else                     ago = Math.floor(diffSec/86400) + "d ago";
          return dateStr + " " + timeStr + " (" + ago + ")";
      }

      // ── Render schedule list ─────────────────────────────────────────
      function renderScheduleList() {
          const el = document.getElementById("scheduleList");
          if (!el) return;
          if (!schedules.length) {
              el.innerHTML = '<p style="color:var(--mu);font-size:13px;padding:4px 0">No schedules yet. Click + Add to create one.</p>';
              return;
          }
          el.innerHTML = schedules.map(function(s, idx) {
              const dayStr = (s.days||[]).map(d => DAYS[d]).join(", ");
              const n = (s.commands||[]).length;
              const on = s.enabled !== false;
              const lastRun = fmtLastRun(s.lastFiredEpoch || 0);
              return `<div class="sched-item">
                  <div class="sched-item-info">
                      <div class="sched-item-name">${s.label||"Unnamed"}</div>
                      <div class="sched-item-meta">${s.time} &middot; ${dayStr} &middot; ${n} setting${n!==1?"s":""}</div>
                      <div class="sched-last-run">Last run: ${lastRun}</div>
                  </div>
                  <button class="sched-tog ${on?"on":"off"}" onclick="toggleSchedule(${idx})">${on?"ON":"OFF"}</button>
                  <button class="ctrl-btn sched-run" id="runBtn_${idx}" onclick="runScheduleNow(${idx})" style="padding:4px 10px;font-size:11px">▶ Run</button>
                  <button class="ctrl-btn" onclick="openSchedModal(${idx})" style="padding:4px 10px;font-size:11px">Edit</button>
                  <button class="ctrl-btn sched-del" onclick="deleteSchedule(${idx})" style="padding:4px 10px;font-size:11px">Del</button>
              </div>`;
          }).join("");
      }

      // ── Run a schedule immediately ───────────────────────────────────
      async function runScheduleNow(idx) {
          const btn = document.getElementById("runBtn_" + idx);
          if (btn) { btn.disabled = true; btn.textContent = "…"; }
          showFb("fbSched", "busy", "Running schedule…");
          try {
              const res = await fetch("./api/schedules/run", {
                  method:  "POST",
                  headers: {"Content-Type":"application/json"},
                  body:    JSON.stringify({index: idx})
              });
              const d = await res.json();
              if (res.ok && d.status === "ok") {
                  if (d.lastFiredEpoch && d.lastFiredEpoch > 0)
                      schedules[idx].lastFiredEpoch = d.lastFiredEpoch;
                  showFb("fbSched", "ok", "✓ " + (d.queued||0) + " command(s) queued");
                  renderScheduleList();
              } else {
                  showFb("fbSched", "err", "✗ " + (d.message||"Failed"));
                  if (btn) { btn.disabled = false; btn.textContent = "▶ Run"; }
              }
          } catch(e) {
              showFb("fbSched", "err", "✗ Network error");
              if (btn) { btn.disabled = false; btn.textContent = "▶ Run"; }
          }
      }

      // ── HTTP helpers ─────────────────────────────────────────────────
      async function loadSchedules() {
          try {
              const res = await fetch("./api/schedules", {cache:"no-store"});
              if (!res.ok) throw new Error("HTTP " + res.status);
              const d = await res.json();
              schedules = Array.isArray(d.schedules) ? d.schedules : [];
              renderScheduleList();
          } catch(e) {
              console.log("[SCHED] load failed:", e);
              document.getElementById("scheduleList").innerHTML =
                  '<span style="color:var(--mu);font-size:13px">Could not load schedules</span>';
          }
      }

      async function saveSchedules() {
          showFb("fbSched","busy","Saving…");
          try {
              const res = await fetch("./api/schedules", {
                  method:  "POST",
                  headers: {"Content-Type":"application/json"},
                  body:    JSON.stringify({schedules})
              });
              if (!res.ok) throw new Error("HTTP " + res.status);
              const d = await res.json();
              if (d.status === "ok") {
                  showFb("fbSched","ok","✓ Saved & synced to cloud");
                  return true;
              } else {
                  showFb("fbSched","err","✗ " + (d.message||"Save failed"));
                  return false;
              }
          } catch(e) {
              showFb("fbSched","err","✗ Network error");
              return false;
          }
      }

      // Load on page open
      loadSchedules();
    </script>
  </body>
</html>
)rawliteral";
