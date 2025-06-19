#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Bitbang.h> // The correct touch library for your hardware

// --- TOUCH SCREEN PINS (for ESP32 Cheap Yellow Display) ---
#define XPT2046_IRQ  36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK  25
#define XPT2046_CS   33

// --- LIBRARY INSTANCES ---
TFT_eSPI tft = TFT_eSPI();
XPT2046_Bitbang ts(XPT2046_MOSI, XPT2046_MISO, XPT2046_CLK, XPT2046_CS);

// --- NEW: A SINGLE, INVISIBLE BUTTON TO CAPTURE ALL TOUCHES ---
TFT_eSPI_Button screenButton;

// --- SCREEN & LAYOUT ---
#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 240
#define GROUND_HEIGHT 20
#define SKY_COLOR     0x64B9 // A nice light blue
#define GROUND_COLOR  0x5A40 // A dirt/grass brown

// --- GAME STATE ---
enum GameState {
  STATE_START_SCREEN,
  STATE_PLAYING,
  STATE_GAME_OVER
};
GameState gameState = STATE_START_SCREEN;

// --- BIRD PROPERTIES ---
#define BIRD_X        80
#define BIRD_RADIUS   10
float bird_y;
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
bool pipe_passed[NUM_PIPES];

// --- SCORE ---
int score = 0;
int highScore = 0;

//====================================================================================
//                                   SETUP
//====================================================================================
void setup() {
  Serial.begin(115200);
  
  tft.init();
  tft.setRotation(1);

  ts.begin();
  // IMPORTANT: Do NOT call ts.setRotation(). Let the button class handle coordinates.

  // --- NEW: Initialize our screen-sized button ---
  // We define it but we will never draw it, making it invisible.
  screenButton.initButton(
    &tft, tft.width() / 2, tft.height() / 2, // x, y (center)
    tft.width(), tft.height(),               // width, height
    TFT_BLACK, TFT_BLACK, TFT_BLACK,         // Colors (outline, fill, text)
    "", 0                                    // Label, text size
  );

  randomSeed(analogRead(A0));
  showStartScreen();
}


//====================================================================================
//                                RESET GAME
//====================================================================================
void resetGame() {
  bird_y = SCREEN_HEIGHT / 2.0;
  bird_vy = 0;
  for (int i = 0; i < NUM_PIPES; i++) {
    pipe_x[i] = SCREEN_WIDTH + i * (SCREEN_WIDTH / 2 + PIPE_WIDTH / 2);
    pipe_gap_y[i] = random(PIPE_GAP, SCREEN_HEIGHT - GROUND_HEIGHT - PIPE_GAP);
    pipe_passed[i] = false;
  }
  score = 0;
  tft.fillScreen(SKY_COLOR);
  drawGround();
}

//====================================================================================
//                             DRAWING FUNCTIONS (Unchanged)
//====================================================================================
void drawGround() {
  tft.fillRect(0, SCREEN_HEIGHT - GROUND_HEIGHT, SCREEN_WIDTH, GROUND_HEIGHT, GROUND_COLOR);
  tft.fillRect(0, SCREEN_HEIGHT - GROUND_HEIGHT, SCREEN_WIDTH, 4, TFT_GREEN);
}
void drawBird() {
  tft.fillCircle(BIRD_X, (int)bird_y, BIRD_RADIUS, TFT_YELLOW);
  tft.fillCircle(BIRD_X + 4, (int)bird_y - 2, 3, TFT_WHITE);
  tft.fillCircle(BIRD_X + 5, (int)bird_y - 2, 1, TFT_BLACK);
}
void drawPipes() {
  for (int i = 0; i < NUM_PIPES; i++) {
    tft.fillRect(pipe_x[i], 0, PIPE_WIDTH, pipe_gap_y[i] - PIPE_GAP / 2, TFT_GREEN);
    tft.fillRect(pipe_x[i]-2, pipe_gap_y[i] - PIPE_GAP / 2 - 20, PIPE_WIDTH+4, 20, TFT_DARKGREEN);
    tft.fillRect(pipe_x[i], pipe_gap_y[i] + PIPE_GAP / 2, PIPE_WIDTH, SCREEN_HEIGHT, TFT_GREEN);
    tft.fillRect(pipe_x[i]-2, pipe_gap_y[i] + PIPE_GAP / 2, PIPE_WIDTH+4, 20, TFT_DARKGREEN);
  }
}
void drawScore() {
  tft.setTextSize(3);
  tft.setTextColor(TFT_WHITE, SKY_COLOR);
  tft.setCursor(10, 10);
  tft.print("Score: ");
  tft.print(score);
}

//====================================================================================
//                                 GAME SCREENS (Unchanged)
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
  // --- NEW: Read touch and update the state of our invisible button ---
  TouchPoint p = ts.getTouch();

  // Check if the screen is being touched within the button's bounds
  if (p.zRaw > 200 && screenButton.contains(p.x, p.y)) {
    screenButton.press(true); // Tell the button it is pressed
  } else {
    screenButton.press(false); // Tell the button it is NOT pressed
  }

  // --- Game State Machine ---
  switch (gameState) {
    case STATE_START_SCREEN:
      // Use the button's built-in state detection
      if (screenButton.justPressed()) {
        resetGame();
        gameState = STATE_PLAYING;
      }
      break;

    case STATE_PLAYING:
      // Flap on a new press
      if (screenButton.justPressed()) {
        bird_vy = FLAP_STRENGTH;
      }

      // Physics
      bird_vy += GRAVITY;
      bird_y += bird_vy;

      // Move pipes and check for score
      for (int i = 0; i < NUM_PIPES; i++) {
        pipe_x[i] -= PIPE_SPEED;
        if (pipe_x[i] < -PIPE_WIDTH) {
          pipe_x[i] = SCREEN_WIDTH;
          pipe_gap_y[i] = random(PIPE_GAP, SCREEN_HEIGHT - GROUND_HEIGHT - PIPE_GAP);
          pipe_passed[i] = false;
        }
        if (!pipe_passed[i] && pipe_x[i] + PIPE_WIDTH < BIRD_X) {
          pipe_passed[i] = true;
          score++;
        }
      }

      // Collision Detection
      if (bird_y + BIRD_RADIUS > SCREEN_HEIGHT - GROUND_HEIGHT || bird_y - BIRD_RADIUS < 0) {
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
      
      // Drawing
      tft.fillScreen(SKY_COLOR);
      drawPipes();
      drawGround();
      drawBird();
      drawScore();
      break;

    case STATE_GAME_OVER:
      if (screenButton.justPressed()) {
        showStartScreen();
      }
      break;
  }

  delay(10);
}
