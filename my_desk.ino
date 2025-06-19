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
// Global Objects & State
// =================================================================================

TFT_eSPI tft = TFT_eSPI();
XPT2046_Bitbang ts(XPT2046_MOSI, XPT2046_MISO, XPT2046_CLK, XPT2046_CS);

TFT_eSPI_Button buttons[3];
#define SIT_BUTTON   0
#define STAND_BUTTON 1
#define AUTO_BUTTON  2

// -- UI Layout Constants (Adjusted for smaller fonts) --
#define TITLE_Y    15
#define STATUS_Y   40
#define TIMER_Y    70  // Y-position for the timer block
#define BUTTON_Y   140 // Y-position for the top row of buttons

// -- Timer Mode State --
bool timerModeActive = false;
enum TimerState { SITTING, STANDING };
TimerState currentTimerState;
unsigned long lastStateChangeTime = 0;
const unsigned long stateDuration = 60 * 1000;
int lastSecondsDisplayed = -1;

//====================================================================================
//                                 HELPER FUNCTIONS
//====================================================================================

void clearStatusArea() {
  tft.fillRect(0, STATUS_Y - 10, tft.width(), 20, TFT_BLACK);
}

void clearTimerArea() {
  tft.fillRect(0, TIMER_Y - 10, tft.width(), 50, TFT_BLACK);
}

void drawUI() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextDatum(MC_DATUM);

  // Using font 2 for title
  tft.drawString("Desk Controller", tft.width() / 2, TITLE_Y, 2);

  // Using smaller font 1 for button labels
  buttons[SIT_BUTTON].initButton(&tft, tft.width() / 4, BUTTON_Y, 80, 50, TFT_WHITE, TFT_BLUE, TFT_WHITE, "SIT", 2);
  buttons[STAND_BUTTON].initButton(&tft, (tft.width() / 4) * 3, BUTTON_Y, 80, 50, TFT_WHITE, TFT_RED, TFT_WHITE, "STAND", 2);
  buttons[AUTO_BUTTON].initButton(&tft, tft.width() / 2, BUTTON_Y + 65, 100, 50, TFT_WHITE, TFT_DARKGREEN, TFT_WHITE, "AUTO", 2);
  
  buttons[SIT_BUTTON].drawButton();
  buttons[STAND_BUTTON].drawButton();
  buttons[AUTO_BUTTON].drawButton();
}

void sendRequest(const char* endpoint) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String serverPath = "http://" + String(serverName) + String(endpoint);
    
    clearStatusArea();
    // Using smaller font 1 for status messages
    tft.drawString("Sending...", tft.width() / 2, STATUS_Y, 1);

    http.begin(serverPath.c_str());
    int httpCode = http.GET();

    clearStatusArea();
    if (httpCode <= 0) {
      tft.drawString("Request Error: " + String(httpCode), tft.width() / 2, STATUS_Y, 1);
    } else {
      tft.drawString("Command OK", tft.width() / 2, STATUS_Y, 1);
    }
    http.end();
  } else {
    clearStatusArea();
    tft.drawString("WiFi disconnected!", tft.width() / 2, STATUS_Y, 1);
  }
}

void handleTimer() {
  if (!timerModeActive) return;

  unsigned long currentTime = millis();
  unsigned long elapsedTime = currentTime - lastStateChangeTime;

  if (elapsedTime >= stateDuration) {
    currentTimerState = (currentTimerState == SITTING) ? STANDING : SITTING;
    sendRequest(currentTimerState == SITTING ? "/s" : "/t");
    lastStateChangeTime = currentTime;
    elapsedTime = 0;
  }

  int secondsRemaining = (stateDuration - elapsedTime) / 1000;
  if (secondsRemaining != lastSecondsDisplayed) {
    clearTimerArea();
    tft.setTextDatum(MC_DATUM);

    String modeText = "Mode: Auto (" + String(currentTimerState == SITTING ? "Sit" : "Stand") + ")";
    // Using smaller font 1 for the mode text
    tft.drawString(modeText, tft.width() / 2, TIMER_Y, 1);

    String timerText = String(secondsRemaining) + "s";
    // Using smaller font 2 for the main countdown to ensure it fits
    tft.drawString(timerText, tft.width() / 2, TIMER_Y + 20, 2);
    
    lastSecondsDisplayed = secondsRemaining;
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
  tft.drawString("Connecting to WiFi...", tft.width() / 2, STATUS_Y, 1);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  
  clearStatusArea();
  tft.drawString("WiFi Connected!", tft.width() / 2, STATUS_Y, 1);
  delay(1000);
  clearStatusArea();
}

//====================================================================================
//                                       LOOP
//====================================================================================

void loop() {
  handleTimer();
  auto p = ts.getTouch();
  
  for (uint8_t i = 0; i < 3; i++) {
    if (p.zRaw > 0 && buttons[i].contains(p.x, p.y)) {
      buttons[i].press(true);
    } else {
      buttons[i].press(false);
    }
  }

  if (!timerModeActive) {
    if (buttons[SIT_BUTTON].justPressed()) {
      buttons[SIT_BUTTON].drawButton(true); sendRequest("/s");
    }
    if (buttons[SIT_BUTTON].justReleased()) {
      buttons[SIT_BUTTON].drawButton(false);
    }
    if (buttons[STAND_BUTTON].justPressed()) {
      buttons[STAND_BUTTON].drawButton(true); sendRequest("/t");
    }
    if (buttons[STAND_BUTTON].justReleased()) {
      buttons[STAND_BUTTON].drawButton(false);
    }
  }

  if (buttons[AUTO_BUTTON].justPressed()) {
    buttons[AUTO_BUTTON].drawButton(true);
    timerModeActive = !timerModeActive;

    if (timerModeActive) {
      currentTimerState = STANDING;
      sendRequest("/t");
      lastStateChangeTime = millis();
      lastSecondsDisplayed = -1;
      buttons[AUTO_BUTTON].drawButton(false, "STOP");
    } else {
      clearTimerArea();
      buttons[AUTO_BUTTON].drawButton(false, "AUTO");
    }
  }
  if (buttons[AUTO_BUTTON].justReleased()) {
     if (!timerModeActive) buttons[AUTO_BUTTON].drawButton(false, "AUTO");
  }
  
  delay(20);
}
