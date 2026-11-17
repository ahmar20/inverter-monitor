#pragma once
// ============================================================================
// page_home.h — the /home system-management page: WiFi profiles, system stats, cloud queue, reboot.
// Pure HTML/CSS/JS served as-is by web_server.h. No C++ logic here.
// ============================================================================
#include <Arduino.h>

const char home_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en" data-theme="dark">
<head>
<meta charset="UTF-8"/>
<meta name="viewport" content="width=device-width,initial-scale=1"/>
<title>Inverter – System Home</title>
<style>
  :root{--font-ui:system-ui,-apple-system,"Segoe UI",Roboto,sans-serif;--font-num:ui-monospace,"SFMono-Regular","JetBrains Mono",monospace;
    --amber:#f5a623;--charge:#34d399;--discharge:#fb7185;--grid-c:#38bdf8;--r:14px;--g:14px;--ease:cubic-bezier(.22,1,.36,1);}
  html[data-theme="dark"]{--bg:#0b1220;--sf:#131d31;--sf2:#182545;--bd:#24334f;--tx:#e8eef8;--mu:#90a2bd;--sh:0 8px 24px -12px rgba(0,0,0,.75);--glass:rgba(19,29,49,.45);}
  html[data-theme="light"]{--bg:#eef2f8;--sf:#fff;--sf2:#f4f7fc;--bd:#d7e0ee;--tx:#17233a;--mu:#5c6b83;--sh:0 8px 24px -16px rgba(30,50,90,.3);--glass:rgba(255,255,255,.45);}
  *{box-sizing:border-box;margin:0;padding:0}
  body{font-family:var(--font-ui);background:var(--bg);color:var(--tx);min-height:100vh;-webkit-font-smoothing:antialiased}
  .wrap{padding:16px 16px 60px}
  header{display:flex;flex-wrap:wrap;align-items:center;justify-content:space-between;gap:10px;position:sticky;top:0;z-index:100;
    background:var(--glass);backdrop-filter:blur(20px);-webkit-backdrop-filter:blur(20px);border-bottom:1px solid var(--bd);padding:12px 16px}
  .brand{display:flex;align-items:center;gap:10px}
  .logo{width:40px;height:40px;border-radius:11px;display:grid;place-items:center;background:linear-gradient(145deg,var(--amber),#d97706);color:#1a1206}
  .logo svg{width:20px;height:20px}
  .brand h1{font-size:16px;font-weight:700}
  .hc{display:flex;gap:7px;flex-wrap:wrap}
  .tbtn{font-family:var(--font-ui);display:inline-flex;align-items:center;gap:5px;padding:6px 11px;border-radius:9px;background:var(--sf2);
    color:var(--tx);border:1px solid var(--bd);font-size:12.5px;font-weight:600;cursor:pointer;text-decoration:none}
  .tbtn:hover{border-color:var(--amber)}
  .section{background:var(--sf);border:1px solid var(--bd);border-radius:var(--r);padding:18px;box-shadow:var(--sh);margin-bottom:var(--g)}
  .sec-head{display:flex;align-items:center;gap:10px;margin-bottom:14px}
  .sec-title{font-size:15px;font-weight:700}
  .grid2{display:grid;grid-template-columns:repeat(auto-fill,minmax(160px,1fr));gap:12px}
  .box{background:var(--sf2);border-radius:10px;padding:12px 14px}
  .box .k{font-size:10.5px;color:var(--mu);text-transform:uppercase;letter-spacing:.06em;font-weight:600}
  .box .v{font-family:var(--font-num);font-size:17px;font-weight:700;margin-top:4px}
  .prof-row{display:flex;align-items:center;gap:8px;background:var(--sf2);border-radius:9px;padding:9px 11px;margin-bottom:7px;flex-wrap:wrap}
  .prof-row input{font-family:var(--font-ui);font-size:13px;background:var(--sf);color:var(--tx);border:1px solid var(--bd);border-radius:7px;padding:6px 8px}
  .prof-ssid{flex:1 1 140px;min-width:0;font-weight:700}
  .prof-pass{flex:1 1 140px;min-width:0}
  .active-badge{font-size:10px;font-weight:700;color:var(--charge);background:color-mix(in srgb,var(--charge) 15%,transparent);padding:3px 8px;border-radius:999px}
  .fallback-badge{font-size:10px;font-weight:700;color:var(--mu);background:var(--sf);border:1px solid var(--bd);padding:3px 8px;border-radius:999px}
  .net-row{display:flex;align-items:center;justify-content:space-between;padding:8px 11px;background:var(--sf2);border-radius:8px;margin-bottom:6px}
  .net-row .nm{font-size:13px;font-weight:600}
  .net-row .rs{font-size:11px;color:var(--mu);font-family:var(--font-num)}
  button.act{font-family:var(--font-ui);font-size:12.5px;font-weight:700;padding:7px 14px;border-radius:8px;border:none;cursor:pointer;
    background:var(--amber);color:#1a1206}
  button.act.danger{background:var(--discharge);color:#2a0a0a}
  button.act.ghost{background:var(--sf2);color:var(--tx);border:1px solid var(--bd)}
  button.act:hover{opacity:.85}
  button.act:disabled{opacity:.4;cursor:not-allowed}
  .fb{font-size:12px;font-weight:600;padding:6px 10px;border-radius:7px;margin-top:8px;display:none}
  .fb.ok{display:block;background:color-mix(in srgb,var(--charge) 14%,transparent);color:var(--charge)}
  .fb.err{display:block;background:color-mix(in srgb,var(--discharge) 14%,transparent);color:var(--discharge)}
  .fb.busy{display:block;background:color-mix(in srgb,var(--amber) 14%,transparent);color:var(--amber)}
  .row{display:flex;gap:10px;flex-wrap:wrap;align-items:center}
  .hint{font-size:11.5px;color:var(--mu);margin-top:10px}
</style>
</head>
<body>
<header>
  <div class="brand">
    <div class="logo"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round"><path d="m3 9 9-7 9 7v11a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2z"/><polyline points="9 22 9 12 15 12 15 22"/></svg></div>
    <h1>System Home</h1>
  </div>
  <div class="hc">
    <a class="tbtn" href="./dashboard">Dashboard</a>
    <a class="tbtn" href="./settings">Settings</a>
    <a class="tbtn" href="./history">History</a>
    <a class="tbtn" href="./update">Firmware Update</a>
    <button class="tbtn" id="themeToggle">Theme</button>
  </div>
</header>
<div class="wrap">

  <div class="section">
    <div class="sec-head"><span class="sec-title">System Status</span></div>
    <div class="grid2" id="sysGrid">
      <div class="box"><div class="k">Uptime</div><div class="v" id="sUptime">--</div></div>
      <div class="box"><div class="k">Chip</div><div class="v" id="sChip">--</div></div>
      <div class="box"><div class="k">Chip Temp</div><div class="v" id="sChipTemp">--</div></div>
      <div class="box"><div class="k">Build</div><div class="v" id="sBuild" style="font-size:12px">--</div></div>
      <div class="box"><div class="k">Free Heap</div><div class="v" id="sHeap">--</div></div>
      <div class="box"><div class="k">PSRAM</div><div class="v" id="sPsram">--</div></div>
      <div class="box"><div class="k">Flash</div><div class="v" id="sFlash">--</div></div>
      <div class="box"><div class="k">Local IP</div><div class="v" id="sIP">--</div></div>
      <div class="box"><div class="k">MAC</div><div class="v" id="sMac" style="font-size:13px">--</div></div>
    </div>
  </div>

  <div class="section">
    <div class="sec-head"><span class="sec-title">Cloud Upload Queue</span></div>
    <div class="grid2">
      <div class="box"><div class="k">RAM Queue</div><div class="v" id="qRam">--</div></div>
      <div class="box"><div class="k">Flash Queue</div><div class="v" id="qFlash">--</div></div>
      <div class="box"><div class="k">Dropped</div><div class="v" id="qDropped">--</div></div>
      <div class="box"><div class="k">Last Upload</div><div class="v" id="qLast" style="font-size:12px">--</div></div>
    </div>
    <div class="row" style="margin-top:14px">
      <button class="act danger" onclick="clearQueue()">Clear Queue</button>
    </div>
    <div class="fb" id="fbQueue"></div>
  </div>

  <div class="section">
    <div class="sec-head"><span class="sec-title">WiFi — Current Connection</span></div>
    <div class="grid2">
      <div class="box"><div class="k">Status</div><div class="v" id="wStatus">--</div></div>
      <div class="box"><div class="k">SSID</div><div class="v" id="wSSID">--</div></div>
      <div class="box"><div class="k">Signal</div><div class="v" id="wRSSI">--</div></div>
    </div>
    <div class="row" style="margin-top:14px">
      <button class="act danger" onclick="disconnectWifi()">Disconnect</button>
    </div>
    <div class="fb" id="fbWifi"></div>
  </div>

  <div class="section">
    <div class="sec-head"><span class="sec-title">Saved WiFi Profiles</span></div>
    <div id="profileList"></div>
    <div class="fb" id="fbProfiles"></div>
    <div class="hint">New networks are added from the scan list below — there's no manual add. Built-in fallback networks are always available even if deleted here or if saved profiles fail.</div>
  </div>

  <div class="section">
    <div class="sec-head"><span class="sec-title">Scan for Networks</span></div>
    <button class="act ghost" onclick="scanWifi()">Scan Now</button>
    <div id="scanList" style="margin-top:12px"></div>
  </div>

  <div class="section">
    <div class="sec-head"><span class="sec-title">Connectivity Log</span></div>
    <div id="invLogStatus" style="font-size:13px;color:var(--mu);margin-bottom:10px">Loading…</div>
    <div id="invLogList"></div>
    <div class="hint">Inverter gaps of 5 seconds or longer, and every WiFi disconnect/reconnect, are logged — each with how long it was down.</div>
  </div>

  <div class="section">
    <div class="sec-head"><span class="sec-title">System Actions</span></div>
    <div class="row">
      <button class="act danger" onclick="rebootDevice()">Reboot ESP32</button>
    </div>
    <div class="fb" id="fbReboot"></div>
  </div>

</div>
<script>
"use strict";
(function(){
  const root=document.documentElement,btn=document.getElementById("themeToggle");
  try{const s=localStorage.getItem("inv-theme");if(s)root.setAttribute("data-theme",s);}catch(e){}
  btn.addEventListener("click",function(){
    const n=root.getAttribute("data-theme")==="dark"?"light":"dark";
    root.setAttribute("data-theme",n);
    try{localStorage.setItem("inv-theme",n);}catch(e){}
  });
})();
const $=id=>document.getElementById(id);
function fmtUptime(ms){const s=Math.floor(ms/1000);const d=Math.floor(s/86400),h=Math.floor(s%86400/3600),m=Math.floor(s%3600/60);
  return (d?d+"d ":"")+(h?h+"h ":"")+m+"m";}
function showFb(id,type,msg){const el=$(id);if(!el)return;el.className="fb "+type;el.textContent=msg;
  if(type==="ok")setTimeout(()=>{el.className="fb"},4000);}

async function fetchSystem(){
  try{
    const r=await fetch("./api/system",{cache:"no-store"});
    if(!r.ok)throw new Error("HTTP "+r.status);
    const d=await r.json();
    $("sUptime").textContent=fmtUptime(d.uptimeMs);
    $("sChip").textContent=d.chipModel+" rev"+d.chipRevision;
    $("sChipTemp").textContent=(typeof d.chipTemp==="number")?d.chipTemp.toFixed(1)+" °C":"--";
    $("sBuild").textContent=d.buildDate;
    $("sHeap").textContent=(d.freeHeap/1024).toFixed(0)+" KB";
    $("sPsram").textContent=d.psramFound?(d.usedPsram/1024).toFixed(0)+" / "+(d.totalPsram/1024).toFixed(0)+" KB":"Not found";
    $("sFlash").textContent=d.flashTotal?(d.flashUsed/1024).toFixed(0)+" / "+(d.flashTotal/1024).toFixed(0)+" KB":"N/A";
    $("sIP").textContent=d.localIP||"--";
    $("sMac").textContent=d.macAddress||"--";
    $("qRam").textContent=d.queueRam+" / "+d.queueRamCap;
    $("qFlash").textContent=d.queueFlash;
    $("qDropped").textContent=d.queueDropped;
    $("qLast").textContent=d.lastCloudUpdate||"Never";
    $("wStatus").textContent=d.wifiConnected?"Connected":"Disconnected";
    $("wSSID").textContent=d.wifiSSID||"--";
    $("wRSSI").textContent=d.wifiConnected?(d.wifiRSSI+" dBm"):"--";
  }catch(e){console.log("[HOME] system fetch failed:",e);}
}

let profiles=[];

function renderProfiles(){
  const el=$("profileList");
  if(!profiles.length){el.innerHTML='<p style="color:var(--mu);font-size:13px">No profiles saved.</p>';return;}
  el.innerHTML=profiles.map((p)=>`
    <div class="prof-row">
      <span class="prof-ssid" style="flex:1 1 140px;min-width:0;font-weight:700">${(p.ssid||"").replace(/</g,"&lt;")}</span>
      ${p.active?'<span class="active-badge">ACTIVE</span>':''}
      ${p.isFallback?'<span class="fallback-badge">BUILT-IN FALLBACK</span>':
        `<button class="act danger" style="padding:6px 10px" onclick='deleteProfile(${JSON.stringify(p.ssid)})'>Delete</button>`}
    </div>`).join("");
}

async function loadProfiles(){
  try{
    const r=await fetch("./api/wifi/profiles",{cache:"no-store"});
    if(!r.ok)throw new Error("HTTP "+r.status);
    const d=await r.json();
    profiles=d.profiles||[];
    renderProfiles();
  }catch(e){
    $("profileList").innerHTML='<p style="color:var(--mu);font-size:13px">Could not load WiFi profiles.</p>';
  }
}

async function addProfile(ssid,password){
  showFb("fbProfiles","busy","Adding…");
  try{
    const r=await fetch("./api/wifi/profiles/add",{method:"POST",headers:{"Content-Type":"application/json"},
      body:JSON.stringify({ssid,password})});
    const d=await r.json();
    if(r.ok&&d.status==="ok"){showFb("fbProfiles","ok","✓ Added");loadProfiles();}
    else showFb("fbProfiles","err","✗ "+(d.message||"Failed"));
  }catch(e){showFb("fbProfiles","err","✗ Network error");}
}

async function deleteProfile(ssid){
  if(!confirm('Delete saved network "'+ssid+'"?'))return;
  showFb("fbProfiles","busy","Deleting…");
  try{
    const r=await fetch("./api/wifi/profiles/delete",{method:"POST",headers:{"Content-Type":"application/json"},
      body:JSON.stringify({ssid})});
    const d=await r.json();
    if(r.ok&&d.status==="ok"){showFb("fbProfiles","ok","✓ Deleted");loadProfiles();}
    else showFb("fbProfiles","err","✗ "+(d.message||"Failed"));
  }catch(e){showFb("fbProfiles","err","✗ Network error");}
}

async function disconnectWifi(){
  if(!confirm("Disconnect from the current WiFi network? The device will rescan and reconnect automatically."))return;
  showFb("fbWifi","busy","Disconnecting…");
  try{
    const r=await fetch("./api/wifi/disconnect",{method:"POST"});
    const d=await r.json();
    if(r.ok&&d.status==="ok"){showFb("fbWifi","ok","✓ Disconnected — reconnecting…");fetchSystem();}
    else showFb("fbWifi","err","✗ "+(d.message||"Failed"));
  }catch(e){showFb("fbWifi","err","✗ Request failed (page may briefly lose connection)");}
}

async function scanWifi(){
  $("scanList").innerHTML='<p style="color:var(--mu);font-size:13px">Scanning…</p>';
  async function poll(start){
    const r=await fetch("./api/wifi/scan"+(start?"?start=1":""),{cache:"no-store"});
    const d=await r.json();
    if(d.scanning){setTimeout(()=>poll(false),1500);return;}
    if(!d.networks.length){$("scanList").innerHTML='<p style="color:var(--mu);font-size:13px">No networks found.</p>';return;}
    $("scanList").innerHTML=d.networks.sort((a,b)=>b.rssi-a.rssi).map(n=>`
      <div class="net-row">
        <div><div class="nm">${n.ssid||"(hidden)"}</div></div>
        <div style="display:flex;align-items:center;gap:10px">
          <span class="rs">${n.rssi} dBm ${n.secure?"🔒":""}</span>
          <button class="act ghost" style="padding:5px 10px;font-size:11px" onclick='addFromScan(${JSON.stringify(n.ssid)},${n.secure})'>+ Add</button>
        </div>
      </div>`).join("");
  }
  poll(true);
}
function addFromScan(ssid,secure){
  let password="";
  if(secure){
    const entered=prompt('Password for "'+ssid+'":');
    if(entered===null)return;   // cancelled
    password=entered;
  }
  addProfile(ssid,password);
}

async function rebootDevice(){
  if(!confirm("Reboot the ESP32 now? The device will be unreachable for a few seconds."))return;
  showFb("fbReboot","busy","Rebooting…");
  try{
    const r=await fetch("./api/reboot",{method:"POST"});
    if(r.status===401){showFb("fbReboot","err","✗ Authentication required - not rebooted");return;}
    if(!r.ok)throw new Error("HTTP "+r.status);
    showFb("fbReboot","ok","✓ Rebooting — reconnect shortly");
  }catch(e){showFb("fbReboot","err","✗ Request failed (device may still be rebooting)");}
}

async function clearQueue(){
  if(!confirm("Clear the cloud upload queue? Unsent data will be lost."))return;
  showFb("fbQueue","busy","Clearing…");
  try{
    const r=await fetch("./api/queue/clear",{method:"POST"});
    if(r.status===401){showFb("fbQueue","err","✗ Authentication required - not cleared");return;}
    const d=await r.json();
    if(r.ok&&d.status==="ok"){showFb("fbQueue","ok","✓ Queue cleared");fetchSystem();}
    else showFb("fbQueue","err","✗ "+(d.message||"Failed"));
  }catch(e){showFb("fbQueue","err","✗ Network error");}
}

function fmtDuration(ms){
  const s=Math.round(ms/1000);
  if(s<60)return s+"s";
  const m=Math.floor(s/60),rs=s%60;
  return m+"m "+rs+"s";
}

async function loadInverterLog(){
  try{
    const r=await fetch("./api/inverter/log",{cache:"no-store"});
    if(!r.ok)throw new Error("HTTP "+r.status);
    const d=await r.json();
    const parts=[];
    if(d.inverterCurrentlyDown)parts.push("⚠ Inverter not responding");
    if(d.wifiCurrentlyDown)parts.push("⚠ WiFi disconnected");
    $("invLogStatus").textContent=parts.length?parts.join(" · "):"All connections normal";
    $("invLogStatus").style.color=parts.length?"var(--discharge)":"var(--mu)";
    const list=$("invLogList");
    if(!d.events||!d.events.length){
      list.innerHTML='<p style="color:var(--mu);font-size:13px">No outages logged yet.</p>';
      return;
    }
    list.innerHTML=d.events.slice().reverse().map(e=>{
      const label=e.source==="wifi"?"WiFi":"Inverter";
      if(e.event==="down"){
        return `<div class="net-row"><div class="nm">⚠ ${label} went down</div><div class="rs">${e.timestamp}</div></div>`;
      }
      return `<div class="net-row"><div class="nm">✓ ${label} recovered</div><div class="rs">${e.timestamp} · down for ${fmtDuration(e.downForMs)}</div></div>`;
    }).join("");
  }catch(e){
    $("invLogStatus").textContent="Could not load connectivity log.";
  }
}

fetchSystem();
loadProfiles();
loadInverterLog();
setInterval(fetchSystem,2000);
setInterval(loadInverterLog,6000);
</script>
</body>
</html>
)rawliteral";
