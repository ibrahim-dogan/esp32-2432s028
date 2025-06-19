#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Bitbang.h>

// --- TOUCH SCREEN PINS (for ESP32 Cheap Yellow Display) ---
#define XPT2046_IRQ  36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK  25
#define XPT2046_CS   33

// --- LIBRARY INSTANCES ---
TFT_eSPI tft = TFT_eSPI();
XPT2046_Bitbang ts(XPT2046_MOSI, XPT2046_MISO, XPT2046_CLK, XPT2046_CS);
TFT_eSPI_Button screenButton;

// --- SCREEN & LAYOUT ---
#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 240
#define GROUND_Y      (SCREEN_HEIGHT - 20)
#define SKY_COLOR     0x64B9 // A nice light blue
#define GROUND_COLOR  0x5A40 // A dirt/grass brown

// --- GAME STATE ---
enum GameState { STATE_START_SCREEN, STATE_PLAYING, STATE_GAME_OVER };
GameState gameState = STATE_START_SCREEN;

// --- BIRD PROPERTIES ---
#define BIRD_X        80
#define BIRD_RADIUS   10
float bird_y, last_bird_y;
float bird_vy;

// --- GAMEPLAY CONSTANTS ---
#define GRAVITY        0.3
#define FLAP_STRENGTH -6.0

// --- PIPE PROPERTIES ---
#define NUM_PIPES     2
#define PIPE_WIDTH    50
#define PIPE_GAP      100
#define PIPE_SPEED    3
float pipe_x[NUM_PIPES];
int pipe_gap_y[NUM_PIPES];
float last_pipe_x[NUM_PIPES];
bool pipe_passed[NUM_PIPES]; // <<<<<<<<<<< THIS LINE WAS MISSING

// --- SCORE ---
int score = 0;
int highScore = 0;
int lastDrawnScore = -1;

//====================================================================================
//                                   SETUP
//====================================================================================
void setup() {
  Serial.begin(115200);
  tft.init();
  tft.setRotation(1);
  ts.begin();
  screenButton.initButton(&tft, tft.width()/2, tft.height()/2, tft.width(), tft.height(), TFT_BLACK, TFT_BLACK, TFT_BLACK, "", 0);
  randomSeed(analogRead(A0));
  showStartScreen();
}

//====================================================================================
//                                GAME SETUP & RESET
//====================================================================================
void setupNewGame() {
  tft.fillScreen(SKY_COLOR);
  tft.fillRect(0, GROUND_Y, SCREEN_WIDTH, SCREEN_HEIGHT - GROUND_Y, GROUND_COLOR);
  tft.fillRect(0, GROUND_Y, SCREEN_WIDTH, 4, TFT_GREEN);

  bird_y = SCREEN_HEIGHT / 2.0;
  last_bird_y = bird_y;
  bird_vy = 0;
  score = 0;
  lastDrawnScore = -1;

  for (int i = 0; i < NUM_PIPES; i++) {
    pipe_x[i] = SCREEN_WIDTH + i * (SCREEN_WIDTH / 2 + PIPE_WIDTH / 2);
    last_pipe_x[i] = pipe_x[i];
    pipe_gap_y[i] = random(PIPE_GAP / 2 + 10, GROUND_Y - PIPE_GAP / 2 - 10);
    pipe_passed[i] = false;
  }
}

//====================================================================================
//                          SMART DRAWING FUNCTIONS
//====================================================================================
void eraseBird() {
  tft.fillCircle(BIRD_X, (int)last_bird_y, BIRD_RADIUS + 1, SKY_COLOR);
}

void drawBird() {
  tft.fillCircle(BIRD_X, (int)bird_y, BIRD_RADIUS, TFT_YELLOW);
  tft.fillCircle(BIRD_X + 4, (int)bird_y - 2, 3, TFT_WHITE);
  tft.fillCircle(BIRD_X + 5, (int)bird_y - 2, 1, TFT_BLACK);
}

void erasePipe(int i) {
  tft.fillRect((int)last_pipe_x[i], 0, PIPE_WIDTH + PIPE_SPEED, GROUND_Y, SKY_COLOR);
}

void drawPipe(int i) {
  tft.fillRect(pipe_x[i], 0, PIPE_WIDTH, pipe_gap_y[i] - PIPE_GAP / 2, TFT_GREEN);
  tft.fillRect(pipe_x[i]-2, pipe_gap_y[i] - PIPE_GAP / 2 - 20, PIPE_WIDTH+4, 20, TFT_DARKGREEN);
  tft.fillRect(pipe_x[i], pipe_gap_y[i] + PIPE_GAP / 2, PIPE_WIDTH, GROUND_Y - (pipe_gap_y[i] + PIPE_GAP / 2), TFT_GREEN);
  tft.fillRect(pipe_x[i]-2, pipe_gap_y[i] + PIPE_GAP / 2, PIPE_WIDTH+4, 20, TFT_DARKGREEN);
}

void drawScore() {
  tft.setTextColor(TFT_WHITE, SKY_COLOR);
  tft.setTextSize(3);
  tft.setCursor(10, 10);
  tft.print("Score: ");
  tft.print(score);
  lastDrawnScore = score;
}

//====================================================================================
//                                 GAME SCREENS
//====================================================================================
void showStartScreen() {
  gameState = STATE_START_SCREEN;
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_YELLOW);
  tft.setTextSize(4);
  tft.setCursor(40, 50);
  tft.print("Flappy Bird");
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.setCursor(70, 120);
  tft.print("Touch to Start");
  tft.setCursor(80, 160);
  tft.print("High Score: ");
  tft.print(highScore);
}

void showGameOverScreen() {
  gameState = STATE_GAME_OVER;
  tft.fillScreen(TFT_RED);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(4);
  tft.setCursor(50, 50);
  tft.print("GAME OVER");
  tft.setTextSize(2);
  tft.setCursor(90, 120);
  tft.print("Score: ");
  tft.print(score);
  tft.setCursor(80, 160);
  tft.print("High Score: ");
  tft.print(highScore);
  tft.setTextSize(1);
  tft.setCursor(100, 210);
  tft.print("Touch screen to restart");
}

//====================================================================================
//                                     LOOP
//====================================================================================
void loop() {
  TouchPoint p = ts.getTouch();
  if (p.zRaw > 200 && screenButton.contains(p.x, p.y)) {
    screenButton.press(true);
  } else {
    screenButton.press(false);
  }

  switch (gameState) {
    case STATE_START_SCREEN:
      if (screenButton.justPressed()) {
        setupNewGame();
        gameState = STATE_PLAYING;
      }
      break;

    case STATE_PLAYING:
      eraseBird();
      for (int i = 0; i < NUM_PIPES; i++) {
        erasePipe(i);
      }

      if (screenButton.justPressed()) {
        bird_vy = FLAP_STRENGTH;
      }
      last_bird_y = bird_y;
      bird_vy += GRAVITY;
      bird_y += bird_vy;

      for (int i = 0; i < NUM_PIPES; i++) {
        last_pipe_x[i] = pipe_x[i];
        pipe_x[i] -= PIPE_SPEED;
        if (pipe_x[i] < -PIPE_WIDTH) {
          pipe_x[i] = SCREEN_WIDTH;
          pipe_gap_y[i] = random(PIPE_GAP / 2 + 10, GROUND_Y - PIPE_GAP / 2 - 10);
          pipe_passed[i] = false;
        }
        if (!pipe_passed[i] && pipe_x[i] + PIPE_WIDTH < BIRD_X) {
          pipe_passed[i] = true;
          score++;
        }
      }
      
      drawBird();
      for (int i = 0; i < NUM_PIPES; i++) {
        drawPipe(i);
      }
      if (score != lastDrawnScore) {
        drawScore();
      }

      if (bird_y + BIRD_RADIUS > GROUND_Y || bird_y - BIRD_RADIUS < 0) {
        if (score > highScore) highScore = score;
        showGameOverScreen();
      }
      for (int i = 0; i < NUM_PIPES; i++) {
        if (BIRD_X + BIRD_RADIUS > pipe_x[i] && BIRD_X - BIRD_RADIUS < pipe_x[i] + PIPE_WIDTH) {
          if (bird_y - BIRD_RADIUS < pipe_gap_y[i] - PIPE_GAP / 2 || bird_y + BIRD_RADIUS > pipe_gap_y[i] + PIPE_GAP / 2) {
             if (score > highScore) highScore = score;
             showGameOverScreen();
          }
        }
      }
      break;

    case STATE_GAME_OVER:
      if (screenButton.justPressed()) {
        showStartScreen();
      }
      break;
  }
  delay(15);
}
