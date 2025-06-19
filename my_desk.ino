#include <WiFi.h>
#include <HTTPClient.h>
#include <TFT_eSPI.h>
#include <XPT2046_Bitbang.h>

// =================================================================================
// >> 1. CONFIGURE YOUR SETTINGS HERE <<
// =================================================================================

// -- Your Wi-Fi Credentials --
const char* ssid = "ssid";
const char* password = "password";

// -- The IP address of the server you want to control --
const char* serverName = "192.168.2.105";

// -- Touchscreen Pins (from the CYD example) --
#define XPT2046_IRQ  36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK  25
#define XPT2046_CS   33

// =================================================================================
// Global Objects
// =================================================================================

TFT_eSPI tft = TFT_eSPI();
XPT2046_Bitbang ts(XPT2046_MOSI, XPT2046_MISO, XPT2046_CLK, XPT2046_CS);

TFT_eSPI_Button buttons[2];
#define SIT_BUTTON   0
#define STAND_BUTTON 1

#define STATUS_Y 80

//====================================================================================
//                                 HELPER FUNCTIONS
//====================================================================================

void clearStatusArea() {
  tft.fillRect(0, STATUS_Y - 15, tft.width(), 40, TFT_BLACK);
}

void drawUI() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextDatum(MC_DATUM);

  tft.drawString("Desk Controller", tft.width() / 2, 25, 4);

  buttons[SIT_BUTTON].initButton(&tft, tft.width() / 4, 160, 120, 60, TFT_WHITE, TFT_BLUE, TFT_WHITE, "SIT", 4);
  buttons[STAND_BUTTON].initButton(&tft, (tft.width() / 4) * 3, 160, 120, 60, TFT_WHITE, TFT_RED, TFT_WHITE, "STAND", 4);
  
  buttons[SIT_BUTTON].drawButton();
  buttons[STAND_BUTTON].drawButton();
}

void sendRequest(const char* endpoint, bool printResponse) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String serverPath = "http://" + String(serverName) + String(endpoint);

    Serial.print("Making request to: ");
    Serial.println(serverPath);
    
    clearStatusArea();
    tft.drawString("Sending...", tft.width() / 2, STATUS_Y, 2);

    http.begin(serverPath.c_str());
    int httpCode = http.GET();

    clearStatusArea();
    if (httpCode > 0) {
      String payload = http.getString();
      if (printResponse) {
        tft.drawString("Status: " + payload, tft.width() / 2, STATUS_Y, 2);
      } else {
        tft.drawString("Command sent!", tft.width() / 2, STATUS_Y, 2);
      }
    } else {
      tft.drawString("Error: " + String(httpCode), tft.width() / 2, STATUS_Y, 2);
    }
    http.end();
  } else {
    clearStatusArea();
    tft.drawString("WiFi disconnected!", tft.width() / 2, STATUS_Y, 2);
  }
}

//====================================================================================
//                                      SETUP
//====================================================================================

void setup() {
  Serial.begin(115200);
  tft.init();
  tft.setRotation(1);
  ts.begin();
  // ts.setRotation(1);
  drawUI();

  tft.setTextDatum(MC_DATUM);
  tft.drawString("Connecting to WiFi...", tft.width() / 2, STATUS_Y, 2);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");
  
  clearStatusArea();
  tft.drawString("WiFi Connected!", tft.width() / 2, STATUS_Y, 2);
  delay(1000);

  sendRequest("/status", true);
}


//====================================================================================
//                                       LOOP
//====================================================================================

void loop() {
  // Use 'auto' to let the compiler figure out the correct type for 'p'
  // This resolves the "TouchPoint was not declared in this scope" error.
  auto p = ts.getTouch();
  
  // 1. Update the 'pressed' state of ALL buttons based on the touch point.
  for (uint8_t i = 0; i < 2; i++) {
    if (p.zRaw > 0 && buttons[i].contains(p.x, p.y)) {
      buttons[i].press(true);
    } else {
      buttons[i].press(false);
    }
  }

  // 2. Now, check each button for a state CHANGE and react.
  if (buttons[SIT_BUTTON].justPressed()) {
    buttons[SIT_BUTTON].drawButton(true);
    sendRequest("/s", false);
  }
  if (buttons[SIT_BUTTON].justReleased()) {
    buttons[SIT_BUTTON].drawButton(false);
  }

  if (buttons[STAND_BUTTON].justPressed()) {
    buttons[STAND_BUTTON].drawButton(true);
    sendRequest("/t", false);
  }
  if (buttons[STAND_BUTTON].justReleased()) {
    buttons[STAND_BUTTON].drawButton(false);
  }
  
  delay(20);
}
