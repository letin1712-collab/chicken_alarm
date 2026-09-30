#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <time.h>
#include <esp_system.h>
#include <DFRobotDFPlayerMini.h>
#include "GithubOTA.h"

// --- Cấu hình Github OTA ---
#define FIRMWARE_VERSION "1.0.1"
#define OTA_VERSION_URL "https://raw.githubusercontent.com/letin1712-collab/chicken_alarm/refs/heads/main/version.txt?token=GHSAT0AAAAAAEGVXY6EKBYDTLJO6MYHKIRK2V4YHLQ" 
#define OTA_FIRMWARE_URL "https://github.com/letin1712-collab/chicken_alarm/releases/download/latest/firmware.bin"
// Nếu dùng Private Repository, hãy điền Github PAT (Personal Access Token) vào đây.
#define GITHUB_TOKEN "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" // Ví dụ: "ghp_xxxxxxxxxxxxxxxxxxxxxxxxxxx"

constexpr uint8_t DF_RX_PIN = 16;
constexpr uint8_t DF_TX_PIN = 17;
constexpr uint8_t DEFAULT_VOLUME = 20;
constexpr uint16_t DEFAULT_DURATION_MIN = 1;
constexpr uint16_t MAX_DURATION_MIN = 120;
constexpr long GMT_OFFSET_SEC = 7 * 3600;
constexpr uint32_t WIFI_TIMEOUT_MS = 15000;

const char *NTP_SERVER = "pool.ntp.org";
const char *AP_SSID = "chicken-alarm";
const char *AP_PASSWORD = "fuvitech.vn";

struct AlarmConfig {
  bool enabled;
  uint8_t hour;
  uint8_t minute;
  uint16_t durationMin;
  uint8_t volume;
};

constexpr uint8_t MAX_ALARMS = 10;
AlarmConfig alarmConfigs[MAX_ALARMS];
uint8_t alarmCount = 0;

DFRobotDFPlayerMini player;
WebServer server(80);
Preferences preferences;
String wifiSsid;
String wifiPassword;
bool playerReady = false;
bool isPlaying = false;
uint16_t trackCount = 0;
uint32_t playStartedAt = 0;
uint32_t alarmStartedAt = 0;
uint32_t alarmDurationMs = 0;
bool alarmSessionActive = false;
uint16_t lastRandomTrack = 0;
uint16_t currentTrack = 0;
uint8_t activeAlarmVolume = DEFAULT_VOLUME;
bool playFromRoot = false;
bool rootFallbackAttempted = false;
uint8_t consecutiveTrackErrors = 0;
uint8_t lastDfPlayerError = 0;
int32_t lastAlarmKeys[MAX_ALARMS];
uint32_t lastTimeSyncAttempt = 0;
uint32_t lastWifiAttempt = 0;

const char PAGE[] PROGMEM = R"HTML(
<!doctype html><html lang="vi"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>ESP32 Alarm</title>
<style>
:root{font:16px system-ui,sans-serif;color:#17202a;background:#f3f6f5}*{box-sizing:border-box}body{margin:0 auto;max-width:720px;padding:16px}header{display:flex;justify-content:space-between;align-items:center;border-bottom:1px solid #ccd5d2;padding:8px 0 14px}h1{font-size:22px;margin:0}section{padding:16px 0;border-bottom:1px solid #ccd5d2}h2{font-size:17px;margin:0 0 12px}.row{display:flex;gap:12px;align-items:center;flex-wrap:wrap}.grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:12px}label{display:grid;gap:5px;color:#45534f;font-size:14px}input,select,button{font:inherit;min-height:42px;border:1px solid #aebbb6;border-radius:5px;padding:8px 10px;background:white;color:#17202a}input[type=checkbox]{min-height:0;width:20px;height:20px;accent-color:#087f68}button{cursor:pointer;font-weight:600;background:#087f68;color:white;border-color:#087f68}button.secondary{background:#fff;color:#17202a;border-color:#aebbb6}button.danger{background:#a33d32;border-color:#a33d32}.status{font-size:14px;color:#45534f}.note{font-size:13px;line-height:1.5;color:#596963}.wide{width:100%}.alarm-list{display:grid;grid-template-columns:1fr;gap:12px;margin-top:12px}.alarm-item{border:1px solid #ccd5d2;border-radius:5px;padding:12px;background:#f9fbfa}.alarm-time{font-weight:600;font-size:18px;color:#087f68}.alarm-meta{font-size:13px;color:#596963;margin-top:4px}@media(max-width:480px){body{padding:12px}.grid{grid-template-columns:1fr}}
</style></head><body>
<header><h1>Báo thức ESP32</h1><span id="clock" class="status">Đang tải giờ…</span></header>
<section><h2>Danh sách báo thức</h2><div id="alarmsList" class="alarm-list">Đang tải...</div><div class="grid"><label>Giờ<input id="hour" type="number" min="0" max="23"></label><label>Phút<input id="minute" type="number" min="0" max="59"></label><label>Thời lượng (phút)<input id="duration" type="number" min="1" max="120"></label><label>Âm lượng (0–30)<input id="volume" type="number" min="0" max="30"></label></div><p class="note">Mỗi lần báo thức bắt đầu, hệ thống chọn ngẫu nhiên một bài trong thẻ nhớ và đổi bài ngẫu nhiên khi phát lặp.</p><div class="row"><button id="saveAlarmButton" onclick="saveAlarm()">Thêm báo thức</button><button class="secondary" onclick="cancelEdit()">Làm mới</button></div><div class="row" style="margin-top:12px"><label>Bài kiểm tra<select id="track"></select></label><button class="secondary" onclick="testTrack()">Phát thử</button><button class="danger" onclick="stopAudio()">Dừng phát</button></div></section>
<section><h2>Kết nối Wi‑Fi</h2><p class="status" id="wifiStatus">Đang kiểm tra…</p><div class="grid"><label>Tên Wi‑Fi<input id="ssid" autocomplete="username"></label><label>Mật khẩu Wi‑Fi<input id="password" type="password" autocomplete="new-password"></label></div><p><button class="secondary" onclick="saveWifi()">Lưu Wi‑Fi</button> <button class="secondary" onclick="rescan()">Quét lại số bài</button> <button class="secondary" onclick="ota()">Cập nhật OTA</button></p><p class="note">Nếu ESP32 chưa vào Wi‑Fi, kết nối điện thoại với mạng 192.168.4.1. Cần Internet để tự đồng bộ giờ NTP.</p></section>
<section><h2>Thẻ nhớ</h2><p class="note" id="cardNote">Đang đọc trạng thái thẻ…</p><p class="note">Có thể đặt file tên liên tục 0001.mp3, 0002.mp3… trong thư mục MP3 hoặc ngay gốc thẻ. Firmware thử cả hai kiểu phát. Thẻ nên định dạng FAT32. Nếu phát không được, kiểm tra Serial Monitor để xem đường dẫn nào lỗi. DFPlayer không hỗ trợ tải lên hoặc xóa file qua UART.</p></section>
<script>
const $=id=>document.getElementById(id);
async function api(path,options={}){const r=await fetch(path,{headers:{'Content-Type':'application/json'},...options});const j=await r.json();if(!r.ok)throw Error(j.error||'Có lỗi');return j}
function showError(e){alert(e.message||e)}
async function ota(){try{alert('Đang kiểm tra cập nhật từ GitHub. Vui lòng xem Serial Monitor!');await api('/api/ota',{method:'POST'});}catch(e){showError(e)}}
let clockBaseSeconds=null,clockBaseAt=0;
function drawClock(){if(clockBaseSeconds===null){$('clock').textContent='Chưa có giờ';return}const n=(clockBaseSeconds+Math.floor((Date.now()-clockBaseAt)/1000))%86400;$('clock').textContent=`${String(Math.floor(n/3600)).padStart(2,'0')}:${String(Math.floor(n/60)%60).padStart(2,'0')}:${String(n%60).padStart(2,'0')}`}
async function refreshStatus(){try{const s=await api('/api/status');if(s.timeValid){const t=s.time.split(':').map(Number);clockBaseSeconds=t[0]*3600+t[1]*60+t[2];clockBaseAt=Date.now()}drawClock();$('wifiStatus').textContent=s.wifiConnected?`Đã kết nối: ${s.ssid} · ${s.ip}`:`Chưa kết nối Wi‑Fi · AP: 192.168.4.1`;let card=s.playerReady?`DFPlayer sẵn sàng · ${s.trackCount} file · ${s.playing?'đang phát':'đã dừng'}`:'Chưa giao tiếp DFPlayer. Kiểm tra nguồn, dây UART, GND, thẻ SD.';if(s.currentTrack)card+=` · bài ${s.currentTrack}`;if(s.lastDfError===6)card+=' · lỗi 6: không tìm thấy file';else if(s.lastDfError)card+=` · lỗi DFPlayer ${s.lastDfError}`;if(s.rootPlayback)card+=' · đang phát theo số thứ tự gốc thẻ';$('cardNote').textContent=card;renderAlarms(s.alarms||[])}catch(e){console.error(e)}}
function renderAlarms(alarms){const html=alarms.length?alarms.map((a,i)=>`<div class="alarm-item"><div><input type="checkbox" ${a.enabled?'checked':''} onchange="toggleAlarm(${i})"><span class="alarm-time">${String(a.hour).padStart(2,'0')}:${String(a.minute).padStart(2,'0')}</span></div><div class="alarm-meta">Chọn bài ngẫu nhiên · âm lượng ${a.volume} · phát trong ${a.durationMin} phút</div><button class="secondary" onclick="editAlarm(${i})">Sửa</button> <button class="danger" onclick="deleteAlarm(${i})">Xóa</button></div>`).join(''):'<p class="note">Chưa có báo thức nào. Thêm báo thức mới.</p>';$('alarmsList').innerHTML=html}
let editingId=-1;
function cancelEdit(){editingId=-1;$('hour').value=8;$('minute').value=0;$('duration').value=1;$('volume').value=20;$('saveAlarmButton').textContent='Thêm báo thức'}
async function load(){try{const s=await api('/api/status');cancelEdit();const select=$('track');select.innerHTML='';for(let i=1;i<=s.trackCount;i++){const o=document.createElement('option');o.value=i;o.textContent=`Bài ${i}`;select.appendChild(o)}if(!s.trackCount){const o=document.createElement('option');o.value=1;o.textContent='Chưa đọc được danh sách bài';select.appendChild(o)}await refreshStatus()}catch(e){showError(e)}}
async function saveAlarm(){const wasEditing=editingId>=0;try{const body={id:editingId,hour:+$('hour').value,minute:+$('minute').value,durationMin:+$('duration').value,volume:+$('volume').value,enabled:true};await api('/api/alarm',{method:'POST',body:JSON.stringify(body)});await load();alert(wasEditing?'Đã cập nhật báo thức':'Đã thêm báo thức')}catch(e){showError(e)}}
async function editAlarm(id){try{const s=await api('/api/status');const a=s.alarms[id];if(!a)return;editingId=id;$('hour').value=a.hour;$('minute').value=a.minute;$('duration').value=a.durationMin;$('volume').value=a.volume;$('saveAlarmButton').textContent='Lưu thay đổi';window.scrollTo({top:0,behavior:'smooth'})}catch(e){showError(e)}}
async function deleteAlarm(id){if(!confirm('Xóa báo thức này?'))return;try{await api('/api/alarm/delete',{method:'POST',body:JSON.stringify({id})});await load()}catch(e){showError(e)}}
async function toggleAlarm(id){try{const s=await api('/api/status');const a=s.alarms[id];if(!a)return;await api('/api/alarm',{method:'POST',body:JSON.stringify({id,hour:a.hour,minute:a.minute,durationMin:a.durationMin,volume:a.volume,enabled:!a.enabled})});await refreshStatus()}catch(e){showError(e)}}
async function testTrack(){try{await api('/api/play',{method:'POST',body:JSON.stringify({track:+$('track').value,volume:+$('volume').value})});await load()}catch(e){showError(e)}}
async function stopAudio(){try{await api('/api/stop',{method:'POST'});await load()}catch(e){showError(e)}}
async function saveWifi(){try{await fetch('/api/wifi',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:new URLSearchParams({ssid:$('ssid').value,password:$('password').value})}).then(async r=>{const j=await r.json();if(!r.ok)throw Error(j.error)});alert('Đã lưu Wi‑Fi. ESP32 đang kết nối lại.');setTimeout(load,3000)}catch(e){showError(e)}}
async function rescan(){try{await api('/api/rescan',{method:'POST'});await load()}catch(e){showError(e)}}
load();setInterval(refreshStatus,10000);setInterval(drawClock,250);
</script></body></html>
)HTML";

void saveConfig() {
  preferences.begin("alarm", false);
  preferences.putUChar("count", alarmCount);
  for (uint8_t i = 0; i < alarmCount; i++) {
    String prefix = "a" + String(i) + "_";
    preferences.putBool((prefix + "en").c_str(), alarmConfigs[i].enabled);
    preferences.putUChar((prefix + "h").c_str(), alarmConfigs[i].hour);
    preferences.putUChar((prefix + "m").c_str(), alarmConfigs[i].minute);
    preferences.putUShort((prefix + "dur").c_str(), alarmConfigs[i].durationMin);
    preferences.putUChar((prefix + "vol").c_str(), alarmConfigs[i].volume);
  }
  preferences.end();
}

void loadConfig() {
  preferences.begin("alarm", false);
  alarmCount = preferences.getUChar("count", 0);
  if (alarmCount > MAX_ALARMS) alarmCount = MAX_ALARMS;
  
  for (uint8_t i = 0; i < alarmCount; i++) {
    String prefix = "a" + String(i) + "_";
    alarmConfigs[i].enabled = preferences.getBool((prefix + "en").c_str(), false);
    alarmConfigs[i].hour = preferences.getUChar((prefix + "h").c_str(), 8);
    alarmConfigs[i].minute = preferences.getUChar((prefix + "m").c_str(), 30);
    alarmConfigs[i].durationMin = preferences.getUShort((prefix + "dur").c_str(), DEFAULT_DURATION_MIN);
    alarmConfigs[i].volume = preferences.getUChar((prefix + "vol").c_str(), DEFAULT_VOLUME);
  }
  wifiSsid = preferences.getString("ssid", "");
  wifiPassword = preferences.getString("password", "");
  preferences.end();
}

void setTimeIfConnected() {
  if (WiFi.status() == WL_CONNECTED) {
    configTime(GMT_OFFSET_SEC, 0, NTP_SERVER, "time.google.com");
    lastTimeSyncAttempt = millis();
  }
}

void refreshTrackCount() {
  trackCount = 0;
  for (uint8_t attempt = 0; attempt < 5 && trackCount == 0; attempt++) {
    delay(350);
    int count = player.readFileCounts(DFPLAYER_DEVICE_SD);
    Serial.printf("Doc so file tren SD lan %u: %d\n", attempt + 1, count);
    if (count > 0) trackCount = count > 3000 ? 3000 : count;
  }
  Serial.printf("Tong so file nhan duoc: %u\n", trackCount);
}
void beginWiFi() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  if (wifiSsid.length()) {
    WiFi.begin(wifiSsid.c_str(), wifiPassword.c_str());
    uint32_t started = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - started < WIFI_TIMEOUT_MS) delay(250);
  }
  if (WiFi.status() == WL_CONNECTED) setTimeIfConnected();
  Serial.printf("Web AP: %s (192.168.4.1)\n", AP_SSID);
  if (WiFi.status() == WL_CONNECTED) Serial.printf("WiFi IP: %s\n", WiFi.localIP().toString().c_str());
}

bool initPlayer() {
  Serial2.begin(9600, SERIAL_8N1, DF_RX_PIN, DF_TX_PIN);
  delay(1200);
  if (!player.begin(Serial2, true, true)) return false;
  player.setTimeOut(500);
  player.outputDevice(DFPLAYER_DEVICE_SD);
  player.EQ(DFPLAYER_EQ_NORMAL);
  player.volume(DEFAULT_VOLUME);
  delay(700);
  refreshTrackCount();
  return true;
}

uint16_t chooseRandomTrack() {
  if (trackCount == 0) return 0;
  if (trackCount == 1) return lastRandomTrack = 1;
  uint16_t track = 1 + (esp_random() % trackCount);
  if (track == lastRandomTrack) track = track % trackCount + 1;
  lastRandomTrack = track;
  return track;
}

bool playTrack(uint16_t track, uint8_t volume) {
  if (!playerReady || track < 1 || track > 3000) {
    Serial.printf("playTrack failed: playerReady=%d, track=%u\n", playerReady, track);
    return false;
  }
  uint8_t vol = volume > 30 ? 30 : volume;
  if (isPlaying) player.stop();
  delay(120);
  player.volume(vol);
  delay(120);
  currentTrack = track;
  rootFallbackAttempted = false;
  if (playFromRoot) player.play(track);
  else player.playMp3Folder(track);
  isPlaying = true;
  playStartedAt = millis();
  Serial.printf("Playing track %u with volume %u\n", track, vol);
  return true;
}

void stopPlayback() {
  if (playerReady) player.stop();
  isPlaying = false;
  alarmStartedAt = 0;
  alarmDurationMs = 0;
  alarmSessionActive = false;
  consecutiveTrackErrors = 0;
}

void sendError(int code, const String &message) {
  String json = "{\"error\":\"" + message + "\"}";
  server.send(code, "application/json; charset=utf-8", json);
}

void handleStatus() {
  struct tm now;
  bool valid = time(nullptr) >= 1704067200 && getLocalTime(&now, 10);
  char timeText[24] = "--:--:--";
  if (valid) strftime(timeText, sizeof(timeText), "%H:%M:%S", &now);
  
  String json = "{\"timeValid\":" + String(valid ? "true" : "false") + ",\"time\":\"" + timeText +
    "\",\"playerReady\":" + String(playerReady ? "true" : "false") + ",\"playing\":" + String(isPlaying ? "true" : "false") +
    ",\"trackCount\":" + String(trackCount) +
    ",\"currentTrack\":" + String(currentTrack) + ",\"lastDfError\":" + String(lastDfPlayerError) +
    ",\"rootPlayback\":" + String(playFromRoot ? "true" : "false") +
    ",\"wifiConnected\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false") +
    ",\"ssid\":\"" + (WiFi.status() == WL_CONNECTED ? WiFi.SSID() : String("")) +
    "\",\"ip\":\"" + (WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : WiFi.softAPIP().toString()) +
    "\",\"alarms\":[";
  
  for (uint8_t i = 0; i < alarmCount; i++) {
    if (i > 0) json += ",";
    json += "{\"id\":" + String(i) + ",\"enabled\":" + String(alarmConfigs[i].enabled ? "true" : "false") +
      ",\"hour\":" + String(alarmConfigs[i].hour) + ",\"minute\":" + String(alarmConfigs[i].minute) +
      ",\"durationMin\":" + String(alarmConfigs[i].durationMin) +
      ",\"volume\":" + String(alarmConfigs[i].volume) + "}";
  }
  json += "]}";
  
  server.send(200, "application/json; charset=utf-8", json);
}

void handleSaveAlarm() {
  String body = server.arg("plain");
  
  auto number = [&body](const char *key, int fallback) -> int {
    String token = String("\"") + key + "\":";
    int at = body.indexOf(token);
    if (at < 0) return fallback;
    at += token.length();
    while (at < (int)body.length() && body[at] == ' ') at++;
    return static_cast<int>(body.substring(at).toInt());
  };
  
  int id = number("id", -1);
  bool enabled = body.indexOf("\"enabled\":true") >= 0;
  int h = number("hour", -1), m = number("minute", -1);
  int d = number("durationMin", -1), vol = number("volume", -1);
  
  if (h < 0 || h > 23 || m < 0 || m > 59 || d < 1 || d > MAX_DURATION_MIN || trackCount == 0 || vol < 0 || vol > 30) {
    sendError(400, "Giá trị giờ, phút, thời lượng hoặc âm lượng không hợp lệ; cần có file trên thẻ SD");
    return;
  }
  
  if (id >= 0 && id < (int)alarmCount) {
    // Update existing alarm
    alarmConfigs[id].hour = h;
    alarmConfigs[id].minute = m;
    alarmConfigs[id].durationMin = d;
    alarmConfigs[id].volume = vol;
    alarmConfigs[id].enabled = enabled;
  } else if (id == -1 && alarmCount < MAX_ALARMS) {
    // Add new alarm
    alarmConfigs[alarmCount].hour = h;
    alarmConfigs[alarmCount].minute = m;
    alarmConfigs[alarmCount].durationMin = d;
    alarmConfigs[alarmCount].volume = vol;
    alarmConfigs[alarmCount].enabled = enabled;
    alarmCount++;
  } else {
    sendError(400, "ID báo thức không hợp lệ hoặc đã đủ số báo thức tối đa");
    return;
  }
  
  saveConfig();
  server.send(200, "application/json", "{\"ok\":true}");
}

void handleDeleteAlarm() {
  String body = server.arg("plain");
  int id = -1;
  String token = "\"id\":";
  int at = body.indexOf(token);
  if (at >= 0) {
    at += token.length();
    while (at < (int)body.length() && body[at] == ' ') at++;
    id = body.substring(at).toInt();
  }
  
  if (id < 0 || id >= (int)alarmCount) {
    sendError(400, "ID báo thức không hợp lệ");
    return;
  }
  
  for (uint8_t i = id; i < alarmCount - 1; i++) {
    alarmConfigs[i] = alarmConfigs[i + 1];
  }
  alarmCount--;
  
  saveConfig();
  server.send(200, "application/json", "{\"ok\":true}");
}

void handlePlay() {
  String body = server.arg("plain");
  int at = body.indexOf("\"track\":");
  if (at < 0) { sendError(400, "Thiếu số bài"); return; }
  int track = body.substring(at + 8).toInt();
  int volAt = body.indexOf("\"volume\":");
  int vol = volAt < 0 ? DEFAULT_VOLUME : body.substring(volAt + 9).toInt();
  if (!playerReady) { sendError(503, "DFPlayer chưa sẵn sàng"); return; }
  if (track < 1 || track > trackCount || vol < 0 || vol > 30) { sendError(400, "Bài hoặc âm lượng không hợp lệ"); return; }
  playTrack(track, vol);
  server.send(200, "application/json", "{\"ok\":true}");
}

void handleWiFiSave() {
  wifiSsid = server.arg("ssid");
  wifiPassword = server.arg("password");
  if (!wifiSsid.length()) { sendError(400, "Tên Wi-Fi không được để trống"); return; }
  preferences.begin("alarm", false);
  preferences.putString("ssid", wifiSsid); preferences.putString("password", wifiPassword);
  preferences.end();
  WiFi.begin(wifiSsid.c_str(), wifiPassword.c_str());
  server.send(200, "application/json", "{\"ok\":true}");
}

void handleRescan() {
  if (!playerReady) playerReady = initPlayer();
  else refreshTrackCount();
  Serial.printf("Rescan: playerReady=%d, trackCount=%u\n", playerReady, trackCount);
  server.send(200, "application/json", "{\"ok\":true}");
}

void startAlarm(uint8_t idx) {
  if (idx >= alarmCount) return;
  if (!playerReady || trackCount == 0) {
    Serial.printf("Bao thuc %u khong phat: DFPlayer chua san sang hoac the khong co file\n", idx);
    return;
  }
  if (alarmSessionActive) {
    Serial.printf("Bo qua bao thuc %u vi dang co bao thuc khac phat\n", idx);
    return;
  }
  uint16_t track = chooseRandomTrack();
  if (!playTrack(track, alarmConfigs[idx].volume)) return;
  alarmStartedAt = millis();
  alarmDurationMs = (uint32_t)alarmConfigs[idx].durationMin * 60000UL;
  alarmSessionActive = true;
  activeAlarmVolume = alarmConfigs[idx].volume;
  Serial.printf("Bao thuc %u: %02u:%02u, ngau nhien bai %u\n", idx, alarmConfigs[idx].hour, alarmConfigs[idx].minute, track);
}

void checkAlarm() {
  if (time(nullptr) < 1704067200) return;
  struct tm now;
  if (!getLocalTime(&now, 10)) return;
  
  for (uint8_t i = 0; i < alarmCount; i++) {
    if (!alarmConfigs[i].enabled) continue;
    if (now.tm_hour != alarmConfigs[i].hour || now.tm_min != alarmConfigs[i].minute) continue;
    
    int32_t key = now.tm_yday * 1440 + now.tm_hour * 60 + now.tm_min;
    if (lastAlarmKeys[i] != key) {
      lastAlarmKeys[i] = key;
      startAlarm(i);
    }
  }
}

void setupWeb() {
  server.on("/", HTTP_GET, [] { server.send_P(200, "text/html; charset=utf-8", PAGE); });
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/alarm", HTTP_POST, handleSaveAlarm);
  server.on("/api/alarm/delete", HTTP_POST, handleDeleteAlarm);
  server.on("/api/play", HTTP_POST, handlePlay);
  server.on("/api/stop", HTTP_POST, [] { stopPlayback(); server.send(200, "application/json", "{\"ok\":true}"); });
  server.on("/api/wifi", HTTP_POST, handleWiFiSave);
  server.on("/api/rescan", HTTP_POST, handleRescan);
  server.on("/api/ota", HTTP_POST, [] { 
    server.send(200, "application/json", "{\"ok\":true}"); 
    GithubOTA::checkAndUpdate(OTA_VERSION_URL, OTA_FIRMWARE_URL, FIRMWARE_VERSION, GITHUB_TOKEN); 
  });
  server.onNotFound([] { sendError(404, "Không tìm thấy trang"); });
  server.begin();
}

void setup() {
  Serial.begin(115200);
  delay(300);
  for (uint8_t i = 0; i < MAX_ALARMS; i++) lastAlarmKeys[i] = -1;
  loadConfig();
  playerReady = initPlayer();
  if (!playerReady) Serial.println("DFPlayer chưa sẵn sàng; web vẫn hoạt động.");
  beginWiFi();
  setupWeb();
  Serial.println("Mo http://192.168.4.1 hoac IP ESP32 hien tren Serial.");
}

void loop() {
  server.handleClient();
  static bool wasWifiConnected = false;
  bool wifiConnected = WiFi.status() == WL_CONNECTED;
  if (wifiConnected && !wasWifiConnected) setTimeIfConnected();
  wasWifiConnected = wifiConnected;
  if (playerReady && player.available()) {
    uint8_t type = player.readType();
    int value = player.read();
    if (type == DFPlayerPlayFinished && isPlaying) {
      lastDfPlayerError = 0;
      consecutiveTrackErrors = 0;
      if (millis() - playStartedAt > 1000 && alarmSessionActive) {
        if (millis() - alarmStartedAt < alarmDurationMs) {
          uint16_t nextTrack = chooseRandomTrack();
          if (nextTrack) playTrack(nextTrack, activeAlarmVolume);
          else stopPlayback();
        } else stopPlayback();
      } else if (!alarmSessionActive) {
        isPlaying = false;
      }
    } else if (type == DFPlayerError) {
      lastDfPlayerError = value;
      if (value == FileMismatch && !playFromRoot && !rootFallbackAttempted) {
        playFromRoot = true;
        rootFallbackAttempted = true;
        player.play(currentTrack);
        isPlaying = true;
        playStartedAt = millis();
        lastDfPlayerError = 0;
        Serial.printf("FileMismatch /MP3/%04u.mp3; dang thu bai %u tai goc the\n", currentTrack, currentTrack);
      } else {
        isPlaying = false;
        Serial.printf("Loi DFPlayer %u khi phat bai %u\n", value, currentTrack);
        if (value == FileMismatch && alarmSessionActive && ++consecutiveTrackErrors < trackCount) {
          uint16_t nextTrack = chooseRandomTrack();
          if (nextTrack) playTrack(nextTrack, activeAlarmVolume);
          else stopPlayback();
        } else if (alarmSessionActive) stopPlayback();
      }
    }
  }

  static uint32_t lastCheck = 0;
  if (millis() - lastCheck >= 500) { lastCheck = millis(); checkAlarm(); }
  if (alarmSessionActive && millis() - alarmStartedAt >= alarmDurationMs) stopPlayback();
  if (WiFi.status() == WL_CONNECTED && millis() - lastTimeSyncAttempt > 6UL * 60UL * 60UL * 1000UL) setTimeIfConnected();
  if (WiFi.status() != WL_CONNECTED && wifiSsid.length() && millis() - lastWifiAttempt > 30000) {
    lastWifiAttempt = millis(); WiFi.begin(wifiSsid.c_str(), wifiPassword.c_str());
  }
}
