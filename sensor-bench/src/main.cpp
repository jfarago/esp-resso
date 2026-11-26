// T-Display S3 sensor bench: INA228 current monitor.
// Shows bus voltage, current, power, cumulative mAh and the time it covers.
// Hold the front button to zero mAh and the timer.

#include <Arduino.h>
#include <Wire.h>
#include <TFT_eSPI.h>
#include <Adafruit_INA228.h>

#include "config.h"

static TFT_eSPI tft;
static Adafruit_INA228 ina;
static bool inaOk = false;

// Colors
static constexpr uint16_t COL_BG = TFT_BLACK;
static constexpr uint16_t COL_LABEL = 0x8410;  // grey

struct Row {
  const char* label;
  const char* unit;
  uint16_t color;
};
static constexpr int NUM_ROWS = 5;
static constexpr int TIME_ROW = 4;
static const Row ROWS[NUM_ROWS] = {
    {"Volts", "V", TFT_CYAN},
    {"Current", "mA", TFT_YELLOW},
    {"Power", "mW", TFT_ORANGE},
    {"Charge", "mAh", TFT_GREEN},
    {"Time", "", TFT_WHITE},
};

static constexpr int ROW_H = 30;
static constexpr int UNIT_X = 318;       // unit text right edge
static constexpr int VALUE_X = 266;      // value right edge
static constexpr int VALUE_PAD = 140;    // value field width to erase
static constexpr int BAR_Y = 152;
static constexpr int BAR_H = 18;

// ---- Button (hold to clear) ----
static uint32_t pressStart = 0;
static bool pressed = false;
static bool clearedThisPress = false;
static uint32_t clearedAt = 0;

// When the mAh accumulator was last zeroed (us; 64-bit so it never wraps).
static int64_t monitorStartUs = 0;

static void resetMeasurement() {
  ina.resetAccumulators();
  monitorStartUs = esp_timer_get_time();
}

static void drawStatic() {
  tft.fillScreen(COL_BG);
  tft.setTextDatum(TL_DATUM);
  tft.setTextPadding(0);
  for (int i = 0; i < NUM_ROWS; i++) {
    tft.setTextColor(COL_LABEL, COL_BG);
    tft.drawString(ROWS[i].label, 6, i * ROW_H + 3, 4);
    tft.setTextColor(ROWS[i].color, COL_BG);
    tft.setTextDatum(TR_DATUM);
    tft.drawString(ROWS[i].unit, UNIT_X, i * ROW_H + 3, 4);
    tft.setTextDatum(TL_DATUM);
  }
}

static void drawValue(int row, const char* text) {
  tft.setTextColor(ROWS[row].color, COL_BG);
  tft.setTextDatum(TR_DATUM);
  tft.setTextPadding(VALUE_PAD);
  // The time has no unit, so it sits flush with the unit column's right edge.
  tft.drawString(text, row == TIME_ROW ? UNIT_X : VALUE_X, row * ROW_H + 3, 4);
  tft.setTextPadding(0);
  tft.setTextDatum(TL_DATUM);
}

// Bottom strip: hold-progress bar, "CLEARED" flash, or hint text.
static void drawBottom(float holdFrac, bool justCleared) {
  static int lastState = -1;  // quantised so we only repaint on change
  int state = justCleared ? 1000 : (int)(holdFrac * 100.0f);
  if (state == lastState) return;
  lastState = state;

  tft.fillRect(0, BAR_Y, 320, BAR_H, COL_BG);
  if (justCleared) {
    tft.setTextColor(TFT_GREEN, COL_BG);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("mAh CLEARED", 160, BAR_Y + BAR_H / 2, 2);
  } else if (holdFrac > 0.0f) {
    tft.drawRect(6, BAR_Y + 2, 308, BAR_H - 4, TFT_RED);
    tft.fillRect(8, BAR_Y + 4, (int)(304 * holdFrac), BAR_H - 8, TFT_RED);
  } else {
    tft.setTextColor(COL_LABEL, COL_BG);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("hold button to clear mAh", 160, BAR_Y + BAR_H / 2, 2);
  }
  tft.setTextDatum(TL_DATUM);
}

static void showMessage(const char* msg, uint16_t color) {
  tft.fillScreen(COL_BG);
  tft.setTextColor(color, COL_BG);
  tft.setTextDatum(MC_DATUM);
  tft.drawString(msg, 160, 85, 4);
  tft.setTextDatum(TL_DATUM);
}

// Log every responding I2C address so wiring problems are visible over serial.
static void scanI2c() {
  int found = 0;
  for (uint8_t a = 1; a < 127; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) {
      Serial.printf("I2C device at 0x%02X\n", a);
      found++;
    }
  }
  if (!found) Serial.println("I2C scan: no devices (check wiring/pins)");
}

static bool initIna() {
  if (!ina.begin(INA228_ADDR, &Wire)) return false;
  ina.setShunt(SHUNT_OHMS, MAX_CURRENT_A);
  ina.setMode(INA228_MODE_CONT_TEMP_BUS_SHUNT);
  ina.setAveragingCount(INA228_COUNT_16);
  ina.setVoltageConversionTime(INA228_TIME_1052_us);
  ina.setCurrentConversionTime(INA228_TIME_1052_us);
  resetMeasurement();
  return true;
}

// INA228 CHARGE register (0x0A): 40-bit signed accumulator, LSB = CURRENT_LSB coulombs.
// The Adafruit library doesn't expose it, so read it over raw I2C. Returns mAh.
static bool readMah(double& mah) {
  Wire.beginTransmission(INA228_ADDR);
  Wire.write(0x0A);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((int)INA228_ADDR, 5) != 5) return false;
  int64_t raw = 0;
  for (int i = 0; i < 5; i++) raw = (raw << 8) | (uint8_t)Wire.read();
  if (raw & (1LL << 39)) raw -= (1LL << 40);  // sign-extend
  const double currentLsb = MAX_CURRENT_A / 524288.0;  // matches Adafruit setShunt(): max/2^19
  mah = raw * currentLsb / 3.6;
  return true;
}

static void clearMah() {
  if (inaOk) resetMeasurement();
  clearedAt = millis();
  Serial.println("mAh cleared");
}

// Returns hold fraction 0..1 (0 when not held).
static float pollButton() {
  bool down = digitalRead(PIN_BUTTON) == LOW;
  uint32_t now = millis();
  if (down && !pressed) {
    pressed = true;
    clearedThisPress = false;
    pressStart = now;
  } else if (!down) {
    pressed = false;
  }
  if (!pressed) return 0.0f;
  uint32_t held = now - pressStart;
  if (!clearedThisPress && held >= CLEAR_HOLD_MS) {
    clearedThisPress = true;  // one clear per press; release to re-arm
    clearMah();
  }
  return clearedThisPress ? 0.0f : (float)held / CLEAR_HOLD_MS;
}

void setup() {
  Serial.begin(115200);
  Serial.setTxTimeoutMs(0);  // never stall the display when no serial monitor is reading
  pinMode(PIN_POWER_ON, OUTPUT);
  digitalWrite(PIN_POWER_ON, HIGH);
  pinMode(PIN_BUTTON, INPUT_PULLUP);

  tft.init();
  tft.setRotation(1);  // landscape, 320x170
  tft.fillScreen(COL_BG);

  Wire.begin(PIN_SDA, PIN_SCL, I2C_FREQ);
  inaOk = initIna();
  if (inaOk) {
    drawStatic();
  } else {
    showMessage("INA228 not found", TFT_RED);
    Serial.printf("INA228 not found at 0x%02X (SDA=%d SCL=%d)\n", INA228_ADDR, PIN_SDA, PIN_SCL);
    scanI2c();
  }
}

void loop() {
  static uint32_t lastDisplay = 0, lastSerial = 0, lastRetry = 0;
  uint32_t now = millis();

  if (!inaOk) {
    if (now - lastRetry > 2000) {
      lastRetry = now;
      if (initIna()) {
        inaOk = true;
        drawStatic();
      }
    }
    return;
  }

  float hold = pollButton();
  // Serial 'r' also clears (handy when headless).
  if (Serial.available() && Serial.read() == 'r') clearMah();

  if (now - lastDisplay >= DISPLAY_PERIOD_MS) {
    lastDisplay = now;
    float v = ina.readBusVoltage() / 1000.0f;  // library returns mV
    float ma = ina.readCurrent();
    float mw = ina.readPower();
    double mahD = 0;
    float mah = readMah(mahD) ? (float)mahD : NAN;

    char buf[16];
    snprintf(buf, sizeof(buf), "%.3f", v);
    drawValue(0, buf);
    snprintf(buf, sizeof(buf), "%.1f", ma);
    drawValue(1, buf);
    snprintf(buf, sizeof(buf), "%.1f", mw);
    drawValue(2, buf);
    if (isnan(mah)) snprintf(buf, sizeof(buf), "--");
    else snprintf(buf, sizeof(buf), "%.3f", mah);
    drawValue(3, buf);

    uint32_t secs = (uint32_t)((esp_timer_get_time() - monitorStartUs) / 1000000);
    char elapsed[16];
    snprintf(elapsed, sizeof(elapsed), "%02lu:%02lu:%02lu", (unsigned long)(secs / 3600),
             (unsigned long)(secs / 60 % 60), (unsigned long)(secs % 60));
    drawValue(TIME_ROW, elapsed);

    drawBottom(hold, clearedAt && now - clearedAt < 1500);

    if (now - lastSerial >= SERIAL_PERIOD_MS) {
      lastSerial = now;
      Serial.printf("V=%.3f V  I=%.2f mA  P=%.1f mW  Q=%.3f mAh  t=%s\n", v, ma, mw, mah, elapsed);
    }
  }
}
