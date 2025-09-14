// ESP32 BLE UART — 4-motor robot with FORWARD/BACKWARD/LEFT/RIGHT/STOP
// Base speed set via "PWM <0-255>" command

#include <Arduino.h>       // Core Arduino types/functions (pinMode, digitalWrite, etc.)
#include <NimBLEDevice.h>  // NimBLE BLE stack for ESP32 (lighter than classic BLE library)
#include <WiFi.h>          // Wi-Fi STA/AP support for ESP32
#include <WebServer.h>     // Lightweight HTTP server (synchronous) shipped with ESP32 core

// ===================== YOUR PINS =====================
// Four motor channels: each motor has a PWM pin (speed) and a DIR pin (direction).
// Change these to whatever pins your motor driver uses.
const int PWM_PIN[4] = {18, 19, 23, 5};   // Motor PWM pins
const int DIR_PIN[4] = {17, 16, 4, 0};    // Motor DIR pins

// ===================== BLE UUIDs =====================
// Nordic UART Service UUIDs (NUS) so generic BLE terminal apps/web can talk "UART-like".
#define SERVICE_UUID        "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_RX   "6E400002-B5A3-F393-E0A9-E50E24DCCA9E" // Phone writes commands here
#define CHARACTERISTIC_TX   "6E400003-B5A3-F393-E0A9-E50E24DCCA9E" // ESP notifies responses here

// ===================== WIFI CREDS ====================
// Your Wi-Fi credentials (2.4 GHz network). These are used to join your LAN.
const char* WIFI_SSID = "WE_B38285";      // <-- your SSID
const char* WIFI_PASS = "*100200300*";    // <-- your password

// ===================== GLOBALS =======================
// Pointers for BLE server and notify characteristic
NimBLEServer* pServer = nullptr;
NimBLECharacteristic* pTxChar = nullptr;

// HTTP server on port 80 (http://<IP>/ ...)
WebServer http(80);

// Base PWM (0..255) used by movement functions; updated by "PWM <val>" command
int baseSpeed = 150;

// Forward declarations for movement and command handling
void moveForward(); void moveBackward(); void moveLeft(); void moveRight(); void stopAll();
String handleCommand(const String& cmd);

// ===================== MOTOR CONTROL =================
// Low-level function that sets ONE motor's direction and speed.
void setMotor(uint8_t i, bool forward, int speed) {
  if (i >= 4) return;                                  // protect against bad index
  digitalWrite(DIR_PIN[i], forward ? LOW : HIGH);      // direction line: LOW=fwd, HIGH=rev (flip if needed)
  analogWrite(PWM_PIN[i], constrain(speed, 0, 255));   // speed via analogWrite (ESP32 core maps to LEDC PWM)
  Serial.printf("Motor %d → DIR=%s, PWM=%d\n", i+1, forward ? "FWD" : "REV", speed);
}

// High-level movement helpers that call setMotor for all four motors.
// Tweak which motors are fwd/rev if your chassis orientation differs.

void moveForward()  {
  Serial.printf("CMD: FORWARD @ PWM=%d\n", baseSpeed);
  for (int i=0;i<4;i++) setMotor(i, true, baseSpeed);
}

void moveBackward() {
  Serial.printf("CMD: BACKWARD @ PWM=%d\n", baseSpeed);
  for (int i=0;i<4;i++) setMotor(i, false, baseSpeed);
}

void moveLeft() {
  Serial.printf("CMD: LEFT @ PWM=%d\n", baseSpeed);
  // Left turn: left side reverse, right side forward (tank steer)
  setMotor(0,false,baseSpeed);  // front-left
  setMotor(1,false,baseSpeed);  // rear-left
  setMotor(2,true, baseSpeed);  // front-right
  setMotor(3,true, baseSpeed);  // rear-right
}

void moveRight() {
  Serial.printf("CMD: RIGHT @ PWM=%d\n", baseSpeed);
  // Right turn: left side forward, right side reverse
  setMotor(0,true, baseSpeed);
  setMotor(1,true, baseSpeed);
  setMotor(2,false,baseSpeed);
  setMotor(3,false,baseSpeed);
}

void stopAll() {
  Serial.println("CMD: STOP (all motors off)");
  for (int i=0;i<4;i++) analogWrite(PWM_PIN[i], 0); // set PWM=0 to stop
}

// ===================== COMMAND ROUTER =================
// Central parser that understands simple text commands from BLE or HTTP.
// Returns a short status string; also notifies BLE client if connected.
String handleCommand(const String& cmdIn) {
  String cmd = cmdIn; cmd.trim();     // trim whitespace/newlines
  String out;                         // response text

  if      (cmd.equalsIgnoreCase("FORWARD"))  { moveForward();  out = "OK: FORWARD @"  + String(baseSpeed); }
  else if (cmd.equalsIgnoreCase("BACKWARD")) { moveBackward(); out = "OK: BACKWARD @" + String(baseSpeed); }
  else if (cmd.equalsIgnoreCase("LEFT"))     { moveLeft();     out = "OK: LEFT @"     + String(baseSpeed); }
  else if (cmd.equalsIgnoreCase("RIGHT"))    { moveRight();    out = "OK: RIGHT @"    + String(baseSpeed); }
  else if (cmd.equalsIgnoreCase("STOP"))     { stopAll();      out = "OK: STOP"; }
  else if (cmd.startsWith("PWM ")) {                 // expects "PWM <0..255>"
    int v = constrain(cmd.substring(4).toInt(), 0, 255);
    baseSpeed = v;                                  // set new base speed
    Serial.printf("Base speed updated → %d\n", baseSpeed);
    out = "OK: BaseSpeed=" + String(baseSpeed);
  } else {
    out = "ERR: Unknown (use FORWARD/BACKWARD/LEFT/RIGHT/STOP/PWM <0-255>)";
    Serial.printf("Unknown command: %s\n", cmd.c_str());
  }

  // Send response to any BLE subscriber (your web/phone client can display it)
  if (pTxChar) { pTxChar->setValue(out.c_str()); pTxChar->notify(); }
  return out;  // HTTP path also returns this to the browser
}

// ===================== BLE CALLBACKS (portable) ==================
// We implement BOTH onWrite signatures (with and without NimBLEConnInfo)
// and avoid 'override' so it compiles across NimBLE versions.

class RxCallback : public NimBLECharacteristicCallbacks {
public:
  void onWrite(NimBLECharacteristic* c) {                    // 1-arg version
    String cmd = String(c->getValue().c_str());             // read written bytes as string
    handleCommand(cmd);                                     // reuse central router
  }
  void onWrite(NimBLECharacteristic* c, NimBLEConnInfo&) {   // 2-arg version
    onWrite(c);                                             // delegate to the 1-arg body
  }
};

class MyServerCallbacks : public NimBLEServerCallbacks {
public:
  void onDisconnect(NimBLEServer* s) {                       // 1-arg version
    (void)s;
    Serial.println("BLE: Central disconnected, re-advertising");
    NimBLEDevice::startAdvertising();                        // keep advertising for next client
  }
  void onDisconnect(NimBLEServer* s, NimBLEConnInfo&) {      // 2-arg version
    onDisconnect(s);                                        // delegate
  }
};

// ===================== WIFI + HTTP ===================
// Small helpers to send CORS headers so browsers can call from other origins.
static void sendTextWithCORS(const String& body, int code=200) {
  http.sendHeader("Access-Control-Allow-Origin", "*");       // allow any origin
  http.sendHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
  http.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  http.send(code, "text/plain", body);                       // send plain text response
}

void setupWifiAndHttp() {
  Serial.printf("Wi-Fi: connecting to %s ...\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);                      // station mode (join existing Wi-Fi)
  WiFi.begin(WIFI_SSID, WIFI_PASS);         // start connection

  // Wait up to ~15s to connect, printing dots for progress
  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) { delay(300); Serial.print("."); }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\nWi-Fi connected. IP: %s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("\nWi-Fi connect failed (timeout). Keeping BLE only.");
    return;  // don’t start HTTP if not connected
  }

  // Generic handler: if browser sends OPTIONS (CORS preflight), return 204.
  http.onNotFound([](){
    if (http.method() == HTTP_OPTIONS) {
      http.sendHeader("Access-Control-Allow-Origin", "*");
      http.sendHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
      http.sendHeader("Access-Control-Allow-Headers", "Content-Type");
      http.send(204);
    } else {
      sendTextWithCORS("ERR: Not Found", 404);
    }
  });

  // Root path (GET /): shows a tiny help and the IP.
  http.on("/", HTTP_GET, [](){
    String info = "ESP32 Robot API\nUse /cmd?c=FORWARD or /cmd?c=PWM%20180 or /cmd?c=STOP\n";
    info += "IP: " + WiFi.localIP().toString() + "\n";
    sendTextWithCORS(info);
  });

  // Command endpoint (GET /cmd?c=...): takes "c" parameter and passes to handleCommand.
  // Note: WebServer uses hasArg()/arg(), not hasParam()/getParam().
  http.on("/cmd", HTTP_GET, [](){
    if (!http.hasArg("c")) {                            // ensure the command param exists
      sendTextWithCORS("ERR: missing c param", 400);
      return;
    }
    String cmd = http.arg("c");                         // read the command string
    String out = handleCommand(cmd);                    // route to the same handler as BLE
    sendTextWithCORS(out);                              // respond to the browser
  });

  http.begin();                                         // start listening on port 80
  Serial.println("HTTP server started at /cmd");
}

void httpLoop() { http.handleClient(); }                // must be called often in loop()

// ===================== SETUP / LOOP ==================
void setup() {
  Serial.begin(115200);                                 // open serial for logs

  // Configure all motor pins and stop motors initially
  for (int i=0; i<4; i++) {
    pinMode(DIR_PIN[i], OUTPUT);
    pinMode(PWM_PIN[i], OUTPUT);
    digitalWrite(DIR_PIN[i], LOW);                      // default forward direction (change if needed)
    analogWrite(PWM_PIN[i], 0);                         // motors off at boot
  }

  // --- BLE initialization ---
  NimBLEDevice::init("CyberTruck");                     // advertised BLE device name
  pServer = NimBLEDevice::createServer();               // create BLE GATT server
  pServer->setCallbacks(new MyServerCallbacks());       // handle disconnect → re-advertise

  // Create NUS-like service and characteristics
  auto service = pServer->createService(SERVICE_UUID);
  pTxChar = service->createCharacteristic(CHARACTERISTIC_TX, NIMBLE_PROPERTY::NOTIFY); // notify responses
  auto rx = service->createCharacteristic(CHARACTERISTIC_RX,
                                          NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR); // phone writes
  rx->setCallbacks(new RxCallback());                   // handle incoming commands over BLE

  service->start();                                     // start the service
  auto adv = NimBLEDevice::getAdvertising();            // create advertiser
  adv->addServiceUUID(SERVICE_UUID);                    // advertise the service UUID
  adv->start();                                         // begin advertising

  // --- Wi-Fi + HTTP initialization ---
  setupWifiAndHttp();                                   // connect to Wi-Fi and start HTTP API

  Serial.println("Ready: FORWARD / BACKWARD / LEFT / RIGHT / STOP; PWM <0-255>");
}

void loop() {
  httpLoop();   // keep HTTP server responsive (process incoming HTTP requests)
}
