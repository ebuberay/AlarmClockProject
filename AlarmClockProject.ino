#include <TFT_eSPI.h>
#include <Wire.h>
#include <RTClib.h>
#include <WiFi.h>
#include <SPI.h>
#include <time.h>


// ================= Constants =================
#define MENU_BTN_X 281
#define MENU_BTN_Y 199
#define WIFI_ICON_X 7
#define WIFI_ICON_Y 214
#define TFT_GREY 0x5AEB
// Define icon dimensions
#define ICON_WIDTH 16
#define ICON_HEIGHT 16
// Define offsets for the clearing rectangle to ensure complete coverage.
// This will make the clear rectangle start 2 pixels left and 2 pixels up from the icon's top-left,
// and be 4 pixels wider and 4 pixels taller than the icon itself (16 + 2 + 2 = 20).
#define CLEAR_OFFSET_X 2
#define CLEAR_OFFSET_Y 2
#define CLEAR_RECT_WIDTH (ICON_WIDTH + CLEAR_OFFSET_X * 2)
#define CLEAR_RECT_HEIGHT (ICON_HEIGHT + CLEAR_OFFSET_Y * 2)
const int CENTER_X = 160;
const int CENTER_Y = 120;
float sx = 0, sy = 1, mx = 1, my = 0, hx = -1, hy = 0;
float sdeg = 0, mdeg = 0, hdeg = 0;
uint16_t osx = CENTER_X, osy = CENTER_Y;
uint16_t omx = CENTER_X, omy = CENTER_Y;
uint16_t ohx = CENTER_X, ohy = CENTER_Y;
uint32_t analogTargetTime = 0;
bool analogInitial = true;
enum Page { PAGE_HOME, PAGE_MENU, PAGE_ANALOG, PAGE_ALARM, PAGE_TIMER, PAGE_ANIM, PAGE_WIFI, PAGE_ALARM_RINGING, PAGE_TIMER_RINGING, PAGE_BRIGHTNESS };

Page currentPage = PAGE_HOME;

// Add these global variables after the 'Page currentPage' declaration:
int horizX = 40;
int horizY = 160;
int horizW = 240;
int horizH = 20;
int horizValue = 50;
int oldKnobX = -1;
int backlightPin = 33;
int freq = 5000;
int resolution = 8;
const int MIN_BRIGHTNESS = 20;
const int MAX_BRIGHTNESS = 255;
int lastIconLevel = -1;

// Alarm variables
int alarmHour = 6;
int alarmMinute = 30;
bool alarmEnabled = false;
bool alarmTriggered = false;
// Timer variables
int timerHours = 0;
int timerMinutes = 0;
int timerSeconds = 0;
bool timerRunning = false;
bool timerTriggered = false;
unsigned long timerStartTime = 0;
unsigned long initialTimerTotalSeconds = 0;
#define BUZZER_PIN 12
// ================= TFT and RTC =================
TFT_eSPI tft = TFT_eSPI();
RTC_DS3231 rtc;
uint16_t calData[5] = { 473, 3410, 383, 3349, 7 }; // touch calibration
DateTime now;
// ================= Long Press & Touch State Variables =================
unsigned long lastPressMillis = 0;
int longPressButton = 0;
// 0=no press, 1=alarm +H, etc.
long longPressInterval = 500; // Initial delay for long press
bool isPressing = false;
// A helper enum to make the long press button tracking more readable
enum LongPressButtons {
    NONE,
    ALARM_H_PLUS, ALARM_H_MINUS, ALARM_M_PLUS, ALARM_M_MINUS,
    TIMER_H_PLUS, TIMER_H_MINUS, TIMER_M_PLUS, TIMER_M_MINUS, TIMER_S_PLUS, TIMER_S_MINUS
};
// ================= WIFI State & Variables =================
enum WifiScreenState { WIFI_HOME, SCAN, ENTER_PASSWORD, CONNECTED };
WifiScreenState currentWifiScreen = WIFI_HOME;
String typedText = "";
bool Caps = false;
bool alphaMode = true;
String selectedSSID = "";
String keysAlpha[3][10] = {
    {"Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P"},
    {"A", "S", "D", "F", "G", "H", "J", "K", "L", "#"},
    {"Caps", "Z", "X", "C", "V", "B", "N", "M", ".", "BS"}
};
String keysNum[3][10] = {
    {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"},
    {"_", "/", ":", ";", "(", ")", "$", "&", "@", "'"},
    {"!", "?", "+", "-", "*", "=", "~", "{", "}", "BS"}
};
String ssidList[20];
int ssidCount = 0;
// Variable to track the previous WiFi connection status
bool lastWifiConnectedStatus = false;
// ================= Bitmap Icons =================
#include "menu_icon.h"       // menuButton
#include "no_con_icon.h"     // NO_CON
#include "wifi_icon.h"       // wifiStatus
#include "analog_icon.h"     // analogClock
#include "alarm_icon.h"      // clockAlarm
#include "digital_icon.h"    // clockDigital
#include "timer_icon.h"      // timerButton
#include "anim_icon.h"       // animButton
#include "wifi_button_icon.h"// wifiButton
#include "blank_icon.h"
#include "bright_High.h"
#include "bright_Mid.h"
#include "bright_Low.h"


static const unsigned char PROGMEM BELL_ICON[] = {

  0x00,0x70,0x07,0xe0,0x0e,0x00,0x00,0x70,0x07,0xe0,0x0e,0x00,0x00,0x70,0x07,0xe0,
  0x0e,0x00,0x1f,0x80,0x3f,0xfc,0x01,0xf8,0x1f,0x80,0x3f,0xfc,0x01,0xf8,0x1f,0x80,
  0x3f,0xfc,0x01,0xf8,0x1c,0x01,0xc0,0x03,0x80,0x38,0x1c,0x01,0xc0,0x03,0x80,0x38,
  0x1c,0x01,0xc0,0x03,0x80,0x38,0xe0,0x0f,0xc0,0x00,0x70,0x07,0xe0,0x0f,0xc0,0x00,
  0x70,0x07,0xe0,0x0f,0xc0,0x00,0x70,0x07,0xe0,0x0e,0x00,0x00,0x70,0x07,0xe0,0x0e,
  0x00,0x00,0x70,0x07,0xe0,0x0e,0x00,0x00,0x70,0x07,0x00,0x70,0x00,0x00,0x0e,0x00,
  0x00,0x70,0x00,0x00,0x0e,0x00,0x00,0x70,0x00,0x00,0x0e,0x00,0x00,0x70,0x00,0x00,
  0x0e,0x00,0x00,0x70,0x00,0x00,0x0e,0x00,0x00,0x70,0x00,0x00,0x0e,0x00,0x00,0x70,
  0x00,0x00,0x0e,0x00,0x00,0x70,0x00,0x00,0x0e,0x00,0x00,0x70,0x00,0x00,0x0e,0x00,
  0x00,0x70,0x00,0x00,0x0e,0x00,0x03,0x80,0x00,0x00,0x01,0xc0,0x03,0x80,0x00,0x00,
  0x01,0xc0,0x03,0x80,0x00,0x00,0x01,0xc0,0x03,0x80,0x00,0x00,0x01,0xc0,0x03,0x80,
  0x00,0x00,0x01,0xc0,0x1c,0x00,0x00,0x00,0x00,0x38,0x1c,0x00,0x00,0x00,0x00,0x38,
  0x1c,0x00,0x00,0x00,0x00,0x38,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x01,0xf8,0x1f,0x80,0x00,0x00,0x01,
  0xf8,0x1f,0x80,0x00,0x00,0x01,0xf8,0x1f,0x80,0x00,0x00,0x00,0x3f,0xfc,0x00,0x00,
  0x00,0x00,0x3f,0xfc,0x00,0x00,0x00,0x00,0x3f,0xfc,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00

};
// ================= Utility =================
const char* daysOfWeek[] = {"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};
const char* monthsOfYear[] = {"January","February","March","April","May","June",
                              "July","August","September","October","November","December"};
String getDaySuffix(int d) {
  if (d >= 11 && d <= 13) return "th";
  switch (d % 10) {
    case 1: return "st"; case 2: return "nd"; case 3: return "rd";
    default: return "th";
  }
}
// ================= Page Functions =================
void updateTimerDisplay() {
  String timerStr = (timerHours < 10 ? "0" : "") + String(timerHours) + ":" +
                    (timerMinutes < 10 ? "0" : "") + String(timerMinutes) + ":" +
                    (timerSeconds < 10 ? "0" : "") + String(timerSeconds);
  tft.fillRect(20, 80, 200, 50, TFT_BLACK); // Clear old timer text at new position
  tft.drawString(timerStr, 20, 80, 6);
  // Draw new timer text at new position
}
void updateAlarmDisplay() {
  String timeStr = (alarmHour < 10 ? "0" : "") + String(alarmHour) + ":" + (alarmMinute < 10 ? "0" : "") + String(alarmMinute);
  tft.fillRect(40, 80, 150, 50, TFT_BLACK); // Clear old alarm text
  tft.drawString(timeStr, 40, 80, 6);
  tft.fillRect(20, 150, 200, 30, TFT_BLACK);
  // Clear old ON/OFF text

 

    // Redraw only the toggle button area
    tft.fillRoundRect(120, 190, 80, 30, 5, alarmEnabled ? TFT_RED : TFT_GREEN);
    tft.drawString(alarmEnabled ? "Turn OFF" : "Turn ON", 125, 195, 2);


  tft.drawString("Alarm is " + String(alarmEnabled ? "ON" : "OFF"), 20, 150, 4);
}



// Function to draw the WiFi status icon, clearing the area first
void drawWifiStatus(bool connected) {
  // Draw the appropriate icon based on connection status
  if (connected) {
    tft.drawBitmap(WIFI_ICON_X, WIFI_ICON_Y, blank_icon, ICON_WIDTH, ICON_HEIGHT, TFT_WHITE);
    tft.drawBitmap(8, 215, wifiStatus, 19, 16, 0xFFFF);
  } else {
    tft.drawBitmap(WIFI_ICON_X, WIFI_ICON_Y, blank_icon, ICON_WIDTH, ICON_HEIGHT, TFT_WHITE);
    tft.drawBitmap(WIFI_ICON_X, WIFI_ICON_Y, NO_CON, ICON_WIDTH, ICON_HEIGHT, TFT_WHITE);
  }
}
void drawMenuButton() {
  tft.drawBitmap(MENU_BTN_X, MENU_BTN_Y, menuButton, 30, 32, TFT_WHITE);
}
void showHomePage() {
  currentPage = PAGE_HOME;
  tft.setTextDatum(TL_DATUM);
  tft.fillScreen(TFT_BLACK); // Full screen clear
  now = rtc.now();
  byte hour = now.hour();
  byte minute = now.minute();
  String timeStr = (hour < 10 ? "0" : "") + String(hour) + ":" +
                   (minute < 10 ? "0" : "") + String(minute);
  int xTime = (tft.width() - tft.textWidth(timeStr, 8)) / 2;
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString(timeStr, xTime, 47, 8);
  String dateStr = String(now.day()) + getDaySuffix(now.day()) + " " +
                   monthsOfYear[now.month() - 1] + " " + String(now.year());
  int xDate = (tft.width() - tft.textWidth(dateStr, 4)) / 2;
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString(dateStr, xDate, 150, 4);
  String dayStr = daysOfWeek[now.dayOfTheWeek()];
  int xDay = (tft.width() - tft.textWidth(dayStr, 4)) / 2;
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString(dayStr, xDay, 200, 4);
  // Draw WiFi status icon based on current connection.
  // This will now be called after a full screen clear.
  drawWifiStatus(WiFi.status() == WL_CONNECTED); 
  drawMenuButton();
}
void drawMenuPage() {
  currentPage = PAGE_MENU;
  tft.setTextDatum(TL_DATUM);
  tft.setTextSize(1);
  tft.fillScreen(TFT_BLACK);
  // Row 1 icons
  tft.drawBitmap(24, 46, clockDigital, 54, 54, TFT_WHITE);
  // Col 1
  tft.drawBitmap(110, 41, analogClock, 60, 64, TFT_WHITE);   // Col 2
  tft.drawBitmap(205, 43, clockAlarm, 60, 64, TFT_WHITE);
  // Col 3
  // Row 2 icons
  tft.drawBitmap(22, 129, timerButton, 60, 64, TFT_WHITE);
  // Col 1
  tft.drawBitmap(112, 141, wifiButton, 57, 48, TFT_WHITE);   // Col 2
  tft.drawBitmap(217, 136, animButton, 51, 48, TFT_WHITE);
  // Col 3
  // Back button (top-right corner)
  tft.fillRoundRect(270, 5, 50, 25, 5, TFT_RED);
  tft.setTextColor(TFT_WHITE);
  tft.drawString("Back", 275, 10, 2);
}
// Placeholder screens for other features
void showPlaceholder(String label) {
  currentPage = PAGE_ANIM;
  // Set a page type for placeholder
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_YELLOW);
  tft.drawString(label, 80, 100, 4);
  tft.fillRoundRect(270, 5, 50, 25, 5, TFT_RED);
  tft.setTextColor(TFT_WHITE);
  tft.drawString("Back", 275, 10, 2);
}
void showAnalogClockPage() {
  currentPage = PAGE_ANALOG;
  analogInitial = true;
  tft.fillScreen(TFT_GREY);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  // Draw clock border and face
  tft.fillCircle(CENTER_X, CENTER_Y, 118, TFT_GREEN);   // outer ring
  tft.fillCircle(CENTER_X, CENTER_Y, 110, TFT_BLACK);
  // face
  // Hour ticks
  for (int i = 0; i < 360; i += 30) {
    float angle = (i - 90) * DEG_TO_RAD;
    int x0 = cos(angle) * 100 + CENTER_X;
    int y0 = sin(angle) * 100 + CENTER_Y;
    int x1 = cos(angle) * 110 + CENTER_X;
    int y1 = sin(angle) * 110 + CENTER_Y;
    tft.drawLine(x0, y0, x1, y1, TFT_GREEN);
  }
  // Hour numbers
  tft.setTextDatum(MC_DATUM);
  for (int i = 1; i <= 12; i++) {
    float angle = (i * 30 - 90) * DEG_TO_RAD;
    int x = cos(angle) * 80 + CENTER_X;
    int y = sin(angle) * 80 + CENTER_Y;
    tft.drawString(String(i), x, y, 2);
  }
  // Minute dots
  for (int i = 0; i < 360; i += 6) {
    if (i % 30 == 0) continue;
    float angle = (i - 90) * DEG_TO_RAD;
    int x = cos(angle) * 108 + CENTER_X;
    int y = sin(angle) * 108 + CENTER_Y;
    tft.drawPixel(x, y, TFT_WHITE);
  }
  // Center dot
  tft.fillCircle(CENTER_X, CENTER_Y + 1, 3, TFT_WHITE);
  tft.setTextDatum(TL_DATUM);
  // Reset to default after using MC_DATUM
  // Back button
  tft.fillRoundRect(270, 5, 50, 25, 5, TFT_GREY);
  tft.setTextColor(TFT_WHITE);
  tft.drawString("Back", 275, 10, 2);
}
void showAlarmPage() {
  currentPage = PAGE_ALARM;
  tft.setTextDatum(TL_DATUM);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  
  tft.drawString("Alarm Time:", 20, 40, 4);
  updateAlarmDisplay();
  // Buttons
  tft.fillRoundRect(220, 60, 40, 30, 5, TFT_BLUE);      // +Hour
  tft.drawString("+H", 225, 65, 2);
  tft.fillRoundRect(270, 60, 40, 30, 5, TFT_BLUE);      // +Min
  tft.drawString("+M", 275, 65, 2);
  
  tft.fillRoundRect(220, 100, 40, 30, 5, TFT_ORANGE);
  // -Hour
  tft.drawString("-H", 225, 105, 2);
  tft.fillRoundRect(270, 100, 40, 30, 5, TFT_ORANGE); // -Min
  tft.drawString("-M", 275, 105, 2);
  tft.fillRoundRect(120, 190, 80, 30, 5, TFT_GREEN);  // Toggle Alarm
 tft.drawString(alarmEnabled ? "Turn OFF" : "Turn ON", 125, 195, 2);
  tft.fillRoundRect(220, 190, 80, 30, 5, TFT_GREY);//CLEAR button
  tft.drawString("Clear", 225, 195, 2);           
  tft.fillRoundRect(270, 5, 50, 25, 5, TFT_RED);
  // Back
  tft.drawString("Back", 275, 10, 2);
}
void showAlarmRingingPage() {
  currentPage = PAGE_ALARM_RINGING;
     tft.init(); // Reinitialize TFT driver (resets all settings)
    tft.setRotation(1); // Set the rotation to the standard rotation for your project
    tft.setTextSize(1); // Set the default text size
 
  tft.setTextDatum(TL_DATUM);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.drawString(" ALARM RINGING!", 60, 55, 4);
  tft.drawBitmap(138, 100, BELL_ICON, 48, 48, TFT_WHITE);
  tft.fillRoundRect(100, 160, 140, 40, 8, TFT_GREEN);
  tft.drawString("DISMISS ALARM", 120, 170, 2);
}
void showTimerPage() {
    currentPage = PAGE_TIMER;
    tft.setTextDatum(TL_DATUM);
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("Timer:", 20, 40, 4);
    // Buttons for setting timer with corrected layout
    tft.fillRoundRect(220, 60, 40, 30, 5, TFT_BLUE);
    // +Hour
    tft.drawString("+H", 225, 65, 2);
    tft.fillRoundRect(270, 60, 40, 30, 5, TFT_ORANGE);
    // -Hour
    tft.drawString("-H", 275, 65, 2);
    tft.fillRoundRect(220, 100, 40, 30, 5, TFT_BLUE);
    // +Min
    tft.drawString("+M", 225, 105, 2);
    tft.fillRoundRect(270, 100, 40, 30, 5, TFT_ORANGE);
    // -Min
    tft.drawString("-M", 275, 105, 2);
    
    tft.fillRoundRect(220, 140, 40, 30, 5, TFT_BLUE);
    // +Sec
    tft.drawString("+S", 225, 145, 2);
    tft.fillRoundRect(270, 140, 40, 30, 5, TFT_ORANGE);
    // -Sec
    tft.drawString("-S", 275, 145, 2);
    // Start/Stop and Clear Buttons
    tft.fillRoundRect(120, 190, 80, 30, 5, timerRunning ? TFT_YELLOW : TFT_GREEN);
    tft.drawString(timerRunning ? "Stop" : "Start", 125, 195, 2);
    tft.fillRoundRect(220, 190, 80, 30, 5, TFT_GREY);
    tft.drawString("Clear", 225, 195, 2);
    // Back button
    tft.fillRoundRect(270, 5, 50, 25, 5, TFT_RED);
    tft.drawString("Back", 275, 10, 2);
    
    updateTimerDisplay();
    // Initial display of the timer time
}
void showTimerRingingPage() {
    currentPage = PAGE_TIMER_RINGING;
    tft.init(); // Reinitialize TFT driver (resets all settings)
    tft.setRotation(1); // Set the rotation to the standard rotation for your project
    tft.setTextSize(1); // Set the default text size


    tft.setTextDatum(TL_DATUM);
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.drawString(" TIMER DONE!", 65, 55, 4);
    tft.drawBitmap(138, 100, BELL_ICON, 48, 48, TFT_WHITE);
    tft.fillRoundRect(100, 160, 140, 40, 8, TFT_GREEN);
    tft.drawString("DISMISS TIMER", 120, 170, 2);
}

// ===== DRAW SLIDER =====
void drawTrack() {
  tft.fillRoundRect(horizX, horizY, horizW, horizH, horizH / 2, TFT_DARKGREY);
}

void drawKnob(int value) {
  if (oldKnobX != -1) {
    tft.fillCircle(oldKnobX, horizY + horizH / 2, horizH / 2, TFT_DARKGREY);
  }
  int knobX = map(value, 0, 100, horizX + horizH / 2, horizX + horizW - horizH / 2);
  tft.fillCircle(knobX, horizY + horizH / 2, horizH / 2, TFT_BLUE);
  oldKnobX = knobX;
}

// ===== UPDATE BRIGHTNESS =====
#define BG_COLOR TFT_DARKGREY  // Change this to your slider page background color

void updateBrightness(int sliderValue) {
  int brightness = map(sliderValue, 0, 100, MIN_BRIGHTNESS, MAX_BRIGHTNESS);
  ledcWrite(backlightPin, brightness);

  int iconLevel;
  if (brightness <= 85) {
    iconLevel = 0; // Low
  }
  else if (brightness <= 145) {
    iconLevel = 1; // Mid
  }
  else {
    iconLevel = 2; // High
  }

  // Only redraw when icon actually changes
  if (iconLevel != lastIconLevel) {
    // Clear the old icon area with the background color
    tft.fillRect(142, 60, 60, 64, TFT_BLACK);

    // Redraw new icon
    if (iconLevel == 0) {
      tft.drawBitmap(142, 60, brightLow, 48, 48, TFT_WHITE);
    }
    else if (iconLevel == 1) {
      tft.drawBitmap(142, 60, brightMid, 45, 48, TFT_WHITE);
    }
    else {
      tft.drawBitmap(142, 60, brightHigh, 60, 64, TFT_WHITE);
    }

    lastIconLevel = iconLevel;
  }
}

void showBrightnessPage() {
    currentPage = PAGE_BRIGHTNESS;
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(60, 30);
    tft.print("Screen Brightness");

    // Back button on top right corner
    tft.fillRoundRect(270, 5, 50, 25, 5, TFT_RED);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("Back", 275, 10, 1);

    drawTrack();
    drawKnob(horizValue);
    updateBrightness(horizValue);

    uint16_t tx, ty;
    while (true) {
        checkAlarmTimerAndBuzzer();
        if (currentPage == PAGE_ALARM_RINGING || currentPage == PAGE_TIMER_RINGING) return;
        if (tft.getTouch(&tx, &ty)) {
            // Check for back button press
            if (tx > 270 && ty > 5 && tx < (270 + 50) && ty < (5 + 25)) {
                showAnimationListPage();
                return;
            }
            // Check for slider touch
            if (tx > horizX && tx < (horizX + horizW) && ty > horizY && ty < (horizY + horizH)) {
                // Map touch x position to slider value (0-100)
                int newHorizValue = map(tx, horizX, horizX + horizW, 0, 100);
                if (newHorizValue != horizValue) {
                    horizValue = newHorizValue;
                    drawKnob(horizValue);
                    updateBrightness(horizValue);
                }
            }
        }
        delay(20);
    }
}



// ================= WIFI Page Functions =================
void drawWifiMainPage() {
  currentPage = PAGE_WIFI;
  currentWifiScreen = WIFI_HOME;
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(3);
  tft.setTextColor(TFT_WHITE);
  tft.fillRoundRect(60, 80, 200, 50, 8, TFT_GREEN);
  tft.setCursor(100, 95); tft.print("WIFI ON");
  tft.fillRoundRect(60, 160, 200, 50, 8, TFT_RED);
  tft.setCursor(95, 175); tft.print("WIFI OFF");
  // Back button
  tft.fillRoundRect(270, 5, 50, 25, 5, TFT_RED);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.drawString("Back", 275, 10, 1);
}
void drawSSIDList() {
  currentPage = PAGE_WIFI;
  currentWifiScreen = SCAN;
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(10, 5); tft.print("Scanning Wi-Fi...");
  WiFi.disconnect(true);
  // Clear any previous connections
  WiFi.mode(WIFI_STA);      // Ensure station mode is active
  delay(100);
  ssidCount = WiFi.scanNetworks();
  tft.fillRect(0, 20, 320, 240, TFT_BLACK);
  if (ssidCount == 0) {
    tft.setCursor(10, 50); tft.print("No networks found.");
  } else {
    for (int i = 0; i < ssidCount && i < 6; i++) {
      ssidList[i] = WiFi.SSID(i);
      tft.fillRoundRect(10, 20 + i * 35, 300, 30, 4, TFT_YELLOW);
      tft.setCursor(20, 27 + i * 35);
      tft.setTextColor(TFT_BLACK);
      tft.print(ssidList[i]);
    }
  }
  tft.fillRoundRect(220, 200, 90, 35, 5, TFT_RED);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(235, 210);
  tft.print("BACK");
}
void drawPasswordInput() {
  currentPage = PAGE_WIFI;
  currentWifiScreen = ENTER_PASSWORD;
  tft.fillScreen(TFT_BLACK);
  drawTextBox();
  drawKeyboard();
  tft.fillRoundRect(260, 5, 55, 25, 3, TFT_RED);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(267, 12);
  tft.print("BACK");
}
void drawTextBox() {
  tft.fillRect(0, 0, 320, 100, TFT_BLACK);
  tft.drawRect(10, 30, 300, 40, TFT_WHITE);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(15, 40);
  tft.print(typedText.length() == 0 ? "Type Password" : typedText);
}
void drawKeyboard() {
  tft.setTextSize(2);
  for (int row = 0; row < 3; row++) {
    for (int col = 0; col < 10; col++) {
      int x = col * 32;
      int y = row * 35 + 100;
      tft.fillRoundRect(x + 1, y, 31, 34, 3, TFT_YELLOW);
      tft.setTextColor(TFT_BLACK);
      tft.setCursor(x + 4, y + 8);
      String label;
      if (alphaMode) {
        label = keysAlpha[row][col];
        if (Caps && label != "Caps" && label != "Sme" && label != "BS") {
          label.toUpperCase();
        } else if (!Caps && label != "Caps" && label != "Sme" && label != "BS") {
          label.toLowerCase();
        }
      } else {
        label = keysNum[row][col];
      }
      tft.print(label);
    }
  }
  tft.fillRoundRect(1, 205, 120, 34, 3, TFT_BLUE);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(35, 215);
  tft.print("Space");
  tft.fillRoundRect(123, 205, 70, 34, 3, TFT_CYAN);
  tft.setTextColor(TFT_BLACK);
  tft.setCursor(130, 215);
  tft.print("Enter");
  tft.fillRoundRect(195, 205, 32, 34, 3, TFT_YELLOW);
  tft.setTextColor(TFT_BLACK);
  tft.setCursor(199, 215);
  tft.print(alphaMode ? "NUM" : "ALP");
  tft.fillRoundRect(229, 205, 90, 34, 3, TFT_WHITE);
  tft.setTextColor(TFT_RED);
  tft.setCursor(245, 217);
  tft.print("RESET");
}
void drawConnectedScreen() {
  currentPage = PAGE_WIFI;
  currentWifiScreen = CONNECTED;
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(TFT_GREEN);
  tft.setCursor(30, 100);
  tft.print("WIFI IS CONNECTED");
  delay(3000);
  configTime(3600, 0, "pool.ntp.org", "time.nist.gov");
  struct tm timeinfo;
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);
  
  if (getLocalTime(&timeinfo)) {
    // If NTP sync is successful, adjust the DS3231 RTC module
    rtc.adjust(DateTime(
      timeinfo.tm_year + 1900,
      timeinfo.tm_mon + 1,
      timeinfo.tm_mday,
      timeinfo.tm_hour,
      timeinfo.tm_min,
      timeinfo.tm_sec
    ));
    tft.setTextColor(TFT_GREEN);
    tft.setCursor(40, 100);
    tft.print("RTC synchronized");
    tft.setCursor(40, 130);
    tft.print("Succesfully!");
    
  } else {
    tft.setTextColor(TFT_RED);
    tft.setCursor(20, 100);
    tft.print("Failed to get time from");
    tft.setCursor(20, 130);
    tft.print("NTP server!");
  }
  delay(3000);
  
  drawWifiMainPage();
}

// --- Animation List Page ---
void showAnimationListPage() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(10, 30);
  tft.println("Animations");
  
  // Back button
  tft.fillRoundRect(270, 5, 50, 25, 5, TFT_RED);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.drawString("Back", 275, 10, 1);
  // Animation options
  tft.setCursor(20, 80); 
   tft.println(" SPIRO");
  tft.setCursor(20, 120); 
  tft.println(" Screen_Brightness");

  uint16_t tx, ty;
  while (true) {
    checkAlarmTimerAndBuzzer();
    if (currentPage == PAGE_ALARM_RINGING || currentPage == PAGE_TIMER_RINGING) return;
    if (tft.getTouch(&tx, &ty)) {
      if (tx > 280 && ty < 20) {
        drawMenuPage();
        return;
      } else if (ty > 70 && ty < 100) { // SPIRO option
        showSpiroAnimation();
        return;
      } else if (ty > 110 && ty < 140) { // Screen_Brightness option
        showBrightnessPage();
        return;
      }
    }
    delay(20);
  }


}
// --- SPIRO Animation ---
void showSpiroAnimation() {
  
  #define DEG2RAD 0.0174532925
  unsigned long touchStart = 0;
  bool touching = false;
  unsigned long runTime = 0;
  while (true) {
    checkAlarmTimerAndBuzzer();
     if (currentPage == PAGE_ALARM_RINGING || currentPage == PAGE_TIMER_RINGING) return;
    runTime = millis();
    tft.fillScreen(TFT_BLACK);
    int n = random(2, 23), r = random(20, 100);
    for (long i = 0; i < (360 * n); i++) {
      float sx = cos((i / n - 90) * DEG2RAD);
      float sy = sin((i / n - 90) * DEG2RAD);
      uint16_t x0 = sx * (120 - r) + 159;
      uint16_t yy0 = sy * (120 - r) + 119;
      sy = cos(((i % 360) - 90) * DEG2RAD);
      sx = sin(((i % 360) - 90) * DEG2RAD);
      uint16_t x1 = sx * r + x0;
      uint16_t yy1 = sy * r + yy0;
      tft.drawPixel(x1, yy1, rainbow(map(i%360,0,360,0,127)));
    }
    r = random(20, 100);
    for (long i = 0; i < (360 * n); i++) {
      float sx = cos((i / n - 90) * DEG2RAD);
      float sy = sin((i / n - 90) * DEG2RAD);
      uint16_t x0 = sx * (120 - r) + 159;
      uint16_t yy0 = sy * (120 - r) + 119;
      sy = cos(((i % 360) - 90) * DEG2RAD);
      sx = sin(((i % 360) - 90) * DEG2RAD);
      uint16_t x1 = sx * r + x0;
      uint16_t yy1 = sy * r + yy0;
      tft.drawPixel(x1, yy1, rainbow(map(i%360,0,360,0,127)));
    }

    delay(2000);
 // inside the main while(true) after drawing frame:

 // --- Touch & hold detection  
       uint16_t x, y;
      if (tft.getTouch(&x, &y)) {  // screen touched
        if (!touching) {
          touching = true;
          touchStart = millis();
        } else if (millis() - touchStart > 1000) { // hold > 1s
          drawMenuPage();// return to menu
          return; // exit animation function
        }
      } else {
        touching = false;
      }
 
  }
}
unsigned int rainbow(int value) {
  byte red = 0, green = 0, blue = 0;
  byte quadrant = value / 32;
  if (quadrant == 0) { blue = 31;
  green = 2 * (value % 32); }
  if (quadrant == 1) { blue = 31 - (value % 32);
  green = 63; }
  if (quadrant == 2) { green = 63; red = value % 32;
  }
  if (quadrant == 3) { green = 63 - 2 * (value % 32); red = 31;
  }
  return (red << 11) + (green << 5) + blue;
}


// ================= Setup =================
void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  rtc.begin();
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC module!");
    while (1);
  }
  if (rtc.lostPower()) {
    Serial.println("RTC lost power, let's set the time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  tft.init();
  tft.setRotation(1);
  tft.setTouch(calData);
 ledcAttach(backlightPin, freq, resolution);
  updateBrightness(horizValue);

  pinMode(BUZZER_PIN, OUTPUT);
  noTone(BUZZER_PIN); // Make sure it's silent at start
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  // Initialize lastWifiConnectedStatus with the current WiFi state
  lastWifiConnectedStatus = (WiFi.status() == WL_CONNECTED);
  showHomePage();
}
bool buzzerState = false;
uint32_t buzzerToggleTime = 0;


void checkAlarmTimerAndBuzzer() {
  DateTime curr = rtc.now();
  // --- Alarm trigger (hour/minute equality) ---
  if (!alarmTriggered && alarmEnabled) {
    if (curr.hour() == alarmHour && curr.minute() == alarmMinute) {
      alarmTriggered = true;
      buzzerToggleTime = millis();
      buzzerState = false;
      showAlarmRingingPage();
    }
  }
  // --- Timer countdown ---
  if (timerRunning && !timerTriggered) {
    static int lastDisplayedSeconds = -1; // New static variable
    unsigned long elapsedSeconds = (millis() - timerStartTime) / 1000;
    long remainingSeconds = (long)initialTimerTotalSeconds - (long)elapsedSeconds;
    if (remainingSeconds <= 0) {
      timerRunning = false;
      timerTriggered = true;
      buzzerToggleTime = millis();
      buzzerState = false;
      showTimerRingingPage();
    } else {
      int newHours = remainingSeconds / 3600;
      int newMinutes = (remainingSeconds % 3600) / 60;
      int newSeconds = remainingSeconds % 60;
      // keep the variables current even when user is not on timer page
      timerHours = newHours;
      timerMinutes = newMinutes;
      timerSeconds = newSeconds;
      if (currentPage == PAGE_TIMER && newSeconds != lastDisplayedSeconds) {
        updateTimerDisplay();
        lastDisplayedSeconds = newSeconds;
      }
    }
  }
  // --- Buzzer toggle while ringing ---
  if ((currentPage == PAGE_ALARM_RINGING && alarmTriggered) || (currentPage == PAGE_TIMER_RINGING && timerTriggered)) {
    if (millis() - buzzerToggleTime >= 500) {
      buzzerToggleTime = millis();
      buzzerState = !buzzerState;
      if (buzzerState) tone(BUZZER_PIN, 2000);
      else noTone(BUZZER_PIN);
    }
  }
}



// ================= Loop =================
void loop() {

  checkAlarmTimerAndBuzzer();

  static uint32_t homePageUpdateTargetTime = millis() + 1000;
  static int lastMinute = -1;
  uint16_t x, y;
  bool touched = tft.getTouch(&x, &y);
  
  // New touch handling logic
  if (touched) {
    // If it's a new press (isPressing was false)
    if (!isPressing) {
        isPressing = true;
        lastPressMillis = millis();
        longPressInterval = 300; // Adjusted initial delay for long press
        longPressButton = NONE; // Reset long press button state

        if (currentPage == PAGE_WIFI) {
          if (currentWifiScreen == WIFI_HOME) {
            if (x > 60 && x < 260 && y > 80 && y < 130) {
              drawSSIDList();
            } else if (x > 60 && x < 260 && y > 160 && y < 210) {
              WiFi.disconnect(true);
              WiFi.mode(WIFI_OFF);
              tft.fillScreen(TFT_BLACK);
              tft.setTextSize(2);
              tft.setTextColor(TFT_WHITE);
              tft.setCursor(80, 120);
              tft.print("WIFI DISCONNECTED!");
              delay(1500);
              drawWifiMainPage();
            } else if (x > 270 && x < 320 && y < 30) {
              drawMenuPage();
            }
          } else if (currentWifiScreen == SCAN) {
            for (int i = 0; i < ssidCount && i < 6; i++) {
              if (y > 20 + i * 35 && y < 50 + i * 35) {
                selectedSSID = ssidList[i];
                typedText = "";
                drawPasswordInput();
                return;
              }
            }
            if (x > 220 && x < 310 && y > 200 && y < 235) {
              drawWifiMainPage();
              return;
            }
          } else if (currentWifiScreen == ENTER_PASSWORD) {
            if (x > 260 && x < 315 && y > 5 && y < 30) {
              drawSSIDList();
              return;
            }
            if (x > 1 && x < 121 && y > 205 && y < 239) {
              typedText += ' ';
              drawTextBox();
              return;
            }
            if (x > 123 && x < 193 && y > 205 && y < 239) {
              tft.fillRect(0, 0, 320, 100, TFT_BLACK);
              tft.setTextSize(2);
              tft.setTextColor(TFT_YELLOW);
              tft.setCursor(15, 40);
              tft.print("Connecting to: " + selectedSSID);
              tft.setCursor(15, 65);
              tft.print("Password: " + String(typedText.length()) + " chars");

              WiFi.begin(selectedSSID.c_str(), typedText.c_str());
              unsigned long start = millis();
              bool connected = false;
              while (millis() - start < 15000) {
                if (WiFi.status() == WL_CONNECTED) {
                  connected = true;
                  break;
                }
                delay(100);
              }

              if (connected) {
                drawConnectedScreen();
              } else {
                WiFi.disconnect(true);
                WiFi.mode(WIFI_STA);
                tft.fillScreen(TFT_BLACK);
                tft.setTextSize(2);
                tft.setTextColor(TFT_RED);
                tft.setCursor(40, 100);
                tft.print("Connection Failed!");
                tft.setCursor(40, 130);
                tft.print("Try again.");
                delay(2000);
                drawPasswordInput();
              }
              return;
            }
            if (x > 195 && x < 227 && y > 205 && y < 239) {
              alphaMode = !alphaMode;
              keysAlpha[2][0] = Caps ? "Sme" : "Caps";
              drawKeyboard();
              return;
            }
            if (x > 229 && x < 319 && y > 205 && y < 239) {
              typedText = "";
              drawTextBox();
              return;
            }
            for (int row = 0; row < 3; row++) {
              for (int col = 0; col < 10; col++) {
                int keyX = col * 32;
                int keyY = row * 35 + 100;
                if (x > keyX && x < keyX + 32 && y > keyY && y < keyY + 35) {
                  String key;
                  if (alphaMode) {
                    key = keysAlpha[row][col];
                    if (key == "Caps") {
                      Caps = !Caps;
                      keysAlpha[2][0] = Caps ? "Sme" : "Caps";
                      drawKeyboard();
                      return;
                    } else if (key == "Sme") {
                      Caps = !Caps;
                      keysAlpha[2][0] = Caps ? "Sme" : "Caps";
                      drawKeyboard();
                      return;
                    } else if (key == "BS") {
                      if (typedText.length() > 0) {
                        typedText.remove(typedText.length() - 1);
                      }
                    } else {
                      if (!Caps) key.toLowerCase();
                      typedText += key;
                    }
                  } else {
                    key = keysNum[row][col];
                    if (key == "BS") {
                      if (typedText.length() > 0) {
                        typedText.remove(typedText.length() - 1);
                      }
                    } else {
                      typedText += key;
                    }
                  }
                  drawTextBox();
                  return;
                }
              }
            }
          }
        }
        
        // Alarm Page buttons
        else if (currentPage == PAGE_ALARM) {
          if (x > 270 && y < 30) {
            drawMenuPage();   // Back
          } else if (x > 220 && x < 260 && y > 60 && y < 90) { // +Hour
            alarmHour = (alarmHour + 1) % 24;
            updateAlarmDisplay();
            longPressButton = ALARM_H_PLUS;
          } else if (x > 270 && x < 310 && y > 60 && y < 90) { // +Min
            alarmMinute = (alarmMinute + 1) % 60;
            updateAlarmDisplay();
            longPressButton = ALARM_M_PLUS;
          } else if (x > 220 && x < 260 && y > 100 && y < 130) { // -Hour
            alarmHour = (alarmHour - 1 + 24) % 24;
            updateAlarmDisplay();
            longPressButton = ALARM_H_MINUS;
          } else if (x > 270 && x < 310 && y > 100 && y < 130) { // -Min
            alarmMinute = (alarmMinute - 1 + 60) % 60;
            updateAlarmDisplay();
            longPressButton = ALARM_M_MINUS;
          } else if (x > 120 && x < 200 && y > 190 && y < 220) {
            alarmEnabled = !alarmEnabled;
            updateAlarmDisplay();
          } 

          else if (x > 220 && x < 300 && y > 190 && y < 220) {
            alarmHour = 0;
            alarmMinute = 0;
            updateAlarmDisplay();
          }
        }
        
        // Timer Page buttons
        else if (currentPage == PAGE_TIMER) {
          if (x > 270 && y < 30) {
            drawMenuPage(); // Back to menu
          } else if (x > 220 && x < 260 && y > 60 && y < 90) { // +H
            timerHours = (timerHours + 1) % 100;
            updateTimerDisplay();
            longPressButton = TIMER_H_PLUS;
          } else if (x > 270 && x < 310 && y > 60 && y < 90) { // -H
            timerHours = (timerHours - 1 + 100) % 100;
            updateTimerDisplay();
            longPressButton = TIMER_H_MINUS;
          } else if (x > 220 && x < 260 && y > 100 && y < 130) { // +M
            timerMinutes = (timerMinutes + 1) % 60;
            updateTimerDisplay();
            longPressButton = TIMER_M_PLUS;
          } else if (x > 270 && x < 310 && y > 100 && y < 130) { // -M
            timerMinutes = (timerMinutes - 1 + 60) % 60;
            updateTimerDisplay();
            longPressButton = TIMER_M_MINUS;
          } else if (x > 220 && x < 260 && y > 140 && y < 170) { // +S
            timerSeconds = (timerSeconds + 1) % 60;
            updateTimerDisplay();
            longPressButton = TIMER_S_PLUS;
          } else if (x > 270 && x < 310 && y > 140 && y < 170) { // -S
            timerSeconds = (timerSeconds - 1 + 60) % 60;
            updateTimerDisplay();
            longPressButton = TIMER_S_MINUS;
          } else if (x > 120 && x < 200 && y > 190 && y < 220) { // Start/Stop button
              if(timerRunning) {
                  timerRunning = false;
                  showTimerPage(); 
              } else {
                  if (timerHours > 0 || timerMinutes > 0 || timerSeconds > 0) {
                    timerRunning = true;
                    timerStartTime = millis();
                    initialTimerTotalSeconds = timerHours * 3600 + timerMinutes * 60 + timerSeconds;
                    showTimerPage(); 
                  }
              }
          } else if (x > 220 && x < 300 && y > 190 && y < 220) { // Clear button
            timerHours = 0;
            timerMinutes = 0;
            timerSeconds = 0;
            timerRunning = false;
            showTimerPage();
          }
        }
        
        // Other Page buttons
        else if (currentPage == PAGE_HOME) {
          if (x > MENU_BTN_X && y > MENU_BTN_Y) {
            drawMenuPage();
          }
        } else if (currentPage == PAGE_MENU) {
          if (x > 270 && y < 30) {
            showHomePage();
          } else if (x > 24 && x < 78 && y > 46 && y < 100) {
            showHomePage();
          } else if (x > 110 && x < 170 && y > 41 && y < 105) {
            showAnalogClockPage();
          } else if (x > 205 && x < 265 && y > 43 && y < 107) {
            showAlarmPage();
          } else if (x > 22 && x < 82 && y > 129 && y < 193) {
            showTimerPage();
          } else if (x > 112 && x < 169 && y > 141 && y < 189) {
            drawWifiMainPage();
          } else if (x > 219 && x < 255 && y > 136 && y < 184) {
            showAnimationListPage();
          }
        } else if (currentPage == PAGE_ANALOG) {
          if (x > 270 && y < 30) {
            drawMenuPage();
          }
        } else if (currentPage == PAGE_ALARM_RINGING) {
          if (x > 90 && x < 230 && y > 150 && y < 190) {
            noTone(BUZZER_PIN);
            alarmEnabled = false;
            alarmTriggered = false;
            showHomePage();
          }
        } else if (currentPage == PAGE_TIMER_RINGING) {
          if (x > 90 && x < 230 && y > 150 && y < 190) {
            noTone(BUZZER_PIN);
            timerTriggered = false;
            showHomePage();
          }
        } else {
          if (x > 270 && y < 30) {
            drawMenuPage();
          }
        }
    } else { // It's a continuous press
        if (longPressButton != NONE && (millis() - lastPressMillis) >= longPressInterval) {
            lastPressMillis = millis();

            // Accelerate the long press interval
            if (longPressInterval > 50) {
              longPressInterval -= 50; // Increased speed of acceleration
            }

            // Perform the increment/decrement based on the button
            if (longPressButton == ALARM_H_PLUS) {
                alarmHour = (alarmHour + 1) % 24;
                updateAlarmDisplay();
            } else if (longPressButton == ALARM_H_MINUS) {
                alarmHour = (alarmHour - 1 + 24) % 24;
                updateAlarmDisplay();
            } else if (longPressButton == ALARM_M_PLUS) {
                alarmMinute = (alarmMinute + 1) % 60;
                updateAlarmDisplay();
            } else if (longPressButton == ALARM_M_MINUS) {
                alarmMinute = (alarmMinute - 1 + 60) % 60;
                updateAlarmDisplay();
            } else if (longPressButton == TIMER_H_PLUS) {
                timerHours = (timerHours + 1) % 100;
                updateTimerDisplay();
            } else if (longPressButton == TIMER_H_MINUS) {
                timerHours = (timerHours - 1 + 100) % 100;
                updateTimerDisplay();
            } else if (longPressButton == TIMER_M_PLUS) {
                timerMinutes = (timerMinutes + 1) % 60;
                updateTimerDisplay();
            } else if (longPressButton == TIMER_M_MINUS) {
                timerMinutes = (timerMinutes - 1 + 60) % 60;
                updateTimerDisplay();
            } else if (longPressButton == TIMER_S_PLUS) {
                timerSeconds = (timerSeconds + 1) % 60;
                updateTimerDisplay();
            } else if (longPressButton == TIMER_S_MINUS) {
                timerSeconds = (timerSeconds - 1 + 60) % 60;
                updateTimerDisplay();
            }
        }
    }
  } else {
    // No touch detected, reset long press state
    isPressing = false;
    longPressButton = NONE;
    longPressInterval = 500; // Reset to original for a new press
  }
  
  // Check if alarm time matches
  if (!alarmTriggered && alarmEnabled) {
    DateTime now = rtc.now();
    if (now.hour() == alarmHour && now.minute() == alarmMinute) {
      alarmTriggered = true;
      buzzerToggleTime = millis();
      buzzerState = false;
      showAlarmRingingPage();
    }
  }

  // Update digital clock on home page
  if (currentPage == PAGE_HOME && millis() > homePageUpdateTargetTime) {
    homePageUpdateTargetTime = millis() + 1000;
    now = rtc.now();
    if (now.minute() != lastMinute) {
      lastMinute = now.minute();
      // Instead of calling updateHomePageTimeAndDate(), we now call showHomePage()
      // to force a full redraw when the minute changes.
      showHomePage(); 
    }
  }

  // === MODIFICATION FOR WIFI ICON FIX ===
  // Check and update WiFi status icon if it changes
  bool currentWifiConnected = (WiFi.status() == WL_CONNECTED);
  if (currentWifiConnected != lastWifiConnectedStatus) {
    if (currentPage == PAGE_HOME) { // Only update if on the home page
      // Call showHomePage() to redraw the entire screen, including the correct WiFi icon
      showHomePage(); 
    }
    lastWifiConnectedStatus = currentWifiConnected;
  }
  // === END MODIFICATION ===

  // Update analog clock every second
  if (currentPage == PAGE_ANALOG && millis() > analogTargetTime) {
    analogTargetTime = millis() + 1000;
    DateTime now = rtc.now();
    int hh = now.hour(), mm = now.minute(), ss = now.second();

    sdeg = ss * 6;
    mdeg = mm * 6 + sdeg * 0.01666667;
    hdeg = hh * 30 + mdeg * 0.0833333;

    hx = cos((hdeg - 90) * DEG_TO_RAD);
    hy = sin((hdeg - 90) * DEG_TO_RAD);
    mx = cos((mdeg - 90) * DEG_TO_RAD);
    my = sin((mdeg - 90) * DEG_TO_RAD);
    sx = cos((sdeg - 90) * DEG_TO_RAD);
    sy = sin((sdeg - 90) * DEG_TO_RAD);

    if (ss == 0 || analogInitial) {
      analogInitial = false;

      // Erase hour hand
      tft.drawLine(ohx, ohy, CENTER_X, CENTER_Y + 1, TFT_BLACK);
      ohx = hx * 62 + CENTER_X;
      ohy = hy * 62 + CENTER_Y + 1;

      // Erase minute hand
      tft.drawLine(omx, omy, CENTER_X, CENTER_Y + 1, TFT_BLACK);
      omx = mx * 84 + CENTER_X;
      omy = my * 84 + CENTER_Y + 1;
    }

    // Erase second hand
    tft.drawLine(osx, osy, CENTER_X, CENTER_Y + 1, TFT_BLACK);
    osx = sx * 90 + CENTER_X;
    osy = sy * 90 + CENTER_Y + 1;

    // Draw new hands
    tft.drawLine(ohx, ohy, CENTER_X, CENTER_Y + 1, TFT_WHITE); // hour
    tft.drawLine(omx, omy, CENTER_X, CENTER_Y + 1, TFT_WHITE); // minute
    tft.drawLine(osx, osy, CENTER_X, CENTER_Y + 1, TFT_RED);   // second

    // Center red cap
    tft.fillCircle(CENTER_X, CENTER_Y + 1, 3, TFT_RED);
  }

  // Handle timer countdown
  if (timerRunning) {
      unsigned long elapsedSeconds = (millis() - timerStartTime) / 1000;
      long remainingSeconds = initialTimerTotalSeconds - elapsedSeconds;

      if (remainingSeconds <= 0) {
          timerRunning = false;
          timerTriggered = true;
          buzzerToggleTime = millis();
          buzzerState = false;
          showTimerRingingPage();
      } else {
          int newHours = remainingSeconds / 3600;
          int newMinutes = (remainingSeconds % 3600) / 60;
          int newSeconds = remainingSeconds % 60;
          
          if(newSeconds != timerSeconds) {
              timerHours = newHours;
              timerMinutes = newMinutes;
              timerSeconds = newSeconds;
              if (currentPage == PAGE_TIMER) {
                  updateTimerDisplay(); // Only update the timer text, not the whole page
              }
          }
      }
  }

  // Toggle buzzer every 500ms if alarm or timer is ringing
  if ((currentPage == PAGE_ALARM_RINGING && alarmTriggered) || (currentPage == PAGE_TIMER_RINGING && timerTriggered)) {
    if (millis() - buzzerToggleTime >= 500) {
      buzzerToggleTime = millis();
      buzzerState = !buzzerState;

      if (buzzerState) {
        tone(BUZZER_PIN, 2000);
      } else {
        noTone(BUZZER_PIN);
      }
    }
  }
}

// New helper function to update only time and date on home page
void updateHomePageTimeAndDate() {
  tft.setTextDatum(TL_DATUM);
  now = rtc.now();

  byte hour = now.hour();
  byte minute = now.minute();

  // Clear and redraw time
  String timeStr = (hour < 10 ? "0" : "") + String(hour) + ":" +
                   (minute < 10 ? "0" : "") + String(minute);
  int xTime = (tft.width() - tft.textWidth(timeStr, 8)) / 2;
  tft.fillRect(0, 47, tft.width(), tft.fontHeight(8), TFT_BLACK); // Clear old time
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString(timeStr, xTime, 47, 8);

  // Clear and redraw date
  String dateStr = String(now.day()) + getDaySuffix(now.day()) + " " +
                   monthsOfYear[now.month() - 1] + " " + String(now.year());
  int xDate = (tft.width() - tft.textWidth(dateStr, 4)) / 2;
  tft.fillRect(0, 150, tft.width(), tft.fontHeight(4), TFT_BLACK); // Clear old date
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString(dateStr, xDate, 150, 4);

  // Clear and redraw day of week
  String dayStr = daysOfWeek[now.dayOfTheWeek()];
  int xDay = (tft.width() - tft.textWidth(dayStr, 4)) / 2;
  tft.fillRect(0, 200, tft.width(), tft.fontHeight(4), TFT_BLACK); // Clear old day
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString(dayStr, xDay, 200, 4);

  // Re-draw the menu button to ensure it's not accidentally cleared
  drawMenuButton();
}
