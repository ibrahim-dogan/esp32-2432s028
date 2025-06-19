#include <TFT_eSPI.h>

// Create an instance of the TFT_eSPI library
TFT_eSPI tft = TFT_eSPI();

// --- Animation Configuration ---
#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 240

#define FONT_SIZE     1 
#define CHAR_WIDTH    (6 * FONT_SIZE)
#define CHAR_HEIGHT   (8 * FONT_SIZE)
#define NUM_COLS      (SCREEN_WIDTH / CHAR_WIDTH)

// --- Animation State ---
int y_pos[NUM_COLS];
int speed[NUM_COLS];

//====================================================================================
//                                    SETUP
//====================================================================================
void setup() {
  tft.init();
  tft.setRotation(1); 
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(FONT_SIZE);

  // Initialize the streams
  for (int i = 0; i < NUM_COLS; i++) {
    y_pos[i] = random(-SCREEN_HEIGHT, 0); 
    speed[i] = random(2, 8);
  }
}


//====================================================================================
//                              GLITCH FUNCTION
//====================================================================================
void triggerGlitch(int col) {
  // This function creates a visual flash and resets a column
  
  // 1. Draw a white rectangle over the entire column to make it flash
  tft.fillRect(col * CHAR_WIDTH, 0, CHAR_WIDTH, SCREEN_HEIGHT, TFT_WHITE);
  
  // Keep the flash on screen for a brief moment
  delay(20); 
  
  // 2. "Erase" the flash by drawing a black rectangle over it
  tft.fillRect(col * CHAR_WIDTH, 0, CHAR_WIDTH, SCREEN_HEIGHT, TFT_BLACK);

  // 3. Reset this column's stream to the top with a very high speed
  y_pos[col] = 0;
  speed[col] = random(15, 25); // Make glitched streams super fast
}


//====================================================================================
//                                     LOOP
//====================================================================================
void loop() {
  
  // --- CHANCE TO GLITCH ---
  // On any given frame, there's a small chance to trigger a glitch.
  if (random(100) > 97) { // This gives a 2% chance per frame. Adjust for more/less frequent glitches.
    int random_col = random(NUM_COLS); // Pick a random column to glitch
    triggerGlitch(random_col);
  }

  // --- REGULAR ANIMATION ---
  // Loop through every column on the screen
  for (int i = 0; i < NUM_COLS; i++) {
    
    // Get a random character to draw
    char randomChar = (char)random(33, 126);

    // Set the cursor to the current column and y_pos
    tft.setCursor(i * CHAR_WIDTH, y_pos[i]);
    
    // The "leader" character is bright white or light green
    tft.setTextColor(TFT_WHITE);
    tft.print(randomChar);

    // The character right behind the leader is bright green
    int trail_y = y_pos[i] - (speed[i] * 2); 
    if (trail_y > 0) {
      tft.setCursor(i * CHAR_WIDTH, trail_y);
      tft.setTextColor(TFT_GREEN);
      tft.print(randomChar);
    }

    // Erase the end of the trail by drawing a black character
    int tail_y = y_pos[i] - (CHAR_HEIGHT * 20); 
    if (tail_y > 0) {
      tft.setCursor(i * CHAR_WIDTH, tail_y);
      tft.setTextColor(TFT_BLACK); 
      tft.print(randomChar);
    }

    // Update the stream's position
    y_pos[i] += speed[i];

    // Reset the stream if it goes off-screen
    if (tail_y > SCREEN_HEIGHT) {
      y_pos[i] = 0; 
      speed[i] = random(2, 8); // Reset to a normal speed
    }
  }
  
  // A small delay to control the animation speed.
  delay(10); 
}
