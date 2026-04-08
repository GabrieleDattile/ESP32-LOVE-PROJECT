#include "Arduino.h"
#include "TFT_eSPI.h" 
#include "pin_config.h"
#include "OneButton.h"

#define LCD_MODULE_CMD_1

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite spr = TFT_eSprite(&tft);

// =========================================================
// ⬇️ USER CONFIGURATION - CHANGE YOUR VALUES HERE ⬇️
// =========================================================

// 1. Change the initials here (1 character recommended)
#define INITIAL_LEFT  "?"
#define INITIAL_RIGHT "?"

// 2. Change your custom messages here
// Use \n to create a new line. Keep lines short to fit the screen!
String messages[] = {
    "You are my\nfavorite\nperson!",
    "Thinking\nof you\nalways!",
    "Smile,\nyou are\nbeautiful!",
    "My heart\nbeats\nonly for you",
    "You are\nthe best thing\nin my life",
    "I Love You\n[NAME]!" // <--- REPLACE [NAME] WITH YOUR PERSON'S NAME
};
int numMessages = 6;

// =========================================================
// ⬆️ END OF CONFIGURATION ⬆️
// =========================================================

OneButton button1(PIN_BUTTON_1, true);
OneButton button2(PIN_BUTTON_2, true);

enum State {
    STATE_HEART,
    STATE_BEARS,
    STATE_BUTTERFLIES,
    STATE_PACMAN,
    STATE_MESSAGE
};

State currentState = STATE_HEART;

struct Particle {
    float x, y;
    float speed;
    uint16_t color;
    bool isHeart;
};

const int numParticles = 18;
Particle particles[numParticles];

int currentMessageIndex = 0;
float animationPos = 0; 
bool actionDone = false;

#if defined(LCD_MODULE_CMD_1)
typedef struct {
    uint8_t cmd;
    uint8_t data[14];
    uint8_t len;
} lcd_cmd_t;

lcd_cmd_t lcd_st7789v[] = {
    {0x11, {0}, 0 | 0x80},
    {0x3A, {0X05}, 1},
    {0xB2, {0X0B, 0X0B, 0X00, 0X33, 0X33}, 5},
    {0xB7, {0X75}, 1},
    {0xBB, {0X28}, 1},
    {0xC0, {0X2C}, 1},
    {0xC2, {0X01}, 1},
    {0xC3, {0X1F}, 1},
    {0xC6, {0X13}, 1},
    {0xD0, {0XA7}, 1},
    {0xD0, {0XA4, 0XA1}, 2},
    {0xD6, {0XA1}, 1},
    {0xE0, {0XF0, 0X05, 0X0A, 0X06, 0X06, 0X03, 0X2B, 0X32, 0X43, 0X36, 0X11, 0X10, 0X2B, 0X32}, 14},
    {0xE1, {0XF0, 0X08, 0X0C, 0X0B, 0X09, 0X24, 0X2B, 0X22, 0X43, 0X38, 0X15, 0X16, 0X2F, 0X37}, 14},
};
#endif

void drawTinyHeart(TFT_eSprite &sprite, int x, int y, uint16_t color) {
    sprite.fillCircle(x-2, y, 2, color);
    sprite.fillCircle(x+2, y, 2, color);
    sprite.fillTriangle(x-4, y, x+4, y, x, y+5, color);
}

void drawRose(TFT_eSprite &sprite, int x, int y) {
    uint16_t roseRed = tft.color565(220, 20, 60);
    uint16_t darkRed = tft.color565(139, 0, 0);
    uint16_t leafGreen = tft.color565(34, 139, 34);
    sprite.fillEllipse(x + 5, y + 5, 4, 2, leafGreen);
    sprite.fillCircle(x, y, 4, roseRed);
    sprite.fillCircle(x+3, y-2, 3, roseRed);
    sprite.fillCircle(x-3, y-2, 3, roseRed);
    sprite.fillCircle(x, y-4, 3, roseRed);
    sprite.drawCircle(x, y-1, 1, darkRed);
}

void drawPacman(TFT_eSprite &sprite, int x, int y, float mouthOpen, bool flip, bool isMs, uint16_t color) {
    uint16_t red = tft.color565(255, 0, 0);
    
    // Body 
    sprite.fillCircle(x, y, 15, color);
    
    // Mouth (Wedge)
    int mouthSize = 10 * mouthOpen;
    int mx = flip ? x - 15 : x + 15;
    sprite.fillTriangle(x, y, mx, y - mouthSize, mx, y + mouthSize, TFT_BLACK);
    
    // Eye
    int eyeX = flip ? x - 5 : x + 5;
    sprite.fillCircle(eyeX, y - 6, 2, TFT_BLACK);
    
    if (isMs) {
        // Simple elegant eyelashes
        sprite.drawLine(eyeX, y-8, eyeX+(flip?2:-2), y-12, TFT_BLACK); 
        sprite.drawLine(eyeX+1, y-8, eyeX+(flip?3:-3), y-11, TFT_BLACK); 
    }
}

void drawButterfly(TFT_eSprite &sprite, int x, int y, uint16_t color, float wingOsc) {
    int w = 22 + (wingOsc * 16); 
    int h1 = 28, h2 = 18;
    uint16_t veinColor = tft.color565(30, 30, 30);
    uint16_t lightColor = tft.color565(255, 255, 255);
    
    // Forewings (top)
    sprite.fillTriangle(x, y-4, x-w, y-h1, x-w/2, y, color);
    sprite.fillTriangle(x, y-4, x+w, y-h1, x+w/2, y, color);
    // Hindwings (bottom)
    sprite.fillEllipse(x-w/2, y+10, w/1.2, h2, color);
    sprite.fillEllipse(x+w/2, y+10, w/1.2, h2, color);
    
    // Realistic Veins (Radial)
    for(int i=0; i<3; i++) {
        sprite.drawLine(x, y-4, x-w+(i*5), y-h1+(i*8), veinColor);
        sprite.drawLine(x, y-4, x+w-(i*5), y-h1+(i*8), veinColor);
    }
    // Highlighting
    sprite.fillCircle(x-w/2, y-10, 4, lightColor);
    sprite.fillCircle(x+w/2, y-10, 4, lightColor);

    // Segmented Body
    sprite.fillCircle(x, y-8, 3, TFT_BLACK);   // Head
    sprite.fillEllipse(x, y, 4, 10, TFT_BLACK); // Thorax
    sprite.fillEllipse(x, y+10, 3, 14, tft.color565(40,40,40)); // Abdomen
    
    // Antennae
    sprite.drawLine(x, y-8, x-12, y-24, TFT_BLACK);
    sprite.drawLine(x, y-8, x+12, y-24, TFT_BLACK);
}

void drawParametricHeart(TFT_eSprite &sprite, int cx, int cy, float scale, uint16_t color) {
    int lastX = -1, lastY = -1;
    for (float t = 0; t <= 2 * PI; t += 0.08) {
        float x = 16 * pow(sin(t), 3);
        float y = -(13 * cos(t) - 5 * cos(2 * t) - 2 * cos(3 * t) - cos(4 * t));
        int px = cx + (int)(x * scale);
        int py = cy + (int)(y * scale);
        if (lastX != -1) {
            sprite.drawLine(cx, cy, px, py, color);
            sprite.drawLine(lastX, lastY, px, py, color);
        }
        lastX = px; lastY = py;
    }
}

void drawBear(TFT_eSprite &sprite, int x, int y, bool flip, uint16_t bodyColor) {
    uint16_t lightMuzzle = tft.color565(255, 235, 205); // Cream
    uint16_t earColor = tft.color565(255, 192, 203); // Pink inner ear
    
    // Ears
    sprite.fillCircle(x - 13, y - 12, 7, bodyColor);
    sprite.fillCircle(x + 13, y - 12, 7, bodyColor);
    sprite.fillCircle(x - 13, y - 12, 3, earColor);
    sprite.fillCircle(x + 13, y - 12, 3, earColor);
    
    // Head (Chibi style: extra round)
    sprite.fillEllipse(x, y, 25, 21, bodyColor);
    
    // Muzzle
    sprite.fillEllipse(x, y + 6, 12, 8, lightMuzzle);
    
    // Eyes
    int eyeX = flip ? -9 : 9;
    sprite.fillCircle(x + eyeX, y - 2, 3, (bodyColor == TFT_BLACK) ? tft.color565(40,40,40) : TFT_BLACK);
    sprite.fillCircle(x - eyeX, y - 2, 3, (bodyColor == TFT_BLACK) ? tft.color565(40,40,40) : TFT_BLACK);
    // Highlights
    sprite.fillCircle(x + eyeX - 1, y - 3, 1, TFT_WHITE);
    sprite.fillCircle(x - eyeX - 1, y - 3, 1, TFT_WHITE);
    
    // Nose
    sprite.fillCircle(x, y + 3, 2, TFT_BLACK);
    
    // Blush
    sprite.fillCircle(x - 16, y + 6, 4, tft.color565(255, 170, 180));
    sprite.fillCircle(x + 16, y + 6, 4, tft.color565(255, 170, 180));
}

void showRandomMessage() {
    currentMessageIndex = random(0, numMessages);
    currentState = STATE_MESSAGE;
    uint16_t bgColor = tft.color565(255, 240, 245);
    uint16_t accentColor = tft.color565(219, 112, 147);
    tft.fillScreen(bgColor);
    tft.drawRoundRect(10, 10, 300, 150, 15, accentColor);
    tft.drawRoundRect(12, 12, 296, 146, 13, accentColor);
    tft.setTextColor(TFT_BLACK, bgColor);
    tft.setTextSize(3);
    int startY = 45;
    if (messages[currentMessageIndex].indexOf('\n') == -1) startY = 65;
    char buf[100];
    messages[currentMessageIndex].toCharArray(buf, 100);
    char * line = strtok(buf, "\n");
    while(line != NULL) {
        int textWidth = strlen(line) * 18;
        tft.setCursor(160 - (textWidth/2), startY);
        tft.println(line);
        startY += 35;
        line = strtok(NULL, "\n");
    }
}

void onButtonClick() {
    animationPos = 0; 
    actionDone = false;
    tft.fillScreen(TFT_BLACK);
    
    if (currentState == STATE_HEART) {
        // Step 1 -> Step 2: Random animation
        int r = random(0, 3); 
        if (r == 0) currentState = STATE_BEARS;
        else if (r == 1) currentState = STATE_BUTTERFLIES;
        else currentState = STATE_PACMAN; 
    } 
    else if (currentState == STATE_BEARS || currentState == STATE_BUTTERFLIES || currentState == STATE_PACMAN) {
        // Step 2 -> Step 3: Show a random message
        showRandomMessage();
    } 
    else {
        // Step 3 -> Step 1: Return to the heart
        currentState = STATE_HEART;
    }
}

void setup() {
    pinMode(PIN_POWER_ON, OUTPUT);
    digitalWrite(PIN_POWER_ON, HIGH);
    Serial.begin(115200);
    delay(500);
    pinMode(PIN_LCD_BL, OUTPUT);
    digitalWrite(PIN_LCD_BL, HIGH);
    tft.begin();
#if defined(LCD_MODULE_CMD_1)
    for (uint8_t i = 0; i < (sizeof(lcd_st7789v) / sizeof(lcd_cmd_t)); i++) {
        tft.writecommand(lcd_st7789v[i].cmd);
        for (int j = 0; j < (lcd_st7789v[i].len & 0x7f); j++) tft.writedata(lcd_st7789v[i].data[j]);
        if (lcd_st7789v[i].len & 0x80) delay(120);
    }
#endif
    tft.setRotation(3);
    tft.setSwapBytes(true);
    tft.fillScreen(TFT_BLACK);
    spr.createSprite(280, 160);
    spr.setSwapBytes(true);
    button1.attachClick(onButtonClick);
    button2.attachClick(onButtonClick);
    for(int i=0; i<numParticles; i++) {
        particles[i].x = random(0, 280);
        particles[i].y = random(0, 160);
        particles[i].speed = (float)random(3, 12) / 10.0;
        particles[i].color = tft.color565(random(180, 255), random(50, 150), random(100, 200));
        particles[i].isHeart = (random(0,3) == 0);
    }
    randomSeed(analogRead(0) + analogRead(4));
}

void loop() {
    button1.tick();
    button2.tick();

    if (currentState != STATE_MESSAGE) {
        spr.fillSprite(TFT_BLACK);
        // Particles
        for(int i=0; i<numParticles; i++) {
            particles[i].y -= particles[i].speed;
            if (particles[i].y < -5) { particles[i].y = 165; particles[i].x = random(0, 280); }
            if (particles[i].isHeart) drawTinyHeart(spr, particles[i].x, particles[i].y, particles[i].color);
            else spr.fillCircle(particles[i].x, particles[i].y, 1, particles[i].color);
        }

        if (currentState == STATE_HEART) {
            float pulse = sin(millis() / 400.0);
            float baseScale = 3.6 + (pulse * 0.3);
            drawParametricHeart(spr, 140, 80, baseScale + 0.8, tft.color565(100, 0, 20));
            drawParametricHeart(spr, 140, 80, baseScale, TFT_RED);
            
            // Render Configurable Initials
            uint16_t colorLeft = tft.color565(0, 191, 255);   // Blue
            uint16_t colorRight = tft.color565(255, 105, 180); // Pink
            
            spr.setTextSize(4);
            
            // Shadows for both (offset by 2px)
            spr.setTextColor(tft.color565(40, 0, 10));
            spr.setCursor(140 - 36, 80 - 13); spr.print(INITIAL_LEFT);
            spr.setCursor(140 + 18, 80 - 13); spr.print(INITIAL_RIGHT);
            drawTinyHeart(spr, 140 + 2, 80 + 2, tft.color565(60, 0, 10));
            
            // Color initials
            spr.setCursor(140 - 38, 80 - 15);
            spr.setTextColor(colorLeft); spr.print(INITIAL_LEFT);
            spr.setCursor(140 + 16, 80 - 15);
            spr.setTextColor(colorRight); spr.print(INITIAL_RIGHT);
            
            // Central Red Heart
            drawTinyHeart(spr, 140, 80, TFT_RED);
            drawTinyHeart(spr, 140, 80, tft.color565(255, 50, 50)); 
        } else if (currentState == STATE_BEARS) {
            if (animationPos < 1.0) animationPos += 0.01; else actionDone = true;
            int x1 = -40 + (animationPos * 155);
            int x2 = 320 - (animationPos * 155);
            float hop = actionDone ? abs(sin(millis()/150.0)*8.0) : 0;
            drawRose(spr, 30, 140); drawRose(spr, 250, 140);
            if (actionDone) { drawRose(spr, 140, 130); drawParametricHeart(spr, 140, 60, 0.9 + (sin(millis()/200.0)*0.5), TFT_RED); }
            drawBear(spr, x1, 100 - hop, false, tft.color565(20,20,20)); 
            drawBear(spr, x2, 100 - hop, true, tft.color565(139,69,19));
        } else if (currentState == STATE_BUTTERFLIES) {
            float flap = sin(millis() / 120.0);
            float floatY = sin(millis() / 250.0) * 15.0;
            drawButterfly(spr, 80 + (sin(millis()/400.0)*50.0), 80 + floatY, tft.color565(30,144,255), flap);
            drawButterfly(spr, 200 + (cos(millis()/500.0)*40.0), 60 - floatY, tft.color565(255,20,147), -flap);
        } else if (currentState == STATE_PACMAN) {
            if (animationPos < 1.0) animationPos += 0.007; else actionDone = true;
            int px1 = -30 + (animationPos * 160);
            int px2 = 310 - (animationPos * 160);
            float wave = sin(animationPos * PI * 2) * 20.0; // Wavy path
            int py = 100 + (int)wave;
            float mouth = abs(sin(millis()/100.0));
            
            // Pac-Man colors
            uint16_t colorBlue = tft.color565(30, 144, 255);
            uint16_t colorPink = tft.color565(255, 105, 180);
            
            // Draw hearts (pills) along the wave
            for(int i=0; i<8; i++) {
                float hPos = (float)i / 8.0;
                int hx = 20 + (hPos * 240);
                int hy = 100 + sin(hPos * PI * 2) * 20.0;
                if ((!actionDone) && (hx > px1 + 15 && hx < px2 - 15)) drawTinyHeart(spr, hx, hy, tft.color565(255, 50, 80));
            }
            
            drawPacman(spr, px1, py, mouth, false, false, colorBlue);
            drawPacman(spr, px2, py, mouth, true, true, colorPink);
            if (actionDone) { drawParametricHeart(spr, 140, 85, 1.0 + (sin(millis()/200.0)*0.4), TFT_RED); }
        }
        spr.pushSprite(20, 5);
        delay(15);
    }
}
