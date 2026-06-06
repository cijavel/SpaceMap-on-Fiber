#include "SettingsWebServer.h"
#include "AppConfig.h"
#include "Configuration.h"
#include "DataSpaceList.h"
#include "NeoPixelLED.h"
#include "WebClientHandler.h"
#include <ArduinoJson.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

// Live-Statusliste aus main.cpp – wird für die LED-Wiederherstellung nach dem Blinken benötigt.
extern std::vector<SpaceStatusList> spaceStatusList;
// Mutex that guards all access to spaceStatusList (defined in main.cpp).
extern SemaphoreHandle_t spaceStatusMutex;

// Definition of the static guard flag declared in SettingsWebServer.h.
// Reset to false by the blink task itself once it finishes (or by the
// handler if xTaskCreate fails to start the task).
std::atomic<bool> SettingsWebServer::_blinkTaskRunning{false};

// ---------------------------------------------------------------------------
// Static PROGMEM blocks — keep large, read-only text out of DRAM entirely.
// Each block is streamed once per request; the ESP32 never needs to hold
// more than one chunk in heap at the same time.
// ---------------------------------------------------------------------------

// Shared CSS (streamed by streamHtmlHeader).
static const char CSS[] PROGMEM = R"css(
body{font-family:sans-serif;margin:0;background:#111;color:#eee;}
nav{background:#1a1a1a;padding:10px 20px;border-bottom:2px solid #e02020;display:flex;align-items:center;justify-content:center;position:relative;}
nav a{color:#2dbe60;text-decoration:none;margin-right:16px;font-weight:bold;}
nav a:hover{color:#fff;text-decoration:underline;}
.nav-links{display:flex;align-items:center;}
#themeToggle{position:absolute;right:20px;background:none;border:1px solid #444;border-radius:6px;
color:#eee;cursor:pointer;font-size:1.1em;padding:3px 10px;line-height:1.4;}
#themeToggle:hover{border-color:#2dbe60;}
.container{max-width:700px;margin:30px auto;padding:0 16px;}
h1{color:#1a7a3c;}
h2{color:#f5a800;}
label{display:block;margin-top:14px;font-size:0.9em;color:#bbb;}
input[type=text],input[type=number],input[type=url]
{width:100%;padding:7px;background:#1e1e1e;color:#eee;border:1px solid #3a3a3a;
border-radius:4px;box-sizing:border-box;margin-top:4px;}
input:focus{outline:2px solid #1a7a3c;border-color:#1a7a3c;}
.btn{display:inline-block;margin-top:20px;padding:10px 24px;
border:none;border-radius:5px;cursor:pointer;font-size:1em;font-weight:bold;}
.btn-save{background:#1a7a3c;color:#fff;}
.btn-save:hover{background:#2d9e55;}
.btn-reset{background:#f5a800;color:#fff;margin-left:12px;}
.btn-reset:hover{background:#ffc030;}
.btn-settings{display:inline-block;margin-top:20px;padding:10px 24px;
background:#1a7a3c;color:#fff;border:none;border-radius:5px;cursor:pointer;
font-size:1em;font-weight:bold;text-decoration:none;}
.btn-settings:hover{background:#24a050;}
.msg{margin-top:14px;padding:10px;background:#0e2e0e;border-left:4px solid #1a7a3c;
border-radius:4px;color:#6fcf6f;}
.msg-reset{background:#2e0e0e;border-left:4px solid #f5a800;color:#f88;}
/* --- Light mode overrides --- */
body.light{background:#f4f4f4;color:#111;}
body.light nav{background:#e8e8e8;border-bottom-color:#c0392b;}
body.light nav a{color:#1a7a3c;}
body.light nav a:hover{color:#000;}
body.light #themeToggle{border-color:#999;color:#111;}
body.light #themeToggle:hover{border-color:#1a7a3c;}
body.light h1{color:#145c2d;}
body.light h2{color:#b07800;}
body.light label{color:#444;}
body.light input[type=text],body.light input[type=number],body.light input[type=url]
{background:#fff !important;color:#111 !important;border-color:#bbb !important;}
body.light input:focus{outline-color:#1a7a3c;border-color:#1a7a3c;}
body.light .msg{background:#d4edda;color:#155724;}
body.light .msg-reset{background:#fff3cd;color:#856404;}
body.light #mapTable th{color:#145c2d !important;}
body.light #mapTable td{color:#111 !important;}
body.light #mapTable input[type=checkbox]{accent-color:#1a7a3c;}
body.light .btn-blink{background:#2a6aaa !important;}
body.light #unsavedBar{background:#e8f5e9 !important;border-top-color:#1a7a3c !important;
box-shadow:0 -4px 16px #0002 !important;}
body.light #barUnsavedLabel,body.light #barSavedMsg{color:#145c2d !important;}
body.light #saveMsg{background:#d4edda !important;color:#155724 !important;}
body.light .btn-clear{background:#fde8e8 !important;color:#c0392b !important;}
body.light #apiStatus,body.light #parseStatus{background:#f0f0f0 !important;border-color:#ccc !important;}
body.light #apiStatus small,body.light #parseStatus small{color:#555 !important;}
)css";

// JavaScript for /spacemap — large block, lives in flash.
static const char SPACEMAP_JS[] PROGMEM = R"rawjs(
<script>
var rowCount = 0;        // set dynamically before this script runs
var originalMapping = {}; // snapshot of saved state, keyed by LED index
var _savedTimer    = null;
var _inSavedState  = false;

// Escape user-supplied text before inserting it into innerHTML markup.
function escHtml(s){return String(s).replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;').replace(/"/g,'&quot;').replace(/'/g,'&#39;');}
// Escape/unescape backslash and quote for the {n,"name","city"} export format.
function escExport(s){return String(s).replace(/\\/g,'\\\\').replace(/"/g,'\\"');}
function unesc(s){return String(s).replace(/\\(.)/g,'$1');}

function showSavedState() {
    _inSavedState = true;
    var bar          = document.getElementById('unsavedBar');
    var savedMsg     = document.getElementById('barSavedMsg');
    var unsavedLabel = document.getElementById('barUnsavedLabel');
    var discardBtn   = document.getElementById('barDiscard');
    var saveBtn      = document.getElementById('barSave');
    if (savedMsg)     savedMsg.style.display     = '';
    if (unsavedLabel) unsavedLabel.style.display  = 'none';
    if (discardBtn)   discardBtn.style.display    = 'none';
    if (saveBtn)      saveBtn.style.display       = 'none';
    if (bar)          bar.style.display           = 'flex';
    if (_savedTimer)  clearTimeout(_savedTimer);
    _savedTimer = setTimeout(function() {
        if (bar) bar.style.display = 'none';
        _inSavedState = false;
        _savedTimer   = null;
    }, 5000);
}

function showUnsavedState() {
    _inSavedState = false;
    if (_savedTimer) { clearTimeout(_savedTimer); _savedTimer = null; }
    var bar          = document.getElementById('unsavedBar');
    var savedMsg     = document.getElementById('barSavedMsg');
    var unsavedLabel = document.getElementById('barUnsavedLabel');
    var discardBtn   = document.getElementById('barDiscard');
    var saveBtn      = document.getElementById('barSave');
    if (savedMsg)     savedMsg.style.display     = 'none';
    if (unsavedLabel) unsavedLabel.style.display  = '';
    if (discardBtn)   discardBtn.style.display    = '';
    if (saveBtn)      saveBtn.style.display       = '';
    if (bar)          bar.style.display           = 'flex';
}

function initSnapshot() {
    document.querySelectorAll('#mapBody tr').forEach(function(r, idx) {
        var inputs = r.querySelectorAll('input[type=text]');
        var disCb  = r.querySelector('.dis-check');
        originalMapping[idx] = {
            name:     inputs.length > 0 ? inputs[0].value : '',
            city:     inputs.length > 1 ? inputs[1].value : '',
            disabled: disCb ? disCb.checked : false
        };
    });
}

function updateRowDirtyState(row) {
    var ledHidden = row.querySelector('input[type=hidden]');
    var inputs    = row.querySelectorAll('input[type=text]');
    var disCb     = row.querySelector('.dis-check');
    var led       = ledHidden ? parseInt(ledHidden.value) : -1;
    var orig      = originalMapping[led];
    var name      = inputs.length > 0 ? inputs[0].value : '';
    var city      = inputs.length > 1 ? inputs[1].value : '';
    var disabled  = disCb ? disCb.checked : false;
    var isDirty   = !orig || name !== orig.name || city !== orig.city || disabled !== orig.disabled;
    var isLight   = document.body.classList.contains('light');
    if (isDirty) {
        row.style.background = isLight ? '#d4edda' : '#0d1f0d';
        row.style.outline    = '1px solid #1a7a3c';
    } else if (row.style.background === 'rgb(13, 31, 13)' || row.style.background === 'rgb(212, 237, 218)') {
        row.style.background = '';
        row.style.outline    = '';
    }
    var msg = document.getElementById('saveMsg');
    if (msg) msg.style.display = 'none';
    var anyDirty = Array.from(document.querySelectorAll('#mapBody tr')).some(function(r2) {
        var lh = r2.querySelector('input[type=hidden]');
        var ii = r2.querySelectorAll('input[type=text]');
        var cb = r2.querySelector('.dis-check');
        var l  = lh ? parseInt(lh.value) : -1;
        var o  = originalMapping[l];
        if (!o) return true;
        return (ii.length > 0 ? ii[0].value : '') !== o.name
            || (ii.length > 1 ? ii[1].value : '') !== o.city
            || (cb ? cb.checked : false) !== o.disabled;
    });
    if (anyDirty) {
        showUnsavedState();
    } else if (!_inSavedState) {
        var bar = document.getElementById('unsavedBar');
        if (bar) bar.style.display = 'none';
    }
}

function buildRow(i, name, city, disabled) {
    var isEmpty = (name === '' && city === '');
    var rowStyle = isEmpty ? 'opacity:0.45;' : '';
    if (disabled) rowStyle += 'font-style:italic;';
    return '<tr id="row_'+i+'" ondragover="onDragOver(event)" ondrop="onDrop(event)" style="'+rowStyle+'">'
        + '<td draggable="true" ondragstart="onDragStart(event)" ondragend="onDragEnd(event)" aria-label="Zeile verschieben" style="padding:4px;width:24px;color:#888;font-size:1.2em;text-align:center;cursor:grab" title="Drag to reorder">&#8597;</td>'
        + '<td style="padding:4px;text-align:center;color:#eee;font-size:0.9em;min-width:36px"><input type="hidden" name="led_'+i+'" value="'+i+'">'+i+'</td>'
        + '<td style="padding:4px;text-align:center"><input type="checkbox" class="dis-check" name="dis_'+i+'" aria-label="LED deaktivieren" title="LED deaktivieren (bleibt aus)" onchange="onDisabledChange(this)"'+(disabled?' checked':'')+''+'></td>'
        + '<td style="padding:4px;text-align:center"><button type="button" class="btn-blink" onclick="blinkLed(this)" aria-label="LED orten" style="background:#1a4a7a;color:#fff;border:none;border-radius:4px;padding:4px 8px;cursor:pointer" title="LED orten">&#128294;</button></td>'
        + '<td style="padding:4px;text-align:center"><span class="space-status" aria-live="polite" style="display:inline-block;min-width:80px;font-size:0.82em;color:#888">&#8212;</span></td>'
        + '<td style="padding:4px"><input type="text" name="name_'+i+'" value="'+escHtml(name)+'" placeholder="(leer)" style="width:100%;background:#1e1e1e;color:#eee;border:1px solid #3a3a3a;border-radius:4px;padding:4px" oninput="onNameOrCityInput(this)"></td>'
        + '<td style="padding:4px"><input type="text" name="city_'+i+'" value="'+escHtml(city)+'" placeholder="(leer)" style="width:100%;background:#1e1e1e;color:#eee;border:1px solid #3a3a3a;border-radius:4px;padding:4px" oninput="onNameOrCityInput(this)"></td>'
        + '<td style="padding:4px;text-align:center"><button type="button" class="btn-clear" onclick="clearRow(this)" aria-label="Eintrag leeren" style="background:#3a1a1a;color:#e74c3c;border:none;border-radius:4px;padding:4px 8px;cursor:pointer" title="Eintrag leeren">&#10005;</button></td>'
        + '</tr>';
}

function onNameOrCityInput(inp) {
    var row = inp.closest('tr');
    updateRowDirtyState(row);
    var inputs = row.querySelectorAll('input[type=text]');
    var empty = inputs[0].value === '' && inputs[1].value === '';
    row.style.opacity = empty ? '0.45' : '';
}

function onDisabledChange(cb) {
    var row = cb.closest('tr');
    updateRowDirtyState(row);
    applyDisabledStyle(row);
}

function applyDisabledStyle(row) {
    var cb = row.querySelector('.dis-check');
    var textInputs = row.querySelectorAll('input[type=text]');
    var span = row.querySelector('.space-status');
    var empty = textInputs.length >= 2 && textInputs[0].value === '' && textInputs[1].value === '';
    var isDisabled = cb && cb.checked;
    row.style.fontStyle = isDisabled ? 'italic' : '';
    if (span) span.style.color = isDisabled ? '#bbb' : '';
    // Keep empty-row dimming intact; don't override it when not disabled.
    if (!isDisabled && !empty) {
        row.style.opacity = '';
    }
}

function clearRow(btn) {
    var row = btn.closest('tr');
    var textInputs = row.querySelectorAll('input[type=text]');
    var disCb = row.querySelector('.dis-check');
    if (textInputs.length >= 2) { textInputs[0].value = ''; textInputs[1].value = ''; }
    if (disCb) disCb.checked = false;
    applyDisabledStyle(row);
    row.style.opacity = '0.45';
    updateRowDirtyState(row);
}


function doSave()    { document.getElementById('mapForm').submit(); }
function doDiscard() { location.reload(); }

function doImport() {
    var raw = document.getElementById('importArea').value;
    // With position + optional disabled flag: { 2, "Name", "City"} or { 2, "Name", "City", 1}
    var rePos = /\{\s*(\d+)\s*,\s*"((?:\\.|[^"\\])*)"\s*,\s*"((?:\\.|[^"\\])*)"(?:\s*,\s*(\d+))?\s*\}/g;
    // Without position + optional disabled flag: { "Name", "City"} or { "Name", "City", 1}
    var reNoPos = /\{\s*"((?:\\.|[^"\\])*)"\s*,\s*"((?:\\.|[^"\\])*)"(?:\s*,\s*(\d+))?\s*\}/g;
    var m;
    var withPos = {};
    while ((m = rePos.exec(raw)) !== null) {
        withPos[parseInt(m[1], 10)] = { pos: parseInt(m[1], 10), name: unesc(m[2]), city: unesc(m[3]), disabled: m[4] === '1' };
    }
    var stripped = raw.replace(/\{\s*\d+\s*,\s*"(?:\\.|[^"\\])*"\s*,\s*"(?:\\.|[^"\\])*"(?:\s*,\s*\d+)?\s*\}/g, '');
    var noPos = [];
    while ((m = reNoPos.exec(stripped)) !== null) {
        noPos.push({ name: unesc(m[1]), city: unesc(m[2]), disabled: m[3] === '1' });
    }
    if (Object.keys(withPos).length === 0 && noPos.length === 0) {
        alert('Keine Eintr\u00e4ge gefunden.\nErwartetes Format:\n{ 2, "MuCCC", "Munich"}\noder: { "MuCCC", "Munich"}');
        return;
    }
    var tbody = document.getElementById('mapBody');
    var rows = Array.from(tbody.querySelectorAll('tr'));
    Object.values(withPos).forEach(function(p) {
        if (p.pos >= 0 && p.pos < rows.length) {
            var row = rows[p.pos];
            var inputs = row.querySelectorAll('input[type=text]');
            var disCb  = row.querySelector('.dis-check');
            if (inputs.length >= 2) {
                inputs[0].value = p.name;
                inputs[1].value = p.city;
                if (disCb) disCb.checked = p.disabled;
                applyDisabledStyle(row);
                row.style.opacity = (p.name === '' && p.city === '') ? '0.45' : row.style.opacity;
                updateRowDirtyState(row);
            }
        }
    });
    var freeIdx = 0;
    noPos.forEach(function(p) {
        while (freeIdx < rows.length) {
            var row = rows[freeIdx];
            var inputs = row.querySelectorAll('input[type=text]');
            var disCb  = row.querySelector('.dis-check');
            freeIdx++;
            if (inputs.length >= 2 && inputs[0].value === '') {
                inputs[0].value = p.name;
                inputs[1].value = p.city;
                if (disCb) disCb.checked = p.disabled;
                applyDisabledStyle(row);
                updateRowDirtyState(row);
                break;
            }
        }
    });
    document.getElementById('importArea').value = '';
}

var dragSrc = null;
function onDragStart(e) {
    dragSrc = e.currentTarget.closest('tr');
    e.dataTransfer.effectAllowed = 'move';
    e.dataTransfer.setData('text/plain', '');
    setTimeout(function() { dragSrc.style.opacity = '0.4'; }, 0);
}
function onDragOver(e) {
    e.preventDefault();
    e.dataTransfer.dropEffect = 'move';
    var target = e.currentTarget;
    if (target !== dragSrc) target.style.borderTop = '2px solid #1a7a3c';
}
function onDrop(e) {
    e.preventDefault();
    var target = e.currentTarget;
    target.style.borderTop = '';
    if (dragSrc && target !== dragSrc) {
        var tbody = document.getElementById('mapBody');
        var rows  = Array.from(tbody.querySelectorAll('tr'));
        var fromIdx = rows.indexOf(dragSrc);
        var toIdx   = rows.indexOf(target);
        if (fromIdx < toIdx) tbody.insertBefore(dragSrc, target.nextSibling);
        else                 tbody.insertBefore(dragSrc, target);
        // After drag: re-assign LED indices based on new row order (positions are fixed)
        Array.from(tbody.querySelectorAll('tr')).forEach(function(r, idx) {
            // Update the hidden led input and the display label
            var ledHidden = r.querySelector('input[type=hidden]');
            var ledDisplay = r.querySelector('td:nth-child(2)');
            if (ledHidden) ledHidden.value = idx;
            if (ledDisplay) {
                // Update text node (last child is the text)
                var tn = ledDisplay.lastChild;
                if (tn && tn.nodeType === 3) tn.nodeValue = idx;
            }
            updateRowDirtyState(r);
        });
        reindexRows();
    }
}
function onDragEnd(e) {
    var row = e.currentTarget.closest('tr');
    var inputs = row.querySelectorAll('input[type=text]');
    var isEmpty = inputs.length >= 2 && inputs[0].value === '' && inputs[1].value === '';
    row.style.opacity = isEmpty ? '0.45' : '';
    document.querySelectorAll('#mapBody tr').forEach(function(r) { r.style.borderTop = ''; });
}

function doExport() {
    var rows = document.getElementById('mapBody').querySelectorAll('tr');
    var parts = [];
    rows.forEach(function(r) {
        var ledHidden = r.querySelector('input[type=hidden]');
        var textInputs = r.querySelectorAll('input[type=text]');
        var disCb = r.querySelector('.dis-check');
        var led      = ledHidden ? ledHidden.value : '0';
        var name     = textInputs.length > 0 ? textInputs[0].value : '';
        var city     = textInputs.length > 1 ? textInputs[1].value : '';
        var disabled = (disCb && disCb.checked) ? '1' : '0';
        parts.push('{ ' + led + ', "' + escExport(name) + '", "' + escExport(city) + '", ' + disabled + '}');
    });
    var blob = new Blob([parts.join(', ') + '\n'], { type: 'text/plain' });
    var a = document.createElement('a');
    a.href = URL.createObjectURL(blob);
    a.download = 'searchList.txt';
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    URL.revokeObjectURL(a.href);
}

function blinkLed(btn) {
    var ledHidden = btn.closest('tr').querySelector('input[type=hidden]');
    var ledIndex = ledHidden ? parseInt(ledHidden.value, 10) : 0;
    var orig = btn.innerHTML;
    btn.disabled = true;
    btn.innerHTML = '&#8987;';
    fetch('/spacemap/blink?led=' + ledIndex)
        .then(function(r){ return r.json(); })
        .then(function(d){
            btn.innerHTML = d.ok ? '&#10003;' : '&#10005;';
            setTimeout(function(){ btn.innerHTML = orig; btn.disabled = false; }, 1500);
        })
        .catch(function(){
            btn.innerHTML = '&#10005;';
            setTimeout(function(){ btn.innerHTML = orig; btn.disabled = false; }, 1500);
        });
}

// Last known ageSec from /api/status — used to detect when the ESP has
// fetched fresh data from the SpaceAPI so the UI can update immediately.
var _lastAgeSec = -1;

function updateSpaceStatus() {
    Promise.all([
        fetch('/api/spacestatus').then(function(r) { return r.json(); }),
        fetch('/api/status').then(function(r) { return r.json(); })
    ])
    .then(function(results) {
        var list      = results[0];
        var apiStatus = results[1];

        // --- Detect a fresh ESP fetch by watching ageSec reset to a small value.
        // When the ESP completes a new API poll, ageSec drops back near 0.
        // We trigger an extra updateSpaceStatus() call a moment later so the
        // UI reflects the new data as soon as it arrives.
        var currentAge = (apiStatus && apiStatus.ageSec !== undefined) ? apiStatus.ageSec : -1;
        if (_lastAgeSec > 5 && currentAge >= 0 && currentAge < _lastAgeSec) {
            // ageSec reset — ESP just fetched new data; schedule an immediate refresh.
            setTimeout(updateSpaceStatus, 500);
        }
        _lastAgeSec = currentAge;

        // ESP has never completed a fetch yet (httpCode === 0) — keep the
        // hourglass indicator in place and try again shortly.
        if (apiStatus.httpCode === 0) {
            return;
        }

        var unmatchedSet = {};
        if (apiStatus.ok && apiStatus.parseErrors === 0 && apiStatus.unmatched) {
            apiStatus.unmatched.forEach(function(n) { unmatchedSet[n.toLowerCase().trim()] = true; });
        }

        if (list.length === 0) {
            document.querySelectorAll('#mapBody .space-status').forEach(function(s) {
                s.style.color = '#555'; s.title = 'No API data yet'; s.innerHTML = '&#8212;';
            });
            return;
        }

        var byLed  = {};
        var byName = {};
        list.forEach(function(e) {
            byLed[e.led]                        = e.status;
            byName[e.name.toLowerCase().trim()] = e.status;
        });
        document.querySelectorAll('#mapBody tr').forEach(function(row) {
            var ledHidden = row.querySelector('input[type=hidden]');
            var textInputs = row.querySelectorAll('input[type=text]');
            var span      = row.querySelector('.space-status');
            if (!ledHidden || !span) return;
            var led    = parseInt(ledHidden.value, 10);
            var nameInput = textInputs.length > 0 ? textInputs[0] : null;
            var name   = nameInput ? nameInput.value.toLowerCase().trim() : '';
            var status = name ? ((byLed[led] !== undefined) ? byLed[led] : byName[name]) : undefined;
            if (unmatchedSet[name]) {
                row.style.background = '#2a1a00'; row.style.outline = '1px solid #f5a800';
                span.style.color = '#f5a800'; span.title = 'Not found in last API response';
                span.innerHTML = '&#9888; N/A'; return;
            } else {
                if (row.style.background === 'rgb(42, 26, 0)') { row.style.background = ''; row.style.outline = ''; }
            }
            span.title = '';
            var disCb = row.querySelector('.dis-check');
            var isDisabled = disCb && disCb.checked;
            if      (status === 'OPEN')    { span.style.color = isDisabled ? '#bbb' : '#2dbe60'; span.innerHTML = '&#9679; OPEN'; }
            else if (status === 'CLOSED')  { span.style.color = isDisabled ? '#bbb' : '#e74c3c'; span.innerHTML = '&#9679; CLOSED'; }
            else if (status === 'UNKNOWN') { span.style.color = isDisabled ? '#bbb' : '#4a90d9'; span.innerHTML = '&#9679; UNKNOWN'; }
            else                           { span.style.color = '#555';    span.innerHTML = '&#8212;'; }
        });
    })
    .catch(function() {
        document.querySelectorAll('#mapBody .space-status').forEach(function(s) {
            s.style.color = '#e74c3c'; s.title = 'Status fetch failed'; s.innerHTML = '&#9888;';
        });
    });
}

// Initial call — zeigt Daten sofort wenn vorhanden, startet sonst den 2s-Retry.
updateSpaceStatus();
// Reguläres Polling alle 15 s als Fallback und für laufende Aktualisierung.
setInterval(updateSpaceStatus, 15000);

function reindexRows() {
    var rows = document.getElementById('mapBody').querySelectorAll('tr');
    rowCount = 0;
    rows.forEach(function(r) {
        r.id = 'row_' + rowCount;
        // Update hidden LED input name + value
        var ledHidden = r.querySelector('input[type=hidden]');
        if (ledHidden) { ledHidden.name = 'led_' + rowCount; ledHidden.value = rowCount; }
        // Update LED display label (text node inside 3rd td)
        var ledTd = r.querySelector('td:nth-child(2)');
        if (ledTd) {
            var tn = ledTd.lastChild;
            if (tn && tn.nodeType === 3) tn.nodeValue = rowCount;
        }
        // Update text input names
        var textInputs = r.querySelectorAll('input[type=text]');
        if (textInputs.length > 0) textInputs[0].name = 'name_' + rowCount;
        if (textInputs.length > 1) textInputs[1].name = 'city_' + rowCount;
        // Update disabled checkbox name
        var disCb = r.querySelector('.dis-check');
        if (disCb) disCb.name = 'dis_' + rowCount;
        rowCount++;
    });
}
</script>
)rawjs";

// JavaScript for / (index page).
static const char INDEX_JS[] PROGMEM = R"rawjs(
<script>
// Escape text (API URL, space names from the external feed) before innerHTML.
function escHtml(s){return String(s).replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;').replace(/"/g,'&quot;').replace(/'/g,'&#39;');}
function checkApi() {
    fetch('/api/status')
        .then(function(r){ return r.json(); })
        .then(function(d) {
            var box = document.getElementById('apiStatus');
            var age = d.ageSec > 0 ? ' &mdash; last checked ' + d.ageSec + 's ago' : '';
            if (d.httpCode === 0) {
                box.style.borderColor = '#555';
                box.innerHTML = '<span style="font-size:1.1em">&#9711;</span>'
                    + ' <strong style="color:#aaa">No fetch yet</strong>'
                    + '<br><small style="color:#777">API URL: ' + escHtml(d.url) +'</small>';
            } else if (d.ok) {
                box.style.borderColor = '#1a7a3c';
                box.innerHTML = '<span style="font-size:1.1em">&#9679;</span>'
                    + ' <strong style="color:#2dbe60">SpaceMap API reachable</strong>' + age
                    + '<br><small style="color:#777">HTTP ' + d.httpCode + ' &mdash; ' + escHtml(d.url) +'</small>';
            } else {
                box.style.borderColor = '#c0392b';
                box.innerHTML = '<span style="font-size:1.1em">&#9679;</span>'
                    + ' <strong style="color:#e74c3c">SpaceMap API unreachable</strong>' + age
                    + '<br><small style="color:#777">HTTP ' + d.httpCode + ' &mdash; ' + escHtml(d.url) +'</small>';
            }
            var pbox = document.getElementById('parseStatus');
            if (d.httpCode === 0) { pbox.style.display = 'none'; return; }
            pbox.style.display = 'block';
            pbox.style.cursor  = 'pointer';
            pbox.onclick       = function() { location.href = '/spacemap'; };
            if (!d.ok) {
                pbox.style.borderColor = '#555';
                pbox.innerHTML = '<span style="color:#777;font-size:0.9em">&#8212; Parse status not available (HTTP error)</span>';
                return;
            }
            if (d.watchListSize === 0) {
                pbox.style.borderColor = '#555';
                pbox.innerHTML = '<span style="font-size:1.1em">&#9711;</span>'
                    + ' <strong style="color:#aaa">Watch list is empty &mdash; no matching needed</strong>';
                return;
            }
            if (d.parseErrors > 0) {
                pbox.style.borderColor = '#f5a800';
                pbox.innerHTML = '<span style="font-size:1.1em">&#9888;</span>'
                    + ' <strong style="color:#f5a800">Parse errors detected</strong>'
                    + '<br><small style="color:#aaa">'
                    + d.parseErrors + ' error(s) out of ' + d.totalObjects + ' objects &mdash; '
                    + d.foundCount + ' of ' + d.watchListSize + ' spaces matched</small>';
                return;
            }
            var notFound = d.unmatched ? d.unmatched.length : 0;
            if (notFound === 0) {
                pbox.style.borderColor = '#1a7a3c';
                pbox.innerHTML = '<span style="font-size:1.1em">&#10003;</span>'
                    + ' <strong style="color:#2dbe60">All ' + d.foundCount + ' of ' + d.watchListSize + ' spaces found in API</strong>';
            } else {
                pbox.style.borderColor = '#f5a800';
                pbox.innerHTML = '<span style="font-size:1.1em">&#9888;</span>'
                    + ' <strong style="color:#f5a800">' + d.foundCount + ' of ' + d.watchListSize + ' spaces found &mdash; '
                    + notFound + ' not in API response</strong>'
                    + '<br><small style="color:#aaa">Not found: ' + escHtml(d.unmatched.join(', ')) + '</small>';
            }
        })
        .catch(function() {
            document.getElementById('apiStatus').innerHTML =
                '<span style="color:#e74c3c">&#9888; Could not reach device</span>';
            document.getElementById('parseStatus').style.display = 'none';
        });
}
checkApi();
setInterval(checkApi, 30000);
</script>
)rawjs";

// --------------------------------------------------------------------------
// Public interface
// --------------------------------------------------------------------------
void SettingsWebServer::begin() {
    registerRoutes();
    _server.begin();
#ifdef DEBUG
    Serial.println("WEB: Settings server started on port 80");
#endif
}

// --------------------------------------------------------------------------
// Register all URL routes on the async web server.
// --------------------------------------------------------------------------
void SettingsWebServer::registerRoutes() {
    _port443Redirect.on("/", HTTP_GET, [this](AsyncWebServerRequest* req) {
        handleHttpsRedirect(req);
    });
    _port443Redirect.onNotFound([this](AsyncWebServerRequest* req) {
        handleHttpsRedirect(req);
    });
    _port443Redirect.begin();

    _server.on("/", HTTP_GET, [this](AsyncWebServerRequest* req) {
        handleIndex(req);
    });

    _server.on("/settings", HTTP_GET, [this](AsyncWebServerRequest* req) {
        handleSettingsGet(req);
    });

    _server.on("/settings", HTTP_POST, [this](AsyncWebServerRequest* req) {
        handleSettingsPost(req);
    });

    _server.on("/settings/reset", HTTP_POST, [this](AsyncWebServerRequest* req) {
        handleSettingsReset(req);
    });

    // Sub-routes must be registered before the parent route, because
    // ESPAsyncWebServer matches handlers in registration order and
    // "/spacemap" would otherwise swallow "/spacemap/reset" etc.
    _server.on("/spacemap/reset", HTTP_POST, [this](AsyncWebServerRequest* req) {
        handleSpaceMapReset(req);
    });

    _server.on("/spacemap/export", HTTP_GET, [this](AsyncWebServerRequest* req) {
        handleSpaceMapExport(req);
    });

    _server.on("/spacemap/blink", HTTP_GET, [this](AsyncWebServerRequest* req) {
        handleSpaceMapBlink(req);
    });

    _server.on("/spacemap", HTTP_GET, [this](AsyncWebServerRequest* req) {
        handleSpaceMapGet(req);
    });

    _server.on("/spacemap", HTTP_POST, [this](AsyncWebServerRequest* req) {
        handleSpaceMapPost(req);
    });

    _server.on("/api/spacemap", HTTP_GET, [this](AsyncWebServerRequest* req) {
        handleApiSpaceMapGet(req);
    });

    _server.on("/api/spacestatus", HTTP_GET, [this](AsyncWebServerRequest* req) {
        // Copy the list under lock, then build JSON without holding the lock
        // to keep the critical section as short as possible (no I/O under lock).
        std::vector<SpaceStatusList> snapshot;
        if (spaceStatusMutex && xSemaphoreTake(spaceStatusMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            snapshot = spaceStatusList;
            xSemaphoreGive(spaceStatusMutex);
        }

        AsyncResponseStream* s = req->beginResponseStream("application/json");
        s->print("[");
        for (size_t i = 0; i < snapshot.size(); i++) {
            const auto& e = snapshot[i];
            const char* statusStr;
            switch (e.getStatus()) {
                case SpaceStatus::open:   statusStr = "OPEN";    break;
                case SpaceStatus::closed: statusStr = "CLOSED";  break;
                default:                  statusStr = "UNKNOWN"; break;
            }
            if (i > 0) s->print(",");
            s->print("{\"name\":\"");
            s->print(jsonEscape(e.getName()));
            s->print("\",\"status\":\"");
            s->print(statusStr);
            s->print("\",\"led\":");
            s->print(e.getLED());
            s->print("}");
        }
        s->print("]");
        req->send(s);
    });

    _server.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest* req) {
        int code = WebClientHandler::getLastHttpCode();
        unsigned long age = (WebClientHandler::getLastAttemptMs() == 0)
                            ? 0
                            : (millis() - WebClientHandler::getLastAttemptMs()) / 1000UL;

        AsyncResponseStream* s = req->beginResponseStream("application/json");
        s->print("{\"httpCode\":");
        s->print(code);
        s->print(",\"ok\":");
        s->print(code == 200 ? "true" : "false");
        s->print(",\"ageSec\":");
        s->print(age);
        s->print(",\"url\":\"");
        s->print(jsonEscape(AppConfig::getInstance().getSpaceApiUrl()));
        s->print("\",\"foundCount\":");
        s->print(WebClientHandler::getLastFoundCount());
        s->print(",\"parseErrors\":");
        s->print(WebClientHandler::getLastParseErrors());
        s->print(",\"totalObjects\":");
        s->print(WebClientHandler::getLastTotalObjects());
        s->print(",\"watchListSize\":");
        s->print(WebClientHandler::getLastWatchListSize());
        s->print(",\"unmatched\":[");
        const auto& unmatched = WebClientHandler::getLastUnmatchedNames();
        for (size_t i = 0; i < unmatched.size(); i++) {
            if (i > 0) s->print(",");
            s->print("\"");
            s->print(jsonEscape(unmatched[i]));
            s->print("\"");
        }
        s->print("]}");
        req->send(s);
    });

    _server.onNotFound([this](AsyncWebServerRequest* req) {
        handleNotFound(req);
    });
}

// --------------------------------------------------------------------------
// Route handlers
// --------------------------------------------------------------------------
void SettingsWebServer::handleIndex(AsyncWebServerRequest* request) {
    AsyncResponseStream* s = request->beginResponseStream("text/html");
    streamIndexPage(s);
    request->send(s);
}

void SettingsWebServer::handleSettingsGet(AsyncWebServerRequest* request) {
    AsyncResponseStream* s = request->beginResponseStream("text/html");
    streamSettingsPage(s);
    request->send(s);
}

void SettingsWebServer::handleSettingsPost(AsyncWebServerRequest* request) {
    AppConfig& cfg = AppConfig::getInstance();

    auto getParam = [&](const String& key) -> String {
        if (!request->hasParam(key, true)) return "";
        return request->getParam(key, true)->value();
    };

    auto applyULong = [](const String& s, unsigned long minVal, unsigned long maxVal,
                         std::function<void(unsigned long)> setter) {
        if (s.length() == 0) return;
        long value = s.toInt();
        if (value > 0 && (unsigned long)value >= minVal && (unsigned long)value <= maxVal)
            setter((unsigned long)value);
    };

    auto applyUInt8 = [](const String& s, std::function<void(uint8_t)> setter) {
        if (s.length() == 0) return;
        int value = s.toInt();
        if (value >= 0 && value <= 255) setter((uint8_t)value);
    };

    auto applyUInt16 = [](const String& s, uint16_t minVal, uint16_t maxVal,
                          std::function<void(uint16_t)> setter) {
        if (s.length() == 0) return;
        int value = s.toInt();
        if (value >= minVal && value <= maxVal) setter((uint16_t)value);
    };

    String apiUrl = getParam("apiUrl");
    if (apiUrl.length() > 0) cfg.setSpaceApiUrl(apiUrl);

    applyULong(getParam("wifiInterval"),    30,   86400, [&](unsigned long v){ cfg.setIntervalWifiCheck(v); });
    applyULong(getParam("ledInterval"),      1,    3600, [&](unsigned long v){ cfg.setIntervalLEDs(v); });
    applyULong(getParam("apiInterval"),     10,    3600, [&](unsigned long v){ cfg.setIntervalApi(v); });
    applyUInt8(getParam("ledBrightness"),              [&](uint8_t v){ cfg.setLedBrightness(v); });
    applyUInt8(getParam("onboardBrightness"),          [&](uint8_t v){ cfg.setOnboardBrightness(v); });
    applyUInt16(getParam("ledMaxPower"), 100, 5000,    [&](uint16_t v){ cfg.setLedMaxPowerMa(v); });

    cfg.save();

#ifdef DEBUG
    Serial.println("WEB: Settings saved via web interface");
#endif

    AsyncResponseStream* s = request->beginResponseStream("text/html");
    streamSettingsPage(s, "Settings saved.");
    request->send(s);
}

void SettingsWebServer::handleSettingsReset(AsyncWebServerRequest* request) {
    AppConfig::getInstance().resetToDefaults();
#ifdef DEBUG
    Serial.println("WEB: Settings reset to defaults");
#endif
    AsyncResponseStream* s = request->beginResponseStream("text/html");
    streamSettingsPage(s, "Settings reset to defaults.");
    request->send(s);
}

void SettingsWebServer::handleNotFound(AsyncWebServerRequest* request) {
    AsyncResponseStream* s = request->beginResponseStream("text/html");
    streamHtmlHeader(s, "404");
    streamNavBar(s);
    s->print("<h2>404 - Page not found</h2><p><a href='/'>Back to home</a></p>");
    streamHtmlFooter(s);
    request->send(s);
}

void SettingsWebServer::handleHttpsRedirect(AsyncWebServerRequest* request) {
    String ip  = WiFi.localIP().toString();
    // Escape the request path before embedding it in HTML — it is fully
    // attacker-controllable and would otherwise allow markup injection.
    String url = htmlAttrEscape(request->url());
    String html = "<!DOCTYPE html><html><head>"
                  "<meta http-equiv='refresh' content='0;url=http://" + ip + url + "'>"
                  "</head><body>"
                  "<p>Redirecting to <a href='http://" + ip + url + "'>http://" + ip + url + "</a></p>"
                  "</body></html>";
    request->send(200, "text/html", html);
}

// --------------------------------------------------------------------------
// Streaming HTML fragment helpers
// Each helper writes its markup piece by piece into the response stream.
// The big CSS block is read straight from PROGMEM via FPSTR(), so it never
// lives in the heap as a copy.
// --------------------------------------------------------------------------
void SettingsWebServer::streamHtmlHeader(AsyncResponseStream* s, const String& title) {
    s->print(F("<!DOCTYPE html><html lang='en'><head>"
               "<meta charset='UTF-8'>"
               "<meta name='viewport' content='width=device-width, initial-scale=1'>"
               "<title>SpaceMap - "));
    s->print(title);
    s->print(F("</title><style>"));
    s->print(FPSTR(CSS));   // streamed from PROGMEM, never copied into a String
    s->print(F("</style></head>"
               "<body>"
               // Apply the saved theme as early as possible so the page does
               // not flash in dark mode before the toggle script runs.
               "<script>if(localStorage.getItem('theme')==='light')document.body.classList.add('light');</script>"));
}

void SettingsWebServer::streamHtmlFooter(AsyncResponseStream* s) {
    s->print(F("</div></main></body></html>"));
}

void SettingsWebServer::streamNavBar(AsyncResponseStream* s) {
    s->print(F("<nav aria-label='Hauptnavigation'>"
               "<div class='nav-links'>"
               "<a href='/'>&#127968; Overview</a>"
               "<a href='/settings'>&#9881; Settings</a>"
               "<a href='/spacemap'>&#128280; SpaceMap</a>"
               "</div>"
               "<button id='themeToggle' onclick='toggleTheme()' title='Toggle light/dark mode'>"
               "<span id='themeIcon'></span>"
               "</button>"
               "</nav>"
               "<script>"
               "function toggleTheme(){"
                 "var isLight=document.body.classList.toggle('light');"
                 "localStorage.setItem('theme',isLight?'light':'dark');"
                 "updateThemeIcon();}"
               "function updateThemeIcon(){"
                 "var el=document.getElementById('themeIcon');"
                 "if(el)el.textContent=document.body.classList.contains('light')?'\\uD83C\\uDF19':'\\u2600\\uFE0F';}"
               "updateThemeIcon();"
               "</script>"
               "<main><div class='container'>"));
}

// --------------------------------------------------------------------------
// Streaming page renderers
// Each function writes into an AsyncResponseStream in small pieces so that
// the ESP32 heap never needs to hold the full page as one String object.
// --------------------------------------------------------------------------

void SettingsWebServer::streamIndexPage(AsyncResponseStream* s, const String& message) {
    streamHtmlHeader(s, "Overview");
    streamNavBar(s);
    s->print("<h1>SpaceMap on Fiber</h1>");
    s->print("<p>Welcome to the SpaceMap controller web interface.</p>");
    s->print("<div id='apiStatus' aria-live='polite' style='margin-top:20px;padding:14px 18px;border-radius:6px;"
             "background:#1a1a1a;border:1px solid #3a3a3a;max-width:480px;'>"
             "<span style='color:#888;font-size:0.9em'>&#8635; Checking API connection&hellip;</span>"
             "</div>");
    s->print("<div id='parseStatus' aria-live='polite' style='margin-top:10px;padding:14px 18px;border-radius:6px;"
             "background:#1a1a1a;border:1px solid #3a3a3a;max-width:480px;display:none;'></div>");
    s->print(FPSTR(INDEX_JS));
    if (message.length() > 0) {
        s->print("<div class='msg'>");
        s->print(message);
        s->print("</div>");
    }
    streamHtmlFooter(s);
}

void SettingsWebServer::streamSettingsPage(AsyncResponseStream* s, const String& message) {
    AppConfig& cfg = AppConfig::getInstance();

    streamHtmlHeader(s, "Settings");
    streamNavBar(s);
    s->print("<h1>Settings</h1>");

    if (message.length() > 0) {
        bool isReset = message.indexOf("reset") >= 0;
        s->print("<div class='msg");
        if (isReset) s->print(" msg-reset");
        s->print("'>");
        s->print(message);
        s->print("</div>");
    }

    s->print("<form method='POST' action='/settings'>");
    s->print("<h2>API</h2>");
    s->print("<label for='apiUrl'>SpaceAPI URL</label>"
             "<input id='apiUrl' type='url' name='apiUrl' value='");
    s->print(htmlAttrEscape(cfg.getSpaceApiUrl()));
    s->print("'>");

    s->print("<label for='apiInterval'>API polling interval (seconds)</label>"
             "<input id='apiInterval' type='number' name='apiInterval' min='10' max='3600' value='");
    s->print(cfg.getIntervalApi());
    s->print("'>");

    s->print("<h2>WiFi</h2>");
    s->print("<label for='wifiInterval'>WiFi health-check interval (seconds)</label>"
             "<input id='wifiInterval' type='number' name='wifiInterval' min='30' max='86400' value='");
    s->print(cfg.getIntervalWifiCheck());
    s->print("'>");

    s->print("<h2>LEDs</h2>");
    s->print("<label for='ledInterval'>LED refresh interval (seconds)</label>"
             "<input id='ledInterval' type='number' name='ledInterval' min='1' max='3600' value='");
    s->print(cfg.getIntervalLEDs());
    s->print("'>");

    s->print("<label for='ledBrightness'>LED brightness (0-255)</label>"
             "<input id='ledBrightness' type='number' name='ledBrightness' min='0' max='255' value='");
    s->print(cfg.getLedBrightness());
    s->print("'>");

    s->print("<label for='onboardBrightness'>Onboard LED brightness (0-255)</label>"
             "<input id='onboardBrightness' type='number' name='onboardBrightness' min='0' max='255' value='");
    s->print(cfg.getOnboardBrightness());
    s->print("'>");

    s->print("<label for='ledMaxPower'>Max. LED power draw (mA)</label>"
             "<input id='ledMaxPower' type='number' name='ledMaxPower' min='100' max='5000' value='");
    s->print(cfg.getLedMaxPowerMa());
    s->print("'>");

    s->print("<br><button type='submit' class='btn btn-save'>&#128190; Save</button>");
    s->print("</form>");

    s->print("<form method='POST' action='/settings/reset' "
             "onsubmit=\"return confirm('Reset all settings to defaults?')\">"
             "<button type='submit' class='btn btn-reset'>&#8635; Reset to defaults</button>"
             "</form>");

    streamHtmlFooter(s);
}

void SettingsWebServer::streamSpaceMapPage(AsyncResponseStream* s, const String& message) {
    streamHtmlHeader(s, "SpaceMap");
    streamNavBar(s);
    s->print("<h1>Hackerspace Mapping</h1>");

    // --- Editor table ---
    s->print("<h2>Active Mapping</h2>"
             "<form method='POST' action='/spacemap' id='mapForm'>"
             "<table id='mapTable' style='width:100%;border-collapse:collapse;margin-top:8px'>"
             "<thead><tr>"
             "<th style='padding:6px;border-bottom:1px solid #3a3a3a;width:24px' title='Zeile ziehen zum Umsortieren'></th>"
             "<th style='text-align:center;padding:6px;color:#2dbe60;border-bottom:1px solid #3a3a3a;width:40px' title='LED-Index (0 = erste LED)'>LED#</th>"
             "<th style='padding:6px;border-bottom:1px solid #3a3a3a;text-align:center;color:#2dbe60' title='LED deaktivieren – bleibt aus unabh&auml;ngig vom Space-Status'>Aus</th>"
             "<th style='padding:6px;border-bottom:1px solid #3a3a3a;text-align:center;color:#2dbe60' title='LED kurz aufblinken lassen zum Orten'>Blinken</th>"
             "<th style='padding:6px;border-bottom:1px solid #3a3a3a;text-align:center;color:#2dbe60' title='Aktueller Space-Status aus der SpaceAPI'>Status</th>"
             "<th style='text-align:left;padding:6px;color:#2dbe60;border-bottom:1px solid #3a3a3a' title='Name exakt wie in der SpaceAPI'>Space Name</th>"
             "<th style='text-align:left;padding:6px;color:#2dbe60;border-bottom:1px solid #3a3a3a' title='Stadt (optional, nur zur Anzeige)'>City</th>"
             "<th style='padding:6px;border-bottom:1px solid #3a3a3a;width:32px'></th>"
             "</tr></thead>"
             "<tbody id='mapBody'>");

    // tbody is populated by JS via fetch('/api/spacemap') — no rows rendered here.
    // This keeps the initial HTML response small and avoids heap pressure on the ESP32.

    s->print("</tbody></table></form>");

    bool isSaved = (message == "Mapping saved.");
    if (message.length() > 0 && !isSaved) {
        bool isReset = message.indexOf("eset") >= 0;
        bool isError = message.indexOf("rror") >= 0;
        const char* color = isError ? "#f88" : "#6fcf6f";
        const char* bg    = isError ? "#2e0e0e" : (isReset ? "#2e1a00" : "#0e2e0e");
        s->print("<div id='saveMsg' style='margin-top:10px;padding:8px 14px;border-radius:4px;font-weight:bold;background:");
        s->print(bg); s->print(";color:"); s->print(color); s->print("'>");
        s->print(message);
        s->print("</div>");
    }
    if (isSaved) s->print("<script>var _showSaved=true;</script>");

    s->print("<div id='unsavedBar' style='display:none;position:fixed;bottom:0;left:0;right:0;"
             "background:#111e11;border-top:2px solid #1a7a3c;padding:10px 24px;"
             "z-index:999;justify-content:center;align-items:center;gap:12px;box-shadow:0 -4px 16px #0008'>"
             "<span id='barSavedMsg' style='display:none;color:#6fcf6f;font-weight:bold'>&#10003; Mapping saved.</span>"
             "<span id='barUnsavedLabel' style='color:#6fcf6f;font-weight:bold'>&#9679; Unsaved changes</span>"
             "<button id='barDiscard' type='button' onclick='doDiscard()' class='btn btn-reset' style='margin:0'>Discard</button>"
             "<button id='barSave' type='button' onclick='doSave()' class='btn btn-save' style='margin:0'>&#128190; Save</button>"
             "</div>");

    // --- Import ---
    s->print("<h2>Import</h2>"
             "<p style='color:#bbb;font-size:0.9em'>Eintr&auml;ge einfügen und Import klicken. Erwartetes Format:</p>"
             "<pre style='background:#1a1a1a;border:1px solid #2a2a2a;border-radius:4px;padding:10px;"
             "font-size:0.8em;color:#888;overflow-x:auto;margin:0 0 10px 0;'>"
             "{ 2, \"MuCCC\", \"Munich\"}&#10;"
             "{ 2, \"MuCCC\", \"Munich\", 1}  &larr; deaktiviert&#10;"
             "{ \"MuCCC\", \"Munich\"}"
             "</pre>"
             "<textarea id='importArea' aria-label='Eintr&auml;ge zum Importieren' rows='6' style='width:100%;background:#1e1e1e;color:#eee;"
             "border:1px solid #3a3a3a;border-radius:4px;padding:7px;font-family:monospace;"
             "font-size:0.85em;box-sizing:border-box;'></textarea>"
             "<div style='margin-top:8px;display:flex;gap:10px;align-items:center;flex-wrap:wrap'>"
             "<button type='button' class='btn btn-save' style='margin:0' onclick='doImport()'>&#8659; Import</button>"
             "</div>");

    // --- Export ---
    s->print("<h2>Export</h2>"
             "<p style='color:#bbb;font-size:0.9em'>L&auml;dt das aktuelle Mapping als <code>searchList.txt</code> herunter "
             "(alle Eintr&auml;ge inkl. leerer Positionen).</p>"
             "<button type='button' class='btn btn-save' onclick='doExport()'>&#8659; Export searchList.txt</button>");

    // --- Reset ---
    s->print("<h2>Reset</h2>"
             "<form method='POST' action='/spacemap/reset' "
             "onsubmit=\"return confirm('Reset mapping to built-in default?')\">"
             "<button type='submit' class='btn btn-reset'>&#8635; Reset to default</button>"
             "</form>");

    // Inject LED_SLOT_COUNT so JS knows how many rows to expect, then load the mapping
    // asynchronously from /api/spacemap to keep the initial HTML small.
    s->print("<script>var LED_SLOT_COUNT = ");
    s->print(LED_SLOT_COUNT);
    s->print("; var rowCount = 0;</script>"
             "<script>"
             "fetch('/api/spacemap').then(function(r){return r.json();}).then(function(entries){"
             "var tbody=document.getElementById('mapBody');"
             "var html='';"
             "for(var i=0;i<LED_SLOT_COUNT;i++){"
             "var e=entries[i]||{led:i,name:'',city:'',disabled:false};"
             "html+=buildRow(i,e.name,e.city,e.disabled);}"
             "tbody.innerHTML=html;"
             "rowCount=LED_SLOT_COUNT;"
             "initSnapshot();"
             "if(typeof _showSaved!=='undefined'&&_showSaved)showSavedState();"
             "updateSpaceStatus();"
             "}).catch(function(){document.getElementById('mapBody').innerHTML="
             "'<tr><td colspan=8 style=color:#e74c3c>&#9888; Failed to load mapping from /api/spacemap</td></tr>';});"
             "</script>");
    s->print(FPSTR(SPACEMAP_JS));

    streamHtmlFooter(s);
}

// --------------------------------------------------------------------------
// /spacemap  – GET
// --------------------------------------------------------------------------
void SettingsWebServer::handleSpaceMapGet(AsyncWebServerRequest* request) {
    String msg = "";
    if (request->hasParam("msg")) {
        String key = request->getParam("msg")->value();
        if      (key == "saved") msg = "Mapping saved.";
        else if (key == "reset") msg = "Mapping reset to default.";
    }
    AsyncResponseStream* s = request->beginResponseStream("text/html");
    streamSpaceMapPage(s, msg);
    request->send(s);
}

// --------------------------------------------------------------------------
// /spacemap  – POST: save edited list
// --------------------------------------------------------------------------
void SettingsWebServer::handleSpaceMapPost(AsyncWebServerRequest* request) {
    std::vector<SpaceSearchList> newList;
    // Always iterate exactly LED_SLOT_COUNT slots — empty entries are stored as empty strings.
    for (int idx = 0; idx < LED_SLOT_COUNT; idx++) {
        String ledKey  = "led_"  + String(idx);
        String nameKey = "name_" + String(idx);
        String cityKey = "city_" + String(idx);
        String disKey  = "dis_"  + String(idx);

        if (!request->hasParam(ledKey, true)) break;

        String ledStr  = request->getParam(ledKey,  true)->value();
        String nameStr = request->hasParam(nameKey, true) ? request->getParam(nameKey, true)->value() : "";
        String cityStr = request->hasParam(cityKey, true) ? request->getParam(cityKey, true)->value() : "";
        // Checkboxes are only submitted when checked — absence means unchecked.
        bool   disVal  = request->hasParam(disKey, true);

        uint8_t ledNum = (uint8_t)constrain(ledStr.toInt(), 0, 255);
        // Store every slot — empty name means LED will be off (black).
        newList.emplace_back(ledNum, nameStr, cityStr, disVal);
    }

    DataSpaceList::getInstance().saveList(newList);

#ifdef DEBUG
    Serial.println("WEB: SpaceMap saved via web interface");
#endif
    // 303 redirect — avoids re-rendering the full page in the POST handler,
    // which would overflow the async task stack with LED_SLOT_COUNT entries.
    AsyncWebServerResponse* resp = request->beginResponse(303);
    resp->addHeader("Location", "/spacemap?msg=saved");
    request->send(resp);
}

// --------------------------------------------------------------------------
// /spacemap/reset  – POST
// --------------------------------------------------------------------------
void SettingsWebServer::handleSpaceMapReset(AsyncWebServerRequest* request) {
    DataSpaceList::getInstance().resetToDefault();
#ifdef DEBUG
    Serial.println("WEB: SpaceMap reset to default");
#endif
    AsyncWebServerResponse* resp = request->beginResponse(303);
    resp->addHeader("Location", "/spacemap?msg=reset");
    request->send(resp);
}

// --------------------------------------------------------------------------
// /spacemap/blink  – GET
// --------------------------------------------------------------------------
void SettingsWebServer::handleSpaceMapBlink(AsyncWebServerRequest* request) {
    if (!request->hasParam("led")) {
        request->send(400, "application/json", "{\"ok\":false,\"error\":\"missing led param\"}");
        return;
    }
    int ledIndex = request->getParam("led")->value().toInt();
    if (ledIndex < 0 || ledIndex >= LED_SLOT_COUNT) {
        request->send(400, "application/json", "{\"ok\":false,\"error\":\"led index out of range\"}");
        return;
    }

    // Reject the request if a blink task is already running. This protects the
    // heap and the FreeRTOS task pool from being flooded by repeated calls.
    if (_blinkTaskRunning) {
        request->send(429, "application/json", "{\"ok\":false,\"error\":\"blink already running\"}");
        return;
    }

    struct BlinkParams {
        uint8_t ledIndex;
        std::vector<SpaceStatusList> snapshot;
    };
    // Copy spaceStatusList under lock before spawning the task.
    std::vector<SpaceStatusList> statusSnapshot;
    if (spaceStatusMutex && xSemaphoreTake(spaceStatusMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        statusSnapshot = spaceStatusList;
        xSemaphoreGive(spaceStatusMutex);
    }
    auto* params = new BlinkParams{(uint8_t)ledIndex, std::move(statusSnapshot)};

    // Mark the guard as running BEFORE xTaskCreate so a fast follow-up
    // request cannot slip past the check above while the task is starting.
    _blinkTaskRunning = true;

    BaseType_t taskCreated = xTaskCreate([](void* arg) {
        auto* bp = static_cast<BlinkParams*>(arg);
        NeoPixelLED::getInstance().blinkLED(bp->ledIndex, bp->snapshot);
        delete bp;
        _blinkTaskRunning = false;   // Allow the next blink request.
        vTaskDelete(nullptr);
    }, "blink_task", kBlinkTaskStackSize, params, 1, nullptr);

    if (taskCreated != pdPASS) {
        // xTaskCreate failed (likely out of heap). Clean up the parameter
        // block we allocated and tell the client to retry later.
        delete params;
        _blinkTaskRunning = false;
        request->send(503, "application/json", "{\"ok\":false,\"error\":\"task creation failed\"}");
        return;
    }

    request->send(200, "application/json", "{\"ok\":true}");
}

// --------------------------------------------------------------------------
// /api/spacemap  – GET: compact JSON for the editor table
// --------------------------------------------------------------------------
void SettingsWebServer::handleApiSpaceMapGet(AsyncWebServerRequest* request) {
    const auto& list = DataSpaceList::getInstance().getList();

    std::vector<const SpaceSearchList*> byLed(LED_SLOT_COUNT, nullptr);
    for (const auto& e : list) {
        if (e.getLED() < LED_SLOT_COUNT) byLed[e.getLED()] = &e;
    }

    AsyncResponseStream* s = request->beginResponseStream("application/json");
    s->print("[");
    for (int i = 0; i < LED_SLOT_COUNT; i++) {
        if (i > 0) s->print(",");
        const SpaceSearchList* e = byLed[i];
        s->print("{\"led\":");
        s->print(i);
        s->print(",\"name\":\"");
        s->print(jsonEscape(e ? e->getName() : ""));
        s->print("\",\"city\":\"");
        s->print(jsonEscape(e ? e->city : ""));
        s->print("\",\"disabled\":");
        s->print((e && e->isDisabled()) ? "true" : "false");
        s->print("}");
    }
    s->print("]");
    request->send(s);
}

// Escape backslash and double-quote so a name/city containing them survives a
// round-trip through the { n, "name", "city"} export/import text format.
// Must mirror the JS escExport()/unesc() pair in SPACEMAP_JS.
static String exportEscape(const String& in) {
    String out = in;
    out.replace("\\", "\\\\");   // backslash first
    out.replace("\"", "\\\"");   // then double-quote
    return out;
}

// --------------------------------------------------------------------------
// /spacemap/export  – GET
// --------------------------------------------------------------------------
void SettingsWebServer::handleSpaceMapExport(AsyncWebServerRequest* request) {
    const auto& list = DataSpaceList::getInstance().getList();

    // Build a lookup by LED index so we can iterate all LED_SLOT_COUNT slots in order.
    std::vector<SpaceSearchList const*> byLed(LED_SLOT_COUNT, nullptr);
    for (const auto& e : list) {
        if (e.getLED() < LED_SLOT_COUNT) byLed[e.getLED()] = &e;
    }

    AsyncResponseStream* s = request->beginResponseStream("application/octet-stream");
    s->addHeader("Content-Disposition", "attachment; filename=\"searchList.txt\"");
    s->addHeader("Content-Transfer-Encoding", "binary");
    for (int i = 0; i < LED_SLOT_COUNT; i++) {
        if (i > 0) s->print(", ");
        if (byLed[i]) {
            s->print("{ ");
            s->print(i);
            s->print(", \"");
            s->print(exportEscape(byLed[i]->getName()));
            s->print("\", \"");
            s->print(exportEscape(byLed[i]->city));
            s->print("\"}");
        } else {
            s->print("{ ");
            s->print(i);
            s->print(", \"\", \"\"}");
        }
    }
    s->print("\n");
    request->send(s);
}

// --------------------------------------------------------------------------
// Escaping helpers
// --------------------------------------------------------------------------

// Escape a string for safe use inside an HTML attribute value or text node.
// Replaces: & -> &amp;  < -> &lt;  > -> &gt;  " -> &quot;  ' -> &#39;
String SettingsWebServer::htmlAttrEscape(const String& in) {
    String out;
    out.reserve(in.length() + 16);
    for (unsigned int i = 0; i < in.length(); i++) {
        char c = in[i];
        switch (c) {
            case '&':  out += F("&amp;");  break;
            case '<':  out += F("&lt;");   break;
            case '>':  out += F("&gt;");   break;
            case '"':  out += F("&quot;"); break;
            case '\'': out += F("&#39;");  break;
            default:   out += c;           break;
        }
    }
    return out;
}

// Escape a string for safe embedding as a JSON string value.
// Replaces: \ -> \\  " -> \"  control chars \n \r \t with their escape sequences.
String SettingsWebServer::jsonEscape(const String& in) {
    String out;
    out.reserve(in.length() + 8);
    for (unsigned int i = 0; i < in.length(); i++) {
        char c = in[i];
        switch (c) {
            case '\\': out += F("\\\\"); break;
            case '"':  out += F("\\\""); break;
            case '\n': out += F("\\n");  break;
            case '\r': out += F("\\r");  break;
            case '\t': out += F("\\t");  break;
            default:   out += c;         break;
        }
    }
    return out;
}
