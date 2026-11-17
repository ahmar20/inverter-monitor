#pragma once
// ============================================================================
// assets.h — small static assets: PWA manifest, icon, service worker.
// ============================================================================
#include <Arduino.h>

const char manifest_json[] PROGMEM = R"rawliteral(
{
  "name": "Inverter Monitor - InfiniSolar VII 5KW",
  "short_name": "Inverter",
  "description": "Real-time solar power system monitoring and control",
  "start_url": "/dashboard",
  "scope": "/",
  "display": "standalone",
  "orientation": "portrait",
  "theme_color": "#0b1220",
  "background_color": "#0b1220",
  "icons": [
    {
      "src": "./icon.svg",
      "sizes": "any",
      "type": "image/svg+xml",
      "purpose": "any maskable"
    }
  ]
}
)rawliteral";


const char icon_svg[] PROGMEM = R"rawliteral(
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 512 512">
  <rect width="512" height="512" rx="80" fill="#f5a623"/>
  <path d="M277 42 64 298H213L192 469 405 213H256L277 42Z" fill="#1a1206"/>
</svg>
)rawliteral";


const char sw_js[] PROGMEM = R"rawliteral(
const CACHE = 'inv-)rawliteral" __DATE__ " " __TIME__ R"rawliteral(';
const BASE = self.location.pathname.replace('sw.js', '');
const SHELL = [
  BASE,
  BASE + 'home',
  BASE + 'dashboard',
  BASE + 'history',
  BASE + 'manifest.json',
  BASE + 'icon.svg',
  'https://cdnjs.cloudflare.com/ajax/libs/Chart.js/4.4.1/chart.umd.js'
];

self.addEventListener('install', e => {
  e.waitUntil(caches.open(CACHE).then(c => c.addAll(SHELL)));
  self.skipWaiting();
});

self.addEventListener('activate', e => {
  e.waitUntil(
    caches.keys().then(keys =>
      Promise.all(keys.filter(k => k !== CACHE).map(k => caches.delete(k)))
    )
  );
  self.clients.claim();
});

self.addEventListener('fetch', e => {
  const url = new URL(e.request.url);
  // Always network for API and settings — never serve stale inverter data
  if (url.pathname.includes('/api/') || url.pathname.endsWith('/settings')) {
    e.respondWith(fetch(e.request));
    return;
  }
  // Cache-first for app shell
  e.respondWith(
    caches.match(e.request).then(cached =>
      cached || fetch(e.request).then(resp => {
        if (resp.ok) {
          caches.open(CACHE).then(c => c.put(e.request, resp.clone()));
        }
        return resp;
      })
    )
  );
});
)rawliteral";


