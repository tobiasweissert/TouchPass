#ifndef WEBPAGE_H
#define WEBPAGE_H

const char WEBPAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1">
<title>TouchPass Configurator</title>
<style>
:root{--bg:#0a0a0f;--bg2:#12121a;--bg3:#1a1a25;--bgi:#0d0d14;--ac:#00d4aa;--acd:#00a88a;--acg:rgba(0,212,170,.3);--tp:#fff;--ts:#8888aa;--tm:#555566;--ok:#00d4aa;--err:#ff4757;--brd:#2a2a3a}
*{box-sizing:border-box;margin:0;padding:0}body{font-family:-apple-system,sans-serif;background:var(--bg);color:var(--tp);min-height:100vh;padding:16px;line-height:1.5}.app{max-width:480px;margin:0 auto}
.header{display:flex;align-items:center;justify-content:space-between;padding:12px 0 24px}.logo{display:flex;align-items:center;gap:10px}.logo-icon{width:32px;height:32px;fill:var(--ac)}.logo-text{font-size:20px;font-weight:600}.logo-text span{color:var(--ac)}
.status-dots{display:flex;gap:8px}.status-dot{width:8px;height:8px;border-radius:50%;background:var(--tm)}.status-dot.active{background:var(--ok);box-shadow:0 0 8px var(--ok)}.status-dot.error{background:var(--err);box-shadow:0 0 8px var(--err)}
.card{background:var(--bg3);border:1px solid var(--brd);border-radius:16px;padding:20px;margin-bottom:16px}.card-header{display:flex;align-items:center;justify-content:space-between;margin-bottom:16px}.card-title{font-size:13px;font-weight:600;text-transform:uppercase;letter-spacing:1px;color:var(--ts)}.card-badge{font-size:11px;padding:4px 10px;border-radius:20px;background:var(--bgi);color:var(--ts)}
.hand-container{display:flex;flex-direction:column;align-items:center;padding:10px 0}.hand-toggle{display:flex;background:var(--bgi);border-radius:25px;padding:4px;margin-bottom:20px}.hand-toggle-btn{padding:8px 20px;border:none;background:transparent;color:var(--ts);font-size:13px;font-weight:500;border-radius:20px;cursor:pointer;transition:all .2s}.hand-toggle-btn.active{background:var(--ac);color:var(--bg)}
.hand-wrapper{position:relative}.hand-svg{width:100%;max-width:280px;height:auto}.hand-outline{fill:var(--bg2);stroke:var(--brd);stroke-width:4;transition:all .3s}.finger-hotspot{fill:rgba(0,212,170,.1);stroke:var(--brd);stroke-width:2;cursor:pointer;transition:all .3s}.finger-hotspot:hover{fill:rgba(0,212,170,.25);stroke:var(--ac)}.finger-hotspot.configured{fill:rgba(0,212,170,.35);stroke:var(--ac);stroke-width:3}.finger-hotspot.selected{fill:rgba(0,212,170,.4);stroke:var(--ac);stroke-width:3;stroke-dasharray:8 4;animation:dash 1s linear infinite}@keyframes dash{to{stroke-dashoffset:-12}}.finger-label{font-size:28px;fill:var(--ts);text-anchor:middle;pointer-events:none;font-weight:500}
.finger-panel{display:none;background:var(--bg2);border:1px solid var(--brd);border-radius:12px;padding:16px;margin-top:16px}.finger-panel.active{display:block}.panel-header{display:flex;align-items:center;justify-content:space-between;margin-bottom:12px}.panel-title{font-size:16px;font-weight:600;color:var(--ac)}.panel-status{font-size:12px;color:var(--ts)}.panel-status.has-pwd{color:var(--ok)}.panel-actions{display:flex;gap:8px}
.btn{padding:12px 24px;border:none;border-radius:10px;font-size:14px;font-weight:500;cursor:pointer;transition:all .2s;display:inline-flex;align-items:center;justify-content:center;gap:8px}.btn:disabled{opacity:.5;cursor:not-allowed}.btn-primary{background:var(--ac);color:var(--bg)}.btn-primary:hover:not(:disabled){background:var(--acd)}.btn-secondary{background:var(--bgi);color:var(--tp);border:1px solid var(--brd)}.btn-danger{background:rgba(255,71,87,.15);color:var(--err)}.btn-sm{padding:8px 16px;font-size:12px}.btn-block{width:100%}
.credentials-list{display:flex;flex-direction:column;gap:10px}.credential-card{background:var(--bg2);border:1px solid var(--brd);border-radius:12px;padding:14px 16px;display:flex;align-items:center;gap:14px;cursor:pointer;transition:all .2s}.credential-card:hover{border-color:var(--ac)}.credential-icon{width:40px;height:40px;border-radius:10px;background:linear-gradient(135deg,var(--ac),var(--acd));display:flex;align-items:center;justify-content:center;flex-shrink:0}.credential-icon svg{width:20px;height:20px;fill:var(--bg)}.credential-info{flex:1;min-width:0}.credential-name{font-size:15px;font-weight:500;margin-bottom:2px}.credential-meta{font-size:12px;color:var(--ts);display:flex;align-items:center;gap:8px}.credential-actions{display:flex;gap:6px}
.icon-btn{width:32px;height:32px;border:none;background:var(--bgi);border-radius:8px;cursor:pointer;display:flex;align-items:center;justify-content:center;color:var(--ts);transition:all .2s}.icon-btn:hover{background:var(--bg3);color:var(--tp)}.icon-btn.danger:hover{background:rgba(255,71,87,.15);color:var(--err)}
.empty-state{text-align:center;padding:40px 20px;color:var(--tm)}.empty-state-icon{width:48px;height:48px;margin:0 auto 12px;opacity:.3}
.status-bar{display:flex;justify-content:space-between;padding:8px 0;margin-bottom:8px}.status-item{display:flex;align-items:center;gap:6px;font-size:12px;color:var(--ts)}.status-item .indicator{width:6px;height:6px;border-radius:50%;background:var(--tm)}.status-item .indicator.success{background:var(--ok)}.status-item .indicator.error{background:var(--err)}
.modal-overlay{display:none;position:fixed;top:0;left:0;right:0;bottom:0;background:rgba(0,0,0,.85);z-index:1000;align-items:center;justify-content:center;padding:20px}.modal-overlay.active{display:flex}.modal{background:var(--bg3);border:1px solid var(--brd);border-radius:20px;width:100%;max-width:360px;overflow:hidden}.modal-header{padding:20px 20px 0;text-align:center}.modal-title{font-size:18px;font-weight:600;margin-bottom:4px}.modal-subtitle{font-size:13px;color:var(--ts)}.modal-body{padding:20px}.modal-footer{padding:0 20px 20px;display:flex;gap:10px}.modal-footer .btn{flex:1}
.fingerprint-container{width:120px;height:120px;margin:20px auto;position:relative}.fingerprint-svg{width:100%;height:100%}.fp-seg{fill:var(--tm);stroke:none;transition:fill .4s,filter .4s}.fp-seg.active{fill:var(--ac);filter:drop-shadow(0 0 4px var(--acg))}.fp-seg.success{fill:var(--ok);filter:drop-shadow(0 0 6px rgba(0,212,170,.6))}.fp-seg.error{fill:var(--err);filter:drop-shadow(0 0 6px rgba(255,71,87,.6))}
.fingerprint-status{text-align:center;margin-top:16px}.fingerprint-status-text{font-size:14px;color:var(--ac);margin-bottom:4px}.fingerprint-status-step{font-size:12px;color:var(--tm)}
.form-group{margin-bottom:16px}.form-label{display:block;font-size:12px;font-weight:500;color:var(--ts);margin-bottom:6px;text-transform:uppercase;letter-spacing:.5px}.form-input{width:100%;padding:14px 16px;background:var(--bgi);border:1px solid var(--brd);border-radius:10px;color:var(--tp);font-size:15px;transition:all .2s}.form-input:focus{outline:none;border-color:var(--ac);box-shadow:0 0 0 3px var(--acg)}.form-input::placeholder{color:var(--tm)}
.password-wrapper{position:relative}.password-toggle{position:absolute;right:12px;top:50%;transform:translateY(-50%);background:none;border:none;color:var(--tm);cursor:pointer;padding:4px;font-size:12px}
.checkbox-group{display:flex;align-items:center;gap:10px}.checkbox{width:20px;height:20px;border-radius:6px;border:2px solid var(--brd);background:var(--bgi);cursor:pointer;appearance:none;transition:all .2s}.checkbox:checked{background:var(--ac);border-color:var(--ac)}.checkbox-label{font-size:14px;cursor:pointer}
.result-container{text-align:center;padding:20px}.result-icon{width:72px;height:72px;margin:0 auto 16px;border-radius:50%;display:flex;align-items:center;justify-content:center;font-size:32px}.result-icon.success{background:rgba(0,212,170,.15);color:var(--ok)}.result-icon.error{background:rgba(255,71,87,.15);color:var(--err)}.result-title{font-size:20px;font-weight:600;margin-bottom:8px}.result-message{font-size:14px;color:var(--ts)}
.log-container{background:var(--bgi);border-radius:10px;padding:12px;max-height:120px;overflow-y:auto;font-family:monospace;font-size:11px}.log-entry{padding:4px 0;display:flex;gap:8px}.log-time{color:var(--tm);flex-shrink:0}.log-msg{color:var(--ts)}.log-msg.success{color:var(--ok)}.log-msg.error{color:var(--err)}.log-msg.info{color:var(--ac)}
.hidden{display:none!important}.text-sm{font-size:12px}.mt-2{margin-top:8px}
.connect-screen{display:flex;flex-direction:column;align-items:center;justify-content:center;min-height:80vh;text-align:center}.connect-screen h1{margin-bottom:16px}.connect-screen p{color:var(--ts);margin-bottom:24px}.connect-screen .btn{min-width:200px}
.connection-status{padding:12px;background:var(--bg2);border-radius:10px;margin-bottom:16px;text-align:center;font-size:13px}.connection-status.connected{color:var(--ok)}.connection-status.disconnected{color:var(--err)}
</style>
</head>
<body>

<!-- Connection Screen -->
<div class="connect-screen" id="connectScreen">
  <div class="logo" style="margin-bottom:24px">
    <svg class="logo-icon" style="width:48px;height:48px" viewBox="0 0 512 512"><path d="M256 0C114.6 0 0 114.6 0 256s114.6 256 256 256 256-114.6 256-256S397.4 0 256 0zm0 464c-114.7 0-208-93.3-208-208S141.3 48 256 48s208 93.3 208 208-93.3 208-208 208zm0-336c-70.7 0-128 57.3-128 128s57.3 128 128 128 128-57.3 128-128-57.3-128-128-128zm0 208c-44.1 0-80-35.9-80-80s35.9-80 80-80 80 35.9 80 80-35.9 80-80 80z"/></svg>
  </div>
  <h1>TouchPass Configurator</h1>
  <p>Connect your TouchPass device via USB to get started</p>
  <p class="text-sm" style="color:var(--tm);margin-bottom:24px">Requires Chrome 89+ or Edge 89+</p>
  <button class="btn btn-primary" onclick="connectDevice()">Connect Device</button>
</div>

<!-- Main App (hidden until connected) -->
<div class="app hidden" id="mainApp">
<div class="connection-status connected" id="connectionStatus">
  Connected to TouchPass
</div>

<header class="header">
  <div class="logo">
    <svg class="logo-icon" viewBox="0 0 512 512"><path d="M256 0C114.6 0 0 114.6 0 256s114.6 256 256 256 256-114.6 256-256S397.4 0 256 0zm0 464c-114.7 0-208-93.3-208-208S141.3 48 256 48s208 93.3 208 208-93.3 208-208 208zm0-336c-70.7 0-128 57.3-128 128s57.3 128 128 128 128-57.3 128-128-57.3-128-128-128zm0 208c-44.1 0-80-35.9-80-80s35.9-80 80-80 80 35.9 80 80-35.9 80-80 80z"/></svg>
    <span class="logo-text">Touch<span>Pass</span></span>
  </div>
  <div class="status-dots">
    <div class="status-dot" id="sensorDot"></div>
    <div class="status-dot" id="bleDot"></div>
  </div>
</header>

<div class="status-bar">
  <div class="status-item"><span class="indicator" id="sensorIndicator"></span><span id="sensorStatus">Sensor: --</span></div>
  <div class="status-item"><span class="indicator" id="bleIndicator"></span><span id="bleStatus">BLE: --</span></div>
  <div class="status-item"><span id="countStatus">0/200</span></div>
</div>

<div class="card">
  <div class="card-header"><span class="card-title">Your Fingers</span><span class="card-badge" id="enrolledBadge">0 enrolled</span></div>
  <div class="hand-container">
    <div class="hand-toggle">
      <button class="hand-toggle-btn active" id="leftHandBtn" onclick="showHand('left')">Left</button>
      <button class="hand-toggle-btn" id="rightHandBtn" onclick="showHand('right')">Right</button>
    </div>
    <div class="hand-wrapper">
      <svg class="hand-svg" id="leftHand" viewBox="0 0 746 746"><path class="hand-outline" d="M567.52,142.808c-18.1-2.46-27.659,12.74-28.846,29.688l-13.495,134.416c-0.252,6.244-5.518,11.114-11.769,10.862c-6.244-0.245-11.106-5.51-10.861-11.768l8.186-240.043c0.598-16.984-12.675-31.234-29.657-31.838c-16.984-0.604-31.234,12.675-31.847,29.651l-7.999,238.13c0,5.036-4.085,9.114-9.114,9.114c-5.034,0-9.112-4.079-9.112-9.114l-0.008-271.147C422.998,13.776,409.222,0,392.231,0s-30.767,13.775-30.767,30.759l-0.224,266.091c0,5.719-4.633,10.322-10.357,10.351c-8.366,0.043-10.014-7.826-10.014-7.826c-0.201-0.816-10.258-236.442-10.258-236.442c-0.885-16.969-15.363-30.004-32.325-29.112c-16.971,0.877-30.012,15.365-29.119,32.327l15.919,336.895c1.61,10.121-5.294,19.623-15.415,21.234c-10.129,1.611-19.631-5.279-21.242-15.408l-29.356-119.524c-4.036-16.516-20.688-26.615-37.19-22.587c-16.509,4.042-26.615,20.695-22.58,37.197l48.714,199.186c0.899,3.652,70.445,127.939,95.802,133.902v69.174c0,22.127,17.935,40.061,40.054,40.061h136.235c22.126,0,40.061-17.934,40.061-40.061v-75.789c35.88-45.924,49.54-132.113,50.116-206.328l17.407-248.525C588.87,158.612,584.122,145.066,567.52,142.808z"/>
<circle class="finger-hotspot" id="f4" cx="190" cy="300" r="35" onclick="selectFinger(4)"/><text class="finger-label" x="140" y="310">Thumb</text>
<circle class="finger-hotspot" id="f0" cx="300" cy="63" r="25" onclick="selectFinger(0)"/><text class="finger-label" x="300" y="50">Index</text>
<circle class="finger-hotspot" id="f1" cx="392" cy="25" r="25" onclick="selectFinger(1)"/><text class="finger-label" x="392" y="75">Middle</text>
<circle class="finger-hotspot" id="f2" cx="483" cy="60" r="25" onclick="selectFinger(2)"/><text class="finger-label" x="540" y="50">Ring</text>
<circle class="finger-hotspot" id="f3" cx="565" cy="170" r="25" onclick="selectFinger(3)"/><text class="finger-label" x="620" y="180">Pinky</text>
</svg>
<svg class="hand-svg hidden" id="rightHand" viewBox="0 0 746 746" style="transform:scaleX(-1)"><path class="hand-outline" d="M567.52,142.808c-18.1-2.46-27.659,12.74-28.846,29.688l-13.495,134.416c-0.252,6.244-5.518,11.114-11.769,10.862c-6.244-0.245-11.106-5.51-10.861-11.768l8.186-240.043c0.598-16.984-12.675-31.234-29.657-31.838c-16.984-0.604-31.234,12.675-31.847,29.651l-7.999,238.13c0,5.036-4.085,9.114-9.114,9.114c-5.034,0-9.112-4.079-9.112-9.114l-0.008-271.147C422.998,13.776,409.222,0,392.231,0s-30.767,13.775-30.767,30.759l-0.224,266.091c0,5.719-4.633,10.322-10.357,10.351c-8.366,0.043-10.014-7.826-10.014-7.826c-0.201-0.816-10.258-236.442-10.258-236.442c-0.885-16.969-15.363-30.004-32.325-29.112c-16.971,0.877-30.012,15.365-29.119,32.327l15.919,336.895c1.61,10.121-5.294,19.623-15.415,21.234c-10.129,1.611-19.631-5.279-21.242-15.408l-29.356-119.524c-4.036-16.516-20.688-26.615-37.19-22.587c-16.509,4.042-26.615,20.695-22.58,37.197l48.714,199.186c0.899,3.652,70.445,127.939,95.802,133.902v69.174c0,22.127,17.935,40.061,40.054,40.061h136.235c22.126,0,40.061-17.934,40.061-40.061v-75.789c35.88-45.924,49.54-132.113,50.116-206.328l17.407-248.525C588.87,158.612,584.122,145.066,567.52,142.808z"/>
<circle class="finger-hotspot" id="f9" cx="190" cy="300" r="35" onclick="selectFinger(9)"/><text class="finger-label" x="140" y="310" style="transform:scaleX(-1);transform-origin:140px 310px">Thumb</text>
<circle class="finger-hotspot" id="f5" cx="300" cy="63" r="25" onclick="selectFinger(5)"/><text class="finger-label" x="300" y="50" style="transform:scaleX(-1);transform-origin:300px 50px">Index</text>
<circle class="finger-hotspot" id="f6" cx="392" cy="25" r="25" onclick="selectFinger(6)"/><text class="finger-label" x="392" y="75" style="transform:scaleX(-1);transform-origin:392px 75px">Middle</text>
<circle class="finger-hotspot" id="f7" cx="483" cy="60" r="25" onclick="selectFinger(7)"/><text class="finger-label" x="540" y="50" style="transform:scaleX(-1);transform-origin:540px 50px">Ring</text>
<circle class="finger-hotspot" id="f8" cx="565" cy="170" r="25" onclick="selectFinger(8)"/><text class="finger-label" x="620" y="180" style="transform:scaleX(-1);transform-origin:620px 180px">Pinky</text>
</svg>
    </div>
  </div>
  <div class="finger-panel" id="fingerPanel">
    <div class="panel-header"><span class="panel-title" id="panelTitle">--</span><span class="panel-status" id="panelStatus">Not enrolled</span></div>
    <div class="panel-actions" id="panelActions"><button class="btn btn-primary btn-sm" onclick="startEnrollSelected()">Enroll</button></div>
  </div>
</div>

<div class="card">
  <div class="card-header"><span class="card-title">Credentials</span></div>
  <div class="credentials-list" id="credentialsList">
    <div class="empty-state">
      <svg class="empty-state-icon" viewBox="0 0 24 24" fill="currentColor"><path d="M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm1 15h-2v-6h2v6zm0-8h-2V7h2v2z"/></svg>
      <p>No fingers enrolled yet</p>
      <p class="text-sm mt-2">Tap a finger above to enroll</p>
    </div>
  </div>
</div>

<div class="card">
  <div class="card-header"><span class="card-title">Keyboard Mode</span><span class="card-badge" id="keyboardModeBadge">--</span></div>
  <div id="keyboardModeContent" style="padding:12px 0">
    <p style="font-size:13px;color:var(--ts);margin-bottom:12px">Chip: <strong id="chipModel">--</strong></p>
    <div class="form-group">
      <label class="form-label">Mode</label>
      <select class="form-input" id="keyboardModeSelect">
        <option value="auto">Auto (Default)</option>
        <option value="ble">BLE Keyboard</option>
      </select>
    </div>
    <button class="btn btn-primary btn-block" onclick="saveKeyboardMode()">Save & Reboot</button>
    <p class="text-sm mt-2" style="color:var(--tm)">Changing mode requires reboot</p>
  </div>
</div>

<div class="card">
  <div class="card-header"><span class="card-title">Activity</span></div>
  <div class="log-container" id="logContainer"></div>
  <button onclick="runDiagnostics()" class="btn btn-secondary btn-block" style="margin-top:12px">
    Run Diagnostics
  </button>
</div>
</div>

<!-- Modals (same as original) -->
<div class="modal-overlay" id="enrollModal">
  <div class="modal">
    <div class="modal-header">
      <h3 class="modal-title">Enroll Finger</h3>
      <p class="modal-subtitle" id="enrollModalSubtitle">Set up a new fingerprint</p>
    </div>
    <div class="modal-body" id="enrollStep1">
      <div class="form-group">
        <label class="form-label">Name</label>
        <input type="text" class="form-input" id="enrollName" placeholder="e.g. GitHub" maxlength="20">
      </div>
      <div class="form-group">
        <label class="form-label">Password</label>
        <div class="password-wrapper">
          <input type="password" class="form-input" id="enrollPassword" placeholder="Password to type" maxlength="64">
          <button class="password-toggle" onclick="togglePwd('enrollPassword')">Show</button>
        </div>
      </div>
      <div class="checkbox-group">
        <input type="checkbox" class="checkbox" id="enrollEnter">
        <label class="checkbox-label" for="enrollEnter">Press Enter after typing</label>
      </div>
    </div>
    <div class="modal-body hidden" id="enrollStep2">
      <div class="fingerprint-container">
        <svg class="fingerprint-svg" viewBox="0 0 123 122">
<path class="fp-seg" id="ridge1" d="M59.44,62.15C59.2,60.86 60.06,59.62 61.35,59.39C62.64,59.15 63.88,60.01 64.11,61.3C65.32,67.82 65.9,74.41 65.92,81.03C65.94,87.62 65.4,94.24 64.38,100.87C64.18,102.16 62.97,103.05 61.67,102.85C60.38,102.65 59.49,101.44 59.69,100.14C60.68,93.76 61.2,87.38 61.18,81.03C61.16,74.7 60.6,68.4 59.44,62.15Z"/>
<path class="fp-seg" id="ridge2" d="M56.72,108.31C57,107.03 58.27,106.22 59.55,106.51C60.83,106.79 61.64,108.06 61.35,109.34L59.27,118.71C58.99,119.99 57.72,120.8 56.44,120.51C55.16,120.23 54.35,118.96 54.64,117.68L56.72,108.31Z M43.15,119.69C42.39,120.76 40.91,121.02 39.84,120.26C38.77,119.5 38.51,118.02 39.27,116.95C40.95,114.58 42.37,111.98 43.53,109.16C44.69,106.32 45.59,103.23 46.21,99.89C47.37,93.66 46.82,85.72 46.29,78.22C46.11,75.69 45.94,73.22 45.83,70.68C45.67,66.97 45.6,63.33 46.29,60.02C47.04,56.41 48.66,53.28 51.91,51C52.64,50.49 53.43,50.03 54.28,49.64C55.13,49.25 56.05,48.9 57.04,48.62C57.78,48.41 58.51,48.24 59.22,48.12C64.09,47.29 67.95,48.55 70.93,51.15C73.76,53.62 75.68,57.26 76.86,61.39C77.21,62.63 77.5,63.92 77.73,65.24C77.85,65.96 77.96,66.69 78.04,67.42C78.12,68.14 78.19,68.88 78.24,69.64C78.74,77.18 78.41,86.59 77.39,95.8C76.41,104.63 74.79,113.33 72.64,120.1C72.24,121.35 70.91,122.04 69.66,121.65C68.41,121.25 67.72,119.92 68.11,118.67C70.15,112.24 71.7,103.86 72.65,95.29C73.64,86.34 73.96,77.23 73.48,69.95C73.44,69.32 73.38,68.66 73.3,67.98C73.23,67.34 73.13,66.7 73.02,66.07C72.82,64.93 72.57,63.81 72.27,62.72C71.33,59.41 69.87,56.57 67.79,54.76C65.87,53.09 63.32,52.29 60.02,52.85C59.49,52.94 58.93,53.07 58.34,53.24C57.57,53.46 56.88,53.71 56.27,54C55.65,54.29 55.1,54.6 54.63,54.93C52.54,56.39 51.47,58.52 50.95,61.02C50.37,63.81 50.44,67.12 50.58,70.51C50.68,72.76 50.86,75.32 51.04,77.94C51.58,85.72 52.16,93.95 50.88,100.79C50.2,104.44 49.21,107.83 47.92,110.99C46.64,114.11 45.04,117.02 43.15,119.69Z"/>
<path class="fp-seg" id="ridge3" d="M72.44,44.51C71.32,43.83 70.97,42.36 71.65,41.25C72.33,40.13 73.79,39.78 74.91,40.46C75.64,40.91 76.35,41.39 77.04,41.91C77.72,42.42 78.39,42.98 79.06,43.59C88.12,51.82 91.19,65.05 91.39,78.89C91.58,92.37 89.05,106.43 86.73,116.8C86.45,118.08 85.18,118.89 83.9,118.6C82.62,118.32 81.81,117.05 82.1,115.77C84.36,105.65 86.84,91.96 86.65,78.94C86.47,66.28 83.77,54.29 75.86,47.1C75.31,46.6 74.74,46.13 74.17,45.7C73.6,45.25 73.02,44.86 72.44,44.51Z M38.95,98.15C38.72,99.44 37.49,100.31 36.2,100.08C34.91,99.85 34.04,98.62 34.27,97.33C34.82,94.25 35.13,90.8 35.24,87.04C35.35,83.23 35.26,79.06 35,74.6C34.95,73.64 34.86,72.45 34.78,71.29C34.24,63.7 33.81,57.58 38.25,49.69C39.23,47.95 40.4,46.33 41.8,44.85C43.2,43.37 44.81,42.03 46.64,40.86C46.76,40.78 46.88,40.72 47.01,40.67C49.72,39.37 52.41,38.36 55.1,37.7C57.87,37.02 60.62,36.72 63.33,36.87C64.64,36.94 65.65,38.05 65.58,39.36C65.51,40.67 64.4,41.68 63.09,41.61C60.84,41.49 58.55,41.74 56.22,42.31C53.9,42.88 51.52,43.77 49.1,44.93C47.63,45.88 46.35,46.94 45.26,48.1C44.14,49.29 43.19,50.59 42.4,52.01C38.66,58.67 39.04,64.15 39.52,70.95C39.59,71.95 39.66,72.99 39.74,74.33C40,78.87 40.09,83.17 39.98,87.16C39.85,91.22 39.53,94.9 38.95,98.15Z"/>
<path class="fp-seg" id="ridge4" d="M44.18,34.97C43.04,35.63 41.59,35.24 40.93,34.1C40.27,32.96 40.66,31.51 41.8,30.85C44.46,29.31 47.3,28.09 50.23,27.2C58.27,24.76 67,24.83 74.94,27.61C82.92,30.41 90.09,35.93 94.97,44.38C96.6,47.19 97.97,50.34 99.03,53.83C100.54,58.81 101.57,64.37 102.1,70.48C102.62,76.49 102.65,83.09 102.19,90.27C102.19,90.34 102.18,90.4 102.17,90.47L100.51,106.85C100.38,108.15 99.22,109.11 97.91,108.98C96.61,108.85 95.65,107.69 95.78,106.38L97.44,89.98L97.44,89.97C97.88,83.06 97.85,76.68 97.35,70.87C96.85,65.05 95.89,59.82 94.49,55.21C93.55,52.1 92.32,49.29 90.86,46.76C86.59,39.36 80.33,34.53 73.38,32.09C66.39,29.64 58.7,29.58 51.61,31.73C49.04,32.53 46.55,33.6 44.18,34.97Z M20.01,106.56C19.03,107.43 17.53,107.34 16.66,106.36C15.79,105.38 15.88,103.88 16.86,103.01C19.81,100.41 21.96,97.05 23.2,92.84C24.48,88.5 24.8,83.23 24.05,76.92C22.67,70.14 22.42,63.88 23.23,58.23C24.07,52.32 26.07,47.08 29.12,42.6C29.86,41.52 31.34,41.24 32.42,41.98C33.5,42.72 33.78,44.2 33.04,45.28C30.4,49.15 28.67,53.72 27.93,58.9C27.2,64.03 27.43,69.74 28.7,75.97C28.73,76.09 28.76,76.21 28.77,76.33C29.61,83.3 29.22,89.21 27.76,94.18C26.26,99.28 23.63,103.36 20.01,106.56Z"/>
<path class="fp-seg" id="ridge5" d="M12.97,96.87C11.98,97.73 10.48,97.62 9.62,96.63C8.76,95.64 8.87,94.14 9.86,93.28C10.46,92.76 10.99,92.14 11.44,91.42C11.91,90.68 12.3,89.81 12.61,88.82C14.2,83.76 13.24,78.48 12.27,73.13C11.61,69.49 10.94,65.82 10.97,61.99C11.09,42.39 23.35,26.41 39.39,18.22C46.12,14.78 53.54,12.72 61.02,12.33C68.53,11.94 76.1,13.26 83.09,16.58C98.7,24 111.38,41.37 114.02,72.13C114.13,73.44 113.15,74.59 111.84,74.7C110.53,74.81 109.38,73.83 109.27,72.52C106.8,43.73 95.25,27.63 81.06,20.88C74.8,17.9 68.01,16.73 61.26,17.07C54.47,17.42 47.71,19.31 41.55,22.45C26.96,29.89 15.82,44.34 15.71,62C15.69,65.4 16.32,68.86 16.94,72.29C18.02,78.23 19.08,84.09 17.15,90.24C16.72,91.62 16.15,92.86 15.46,93.96C14.76,95.07 13.92,96.04 12.97,96.87Z M109.22,82.01C109.22,80.69 110.29,79.63 111.6,79.63C112.91,79.63 113.98,80.7 113.98,82.01L113.98,91.99C113.98,93.31 112.91,94.37 111.6,94.37C110.29,94.37 109.22,93.3 109.22,91.99L109.22,82.01Z"/>
<path class="fp-seg" id="ridge6" d="M32.18,105.73C32.88,104.62 34.34,104.29 35.45,104.99C36.56,105.69 36.89,107.15 36.19,108.26L31.38,115.94C30.68,117.05 29.22,117.38 28.11,116.68C27,115.98 26.67,114.52 27.37,113.41L32.18,105.73Z M6.57,75.32C6.9,76.59 6.15,77.89 4.88,78.22C3.61,78.55 2.31,77.8 1.98,76.53C1.59,75.03 1.25,73.52 0.96,71.99C0.68,70.47 0.46,68.96 0.3,67.45C0.2,66.47 0.12,65.47 0.08,64.46C0.02,63.37 0,62.36 0,61.44C0,44.47 6.88,29.11 18,18C29.11,6.88 44.47,0 61.44,0C78.41,0 93.78,6.83 104.91,17.91C116.02,28.97 122.88,44.27 122.88,61.21C122.88,62.52 121.81,63.59 120.5,63.59C119.19,63.59 118.12,62.52 118.12,61.21C118.12,45.57 111.79,31.47 101.56,21.28C91.3,11.06 77.12,4.76 61.44,4.76C45.79,4.76 31.62,11.1 21.36,21.36C11.11,31.62 4.76,45.79 4.76,61.44C4.76,62.48 4.78,63.41 4.82,64.24C4.86,65.15 4.93,66.06 5.02,66.97C5.17,68.37 5.37,69.76 5.63,71.14C5.89,72.51 6.2,73.9 6.57,75.32Z"/>
        </svg>
      </div>
      <div class="fingerprint-status">
        <p class="fingerprint-status-text" id="enrollStatusText">Place finger on sensor</p>
        <p class="fingerprint-status-step" id="enrollStatusStep">Step 1 of 6</p>
      </div>
    </div>
    <div class="modal-body hidden" id="enrollStep3">
      <div class="result-container">
        <div class="result-icon" id="enrollResultIcon">✓</div>
        <h3 class="result-title" id="enrollResultTitle">Success!</h3>
        <p class="result-message" id="enrollResultMsg">Fingerprint enrolled</p>
      </div>
    </div>
    <div class="modal-footer" id="enrollFooter1">
      <button class="btn btn-secondary" onclick="closeEnrollModal()">Cancel</button>
      <button class="btn btn-primary" onclick="beginEnroll()">Continue</button>
    </div>
    <div class="modal-footer hidden" id="enrollFooter2">
      <button class="btn btn-secondary btn-block" onclick="cancelEnroll()">Cancel</button>
    </div>
    <div class="modal-footer hidden" id="enrollFooter3">
      <button class="btn btn-primary btn-block" onclick="closeEnrollModal()">Done</button>
    </div>
  </div>
</div>

<div class="modal-overlay" id="editModal">
  <div class="modal">
    <div class="modal-header">
      <h3 class="modal-title">Edit Credential</h3>
      <p class="modal-subtitle" id="editModalSubtitle">Update settings</p>
    </div>
    <div class="modal-body">
      <input type="hidden" id="editId">
      <div class="form-group">
        <label class="form-label">Name</label>
        <input type="text" class="form-input" id="editName" placeholder="Name" maxlength="20">
      </div>
      <div class="form-group">
        <label class="form-label">New Password</label>
        <div class="password-wrapper">
          <input type="password" class="form-input" id="editPassword" placeholder="Leave empty to keep" maxlength="64">
          <button class="password-toggle" onclick="togglePwd('editPassword')">Show</button>
        </div>
      </div>
      <div class="checkbox-group">
        <input type="checkbox" class="checkbox" id="editEnter">
        <label class="checkbox-label" for="editEnter">Press Enter after typing</label>
      </div>
    </div>
    <div class="modal-footer">
      <button class="btn btn-secondary" onclick="closeEditModal()">Cancel</button>
      <button class="btn btn-primary" onclick="saveEdit()">Save</button>
    </div>
  </div>
</div>

<div class="modal-overlay" id="detectModal">
  <div class="modal">
    <div class="modal-header">
      <h3 class="modal-title">Finger Detected</h3>
    </div>
    <div class="modal-body">
      <div class="result-container">
        <div class="result-icon" id="detectResultIcon">✓</div>
        <h3 class="result-title" id="detectResultTitle">--</h3>
        <p class="result-message" id="detectResultMsg">--</p>
      </div>
    </div>
    <div class="modal-footer">
      <button class="btn btn-primary btn-block" onclick="closeDetectModal()">OK</button>
    </div>
  </div>
</div>

<script>
// Check Web Serial API support
if (!('serial' in navigator)) {
  alert('Web Serial API not supported. Please use Chrome 89+ or Edge 89+.');
}

// TouchPass Serial Communication Class
class TouchPassSerial {
  constructor() {
    this.port = null;
    this.reader = null;
    this.writer = null;
    this.readableStreamClosed = null;
    this.writableStreamClosed = null;
    this.commandId = 0;
    this.pendingCommands = new Map();
  }

  async connect() {
    try {
      this.port = await navigator.serial.requestPort();
      await this.port.open({ baudRate: 115200 });

      const textDecoder = new TextDecoderStream();
      this.readableStreamClosed = this.port.readable.pipeTo(textDecoder.writable);
      this.reader = textDecoder.readable.getReader();

      const textEncoder = new TextEncoderStream();
      this.writableStreamClosed = textEncoder.readable.pipeTo(this.port.writable);
      this.writer = textEncoder.writable.getWriter();

      this.readLoop().catch(err => console.error('Read loop error:', err));
      return true;
    } catch (error) {
      console.error('Connection failed:', error);
      throw error;
    }
  }

  async sendCommand(cmd, params = {}) {
    const id = ++this.commandId;
    const message = JSON.stringify({ cmd, params, id }) + '\n';

    return new Promise((resolve, reject) => {
      const timeout = setTimeout(() => {
        this.pendingCommands.delete(id);
        reject(new Error('Command timeout'));
      }, 5000);

      this.pendingCommands.set(id, { resolve, reject, timeout });
      this.writer.write(message);
    });
  }

  async readLoop() {
    let buffer = '';
    try {
      while (true) {
        const { value, done } = await this.reader.read();
        if (done) break;

        buffer += value;
        const lines = buffer.split('\n');
        buffer = lines.pop();

        for (const line of lines) {
          if (line.trim()) {
            try {
              const parsed = JSON.parse(line);
              this.handleResponse(parsed);
            } catch (e) {
              console.error('Parse error:', line);
            }
          }
        }
      }
    } catch (error) {
      console.error('Read error:', error);
      this.handleDisconnect();
    }
  }

  handleResponse(response) {
    const { id, status, data, message } = response;

    if (this.pendingCommands.has(id)) {
      const { resolve, reject, timeout } = this.pendingCommands.get(id);
      clearTimeout(timeout);
      this.pendingCommands.delete(id);

      if (status === 'ok') {
        resolve(data);
      } else {
        reject(new Error(message || 'Command failed'));
      }
    }
  }

  handleDisconnect() {
    document.getElementById('connectionStatus').textContent = 'Disconnected';
    document.getElementById('connectionStatus').className = 'connection-status disconnected';
    log('Disconnected', 'error');
  }

  async disconnect() {
    try {
      if (this.reader) {
        await this.reader.cancel();
        await this.readableStreamClosed.catch(() => {});
      }
      if (this.writer) {
        await this.writer.close();
        await this.writableStreamClosed;
      }
      if (this.port) {
        await this.port.close();
      }
    } catch (e) {
      console.error('Disconnect error:', e);
    }
  }
}

// Global state
let serial = null;
let selectedFinger = null;
let currentHand = 'left';
let enrolledFingers = {};
let enrollPollInterval = null;
let detectPollInterval = null;

const fingerNames = {
  0:'Left Index', 1:'Left Middle', 2:'Left Ring', 3:'Left Pinky', 4:'Left Thumb',
  5:'Right Index', 6:'Right Middle', 7:'Right Ring', 8:'Right Pinky', 9:'Right Thumb'
};

// API wrapper (replaces fetch with serial commands)
async function api(cmd, params = {}) {
  try {
    return await serial.sendCommand(cmd, params);
  } catch (e) {
    log('Error: ' + e.message, 'error');
    throw e;
  }
}

function log(msg, type='info') {
  const container = document.getElementById('logContainer');
  const time = new Date().toLocaleTimeString('en-US', {hour12:false});
  container.innerHTML = `<div class="log-entry"><span class="log-time">${time}</span><span class="log-msg ${type}">${msg}</span></div>` + container.innerHTML;
}

// Connect to device
async function connectDevice() {
  try {
    serial = new TouchPassSerial();
    await serial.connect();

    document.getElementById('connectScreen').classList.add('hidden');
    document.getElementById('mainApp').classList.remove('hidden');

    log('Connected', 'success');

    await refreshStatus();
    await refreshBle();
    await loadSystemInfo();

    setInterval(refreshStatus, 10000);
    setInterval(refreshBle, 3000);
    startDetectionPolling();
  } catch (error) {
    console.error('Connection failed:', error);
    alert('Failed to connect: ' + error.message);
  }
}

// Status and BLE refresh
async function refreshStatus() {
  const status = await api('get_status');
  if (status) {
    document.getElementById('sensorDot').className = 'status-dot' + (status.sensor ? ' active' : ' error');
    document.getElementById('sensorIndicator').className = 'indicator' + (status.sensor ? ' success' : ' error');
    document.getElementById('sensorStatus').textContent = 'Sensor: ' + (status.sensor ? 'OK' : 'Error');
    document.getElementById('countStatus').textContent = (status.count || 0) + '/200';
    document.getElementById('enrolledBadge').textContent = (status.count || 0) + ' enrolled';
  }
  await loadFingers();
}

async function refreshBle() {
  const status = await api('get_ble_status');
  const connected = status && status.connected;
  const mode = status && status.mode ? status.mode : 'BLE';
  document.getElementById('bleDot').className = 'status-dot' + (connected ? ' active' : '');
  document.getElementById('bleIndicator').className = 'indicator' + (connected ? ' success' : '');
  document.getElementById('bleStatus').textContent = mode + ': ' + (connected ? 'Connected' : 'Waiting');
}

// Finger management
function showHand(hand) {
  currentHand = hand;
  document.getElementById('leftHand').classList.toggle('hidden', hand !== 'left');
  document.getElementById('rightHand').classList.toggle('hidden', hand !== 'right');
  document.getElementById('leftHandBtn').classList.toggle('active', hand === 'left');
  document.getElementById('rightHandBtn').classList.toggle('active', hand === 'right');
  hideFingerPanel();
}

function selectFinger(id) {
  selectedFinger = id;
  document.querySelectorAll('.finger-hotspot').forEach(e => e.classList.remove('selected'));
  document.getElementById('f' + id).classList.add('selected');
  document.getElementById('fingerPanel').classList.add('active');
  document.getElementById('panelTitle').textContent = fingerNames[id];

  const finger = enrolledFingers[id];
  if (finger) {
    document.getElementById('panelStatus').textContent = finger.hasPassword ? 'Password set' : 'No password';
    document.getElementById('panelStatus').className = 'panel-status' + (finger.hasPassword ? ' has-pwd' : '');
    document.getElementById('panelActions').innerHTML =
      `<button class="btn btn-secondary btn-sm" onclick="editFinger(${finger.id})">Edit</button>` +
      `<button class="btn btn-danger btn-sm" onclick="deleteFinger(${finger.id},'${finger.name}')">Delete</button>`;
  } else {
    document.getElementById('panelStatus').textContent = 'Not enrolled';
    document.getElementById('panelStatus').className = 'panel-status';
    document.getElementById('panelActions').innerHTML = '<button class="btn btn-primary btn-sm" onclick="startEnrollSelected()">Enroll</button>';
  }
}

function hideFingerPanel() {
  document.getElementById('fingerPanel').classList.remove('active');
  document.querySelectorAll('.finger-hotspot').forEach(e => e.classList.remove('selected'));
  selectedFinger = null;
}

async function loadFingers() {
  const result = await api('get_fingers');
  const list = document.getElementById('credentialsList');
  enrolledFingers = {};

  // Clear all configured markers
  for (let i = 0; i < 10; i++) {
    const el = document.getElementById('f' + i);
    if (el) el.classList.remove('configured');
  }

  if (!result.fingers || !result.fingers.length) {
    list.innerHTML = '<div class="empty-state"><svg class="empty-state-icon" viewBox="0 0 24 24" fill="currentColor"><path d="M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm1 15h-2v-6h2v6zm0-8h-2V7h2v2z"/></svg><p>No fingers enrolled</p><p class="text-sm mt-2">Tap a finger to start</p></div>';
    return;
  }

  let html = '';
  for (const finger of result.fingers) {
    const detail = await api('get_finger', { id: finger.id });
    const fid = finger.fingerId >= 0 ? finger.fingerId : finger.id;
    enrolledFingers[fid] = { id: finger.id, name: finger.name, hasPassword: detail.hasPassword, pressEnter: detail.pressEnter, fingerId: fid };

    const fc = document.getElementById('f' + fid);
    if (fc) fc.classList.add('configured');

    html += `<div class="credential-card">
      <div class="credential-info">
        <div class="credential-name">${finger.name}</div>
        <div class="credential-meta">
          ${detail.hasPassword ? '<span style="color:var(--ok)">●</span> Password' : '<span style="color:var(--tm)">○</span> No password'}
          ${detail.pressEnter ? ' ↵' : ''} • ${fingerNames[fid] || 'Unknown'}
        </div>
      </div>
      <div class="credential-actions" onclick="event.stopPropagation()">
        <button class="icon-btn" onclick="editFinger(${finger.id})">
          <svg width="16" height="16" viewBox="0 0 24 24" fill="currentColor"><path d="M3 17.25V21h3.75L17.81 9.94l-3.75-3.75L3 17.25zM20.71 7.04c.39-.39.39-1.02 0-1.41l-2.34-2.34c-.39-.39-1.02-.39-1.41 0l-1.83 1.83 3.75 3.75 1.83-1.83z"/></svg>
        </button>
        <button class="icon-btn danger" onclick="deleteFinger(${finger.id},'${finger.name}')">
          <svg width="16" height="16" viewBox="0 0 24 24" fill="currentColor"><path d="M6 19c0 1.1.9 2 2 2h8c1.1 0 2-.9 2-2V7H6v12zM19 4h-3.5l-1-1h-5l-1 1H5v2h14V4z"/></svg>
        </button>
      </div>
    </div>`;
  }
  list.innerHTML = html;
}

function togglePwd(id) {
  const input = document.getElementById(id);
  const btn = input.nextElementSibling;
  if (input.type === 'password') {
    input.type = 'text';
    btn.textContent = 'Hide';
  } else {
    input.type = 'password';
    btn.textContent = 'Show';
  }
}

// Enrollment
function startEnrollSelected() {
  if (selectedFinger !== null) {
    document.getElementById('enrollModalSubtitle').textContent = fingerNames[selectedFinger];
    openEnrollModal();
  }
}

function openEnrollModal() {
  document.getElementById('enrollName').value = '';
  document.getElementById('enrollPassword').value = '';
  document.getElementById('enrollEnter').checked = false;
  showEnrollStep(1);
  document.getElementById('enrollModal').classList.add('active');
  document.getElementById('enrollName').focus();
  stopDetectionPolling();
}

function closeEnrollModal() {
  document.getElementById('enrollModal').classList.remove('active');
  if (enrollPollInterval) clearInterval(enrollPollInterval);
  api('enroll_cancel');
  resetFp();
  startDetectionPolling();
}

function showEnrollStep(step) {
  document.getElementById('enrollStep1').classList.toggle('hidden', step !== 1);
  document.getElementById('enrollStep2').classList.toggle('hidden', step !== 2);
  document.getElementById('enrollStep3').classList.toggle('hidden', step !== 3);
  document.getElementById('enrollFooter1').classList.toggle('hidden', step !== 1);
  document.getElementById('enrollFooter2').classList.toggle('hidden', step !== 2);
  document.getElementById('enrollFooter3').classList.toggle('hidden', step !== 3);
}

async function beginEnroll() {
  const name = document.getElementById('enrollName').value.trim();
  if (!name) {
    document.getElementById('enrollName').focus();
    return;
  }

  const password = document.getElementById('enrollPassword').value;
  const pressEnter = document.getElementById('enrollEnter').checked;

  showEnrollStep(2);
  document.getElementById('enrollStatusText').textContent = 'Place finger on sensor';
  document.getElementById('enrollStatusStep').textContent = 'Step 1 of 6';
  resetFp();

  try {
    const result = await api('enroll_start', { name, password, pressEnter, finger: selectedFinger });
    if (result.ok) {
      enrollPollInterval = setInterval(pollEnroll, 400);
    } else {
      showEnrollResult(false, 'Error', result.status || 'Failed');
    }
  } catch (e) {
    showEnrollResult(false, 'Error', e.message);
  }
}

async function pollEnroll() {
  const status = await api('enroll_status');
  if (status.step > 0 && status.step <= 6) {
    document.getElementById('enrollStatusStep').textContent = 'Step ' + status.step + ' of 6';
    if (status.message) {
      document.getElementById('enrollStatusText').textContent = status.message;
    }
    if (status.captured) {
      animateFp(status.step);
    }
  }

  if (status.done) {
    clearInterval(enrollPollInterval);
    if (status.ok) {
      animateFpOk();
      setTimeout(() => {
        showEnrollResult(true, 'Enrolled!', status.name + ' saved');
        log(status.name + ' enrolled', 'success');
      }, 500);
    } else {
      animateFpErr();
      setTimeout(() => {
        showEnrollResult(false, 'Failed', status.status || 'Error');
        log('Enrollment failed', 'error');
      }, 500);
    }
  }
}

function cancelEnroll() {
  closeEnrollModal();
  log('Cancelled', 'info');
}

async function showEnrollResult(success, title, msg) {
  showEnrollStep(3);
  const icon = document.getElementById('enrollResultIcon');
  icon.textContent = success ? '✓' : '✕';
  icon.className = 'result-icon ' + (success ? 'success' : 'error');
  document.getElementById('enrollResultTitle').textContent = title;
  document.getElementById('enrollResultMsg').textContent = msg;
  await refreshStatus();
  if (success && selectedFinger !== null) {
    selectFinger(selectedFinger);
  }
}

function resetFp() {
  for (let i = 1; i <= 6; i++) {
    const el = document.getElementById('ridge' + i);
    if (el) el.classList.remove('active', 'success', 'error');
  }
}

function animateFp(step) {
  for (let i = 1; i <= step && i <= 6; i++) {
    const el = document.getElementById('ridge' + i);
    if (el) el.classList.add('active');
  }
}

function animateFpOk() {
  for (let i = 1; i <= 6; i++) {
    const el = document.getElementById('ridge' + i);
    if (el) {
      el.classList.remove('active');
      el.classList.add('success');
    }
  }
}

function animateFpErr() {
  for (let i = 1; i <= 6; i++) {
    const el = document.getElementById('ridge' + i);
    if (el) {
      el.classList.remove('active');
      el.classList.add('error');
    }
  }
}

// Edit
async function editFinger(id) {
  stopDetectionPolling();
  const finger = await api('get_finger', { id });
  if (finger.ok) {
    document.getElementById('editId').value = id;
    document.getElementById('editName').value = finger.name;
    document.getElementById('editPassword').value = '';
    document.getElementById('editEnter').checked = finger.pressEnter;
    document.getElementById('editModalSubtitle').textContent = finger.name;
    document.getElementById('editModal').classList.add('active');
  }
}

function closeEditModal() {
  document.getElementById('editModal').classList.remove('active');
  startDetectionPolling();
}

async function saveEdit() {
  const id = document.getElementById('editId').value;
  const name = document.getElementById('editName').value.trim();
  const password = document.getElementById('editPassword').value;
  const pressEnter = document.getElementById('editEnter').checked;

  if (!name) {
    document.getElementById('editName').focus();
    return;
  }

  const params = { id: parseInt(id), name, pressEnter };
  if (password) params.password = password;

  try {
    const result = await api('update_finger', params);
    if (result.ok) {
      log('Updated ' + name, 'success');
      closeEditModal();
      refreshStatus();
    } else {
      log('Update failed', 'error');
    }
  } catch (e) {
    log('Update error: ' + e.message, 'error');
  }
}

// Delete
async function deleteFinger(id, name) {
  if (confirm('Delete "' + name + '"?')) {
    try {
      const result = await api('delete_finger', { id });
      if (result.ok) {
        log('Deleted ' + name, 'success');
        hideFingerPanel();
        refreshStatus();
      } else {
        log('Delete failed', 'error');
      }
    } catch (e) {
      log('Delete error: ' + e.message, 'error');
    }
  }
}

// Detection polling
async function pollDetection() {
  const result = await api('get_detect');
  if (result.detected) {
    stopDetectionPolling();
    showDetectionResult(result.matched, result.finger || 'Unknown', result.score || 0);
  }
}

function showDetectionResult(success, name, score) {
  const icon = document.getElementById('detectResultIcon');
  icon.textContent = success ? '✓' : '✕';
  icon.className = 'result-icon ' + (success ? 'success' : 'error');
  document.getElementById('detectResultTitle').textContent = success ? name : 'Unknown';
  document.getElementById('detectResultMsg').textContent = success ? 'Password typed • Score: ' + score : 'Not enrolled';
  document.getElementById('detectModal').classList.add('active');
  log(success ? name + ' (' + score + ')' : 'Unknown finger', success ? 'success' : 'error');
}

function closeDetectModal() {
  document.getElementById('detectModal').classList.remove('active');
  startDetectionPolling();
}

function startDetectionPolling() {
  if (!detectPollInterval) {
    detectPollInterval = setInterval(pollDetection, 1000);
  }
}

function stopDetectionPolling() {
  if (detectPollInterval) {
    clearInterval(detectPollInterval);
    detectPollInterval = null;
  }
}

// Keyboard mode
async function loadSystemInfo() {
  const info = await api('get_system_info');
  if (info) {
    document.getElementById('chipModel').textContent = info.chip;
    const mode = await api('get_keyboard_mode');
    if (mode) {
      document.getElementById('keyboardModeBadge').textContent = mode.current;
      const select = document.getElementById('keyboardModeSelect');
      select.innerHTML = '<option value="auto">Auto (Default)</option>';
      if (mode.availableModes.includes('BLE')) {
        select.innerHTML += '<option value="ble">BLE Keyboard</option>';
      }
      if (mode.availableModes.includes('USB-HID')) {
        select.innerHTML += '<option value="usb">USB-HID Keyboard</option>';
      }
      const saved = mode.saved.toLowerCase();
      select.value = saved === 'usb-hid' ? 'usb' : saved;
    }
  }
}

async function saveKeyboardMode() {
  const mode = document.getElementById('keyboardModeSelect').value;
  try {
    const result = await api('set_keyboard_mode', { mode });
    if (result.ok) {
      if (confirm('Keyboard mode saved. Reboot now to apply?')) {
        await api('reboot');
        setTimeout(() => {
          alert('Device rebooting... Reconnect after reboot.');
        }, 500);
      } else {
        log('Mode saved, reboot to apply', 'info');
      }
    } else {
      log('Failed to save mode: ' + (result.error || 'Unknown error'), 'error');
    }
  } catch (e) {
    log('Save error: ' + e.message, 'error');
  }
}

async function runDiagnostics() {
  try {
    log('Running diagnostics...');
    const result = await api('diagnostics');
    console.log('Diagnostics:', result);

    // Display in activity log
    log(`UART1: RX=GPIO${result.uart1.rxPin}, TX=GPIO${result.uart1.txPin}, Baud=${result.uart1.baud}`);
    log(`USB: HID=${result.usb.hid}, Serial=${result.usb.serial}, Mode=${result.usb.mode}`);
    log(`Sensor: Connected=${result.sensor.connected}, Library=${result.sensor.library}`);
    log(`Chip: ${result.chip.model}, ${result.chip.cores} cores, ${result.chip.freeHeap} bytes free`);

    alert('Diagnostics complete - check activity log');
  } catch (err) {
    log(`Diagnostics error: ${err.message}`, 'error');
  }
}
</script>
</body>
</html>

)rawliteral";

#endif
