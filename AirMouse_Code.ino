#include <Wire.h>
#include <BleMouse.h>

#define MPU 0x68

// ESP32 I2C pins
#define SDA_PIN 21
#define SCL_PIN 22

// Button pins
#define LEFT_BTN 14
#define RIGHT_BTN 27
#define SCROLL_UP 26
#define SCROLL_DOWN 25

// Gyro drift calibration
float gyroX_offset = 0;
float gyroY_offset = 0;
const int samples = 100; // number of samples for calibration

// Deadzone to prevent cursor drift
const float threshold = 1.0; // degrees/sec

BleMouse bleMouse("ESP32 Air Mouse", "ESP32", 100);

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Initialize I2C
  Wire.begin(SDA_PIN, SCL_PIN);

  // Wake up MPU6050
  Wire.beginTransmission(MPU);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);

  // Initialize buttons
  pinMode(LEFT_BTN, INPUT_PULLUP);
  pinMode(RIGHT_BTN, INPUT_PULLUP);
  pinMode(SCROLL_UP, INPUT_PULLUP);
  pinMode(SCROLL_DOWN, INPUT_PULLUP);

  // Start BLE Mouse
  bleMouse.begin();
  Serial.println("BLE Mouse started. Pair with your PC/Mac...");

  // --- Gyro calibration ---
  Serial.println("Calibrating gyro, keep sensor still...");
  for (int i = 0; i < samples; i++) {
    Wire.beginTransmission(MPU);
    Wire.write(0x43); // GYRO_XOUT_H
    Wire.endTransmission(false);
    Wire.requestFrom(MPU, 6, true);

    int16_t GyX = Wire.read() << 8 | Wire.read();
    int16_t GyY = Wire.read() << 8 | Wire.read();
    Wire.read(); Wire.read(); // skip GZ

    gyroX_offset += GyX / 131.0;
    gyroY_offset += GyY / 131.0;

    delay(5);
  }
  gyroX_offset /= samples;
  gyroY_offset /= samples;
  Serial.println("Calibration done.");
}

void loop() {
  if (!bleMouse.isConnected()) return;

  // --- Read MPU6050 gyro for cursor movement ---
  Wire.beginTransmission(MPU);
  Wire.write(0x43); // GYRO_XOUT_H
  Wire.endTransmission(false);
  Wire.requestFrom(MPU, 6, true);

  int16_t GyX = Wire.read() << 8 | Wire.read();
  int16_t GyY = Wire.read() << 8 | Wire.read();
  Wire.read(); Wire.read(); // skip GZ

  float gx = GyX / 131.0 - gyroX_offset;
  float gy = GyY / 131.0 - gyroY_offset;

  // Apply deadzone
  if (abs(gx) < threshold) gx = 0;
  if (abs(gy) < threshold) gy = 0;

  // Map gyro to mouse movement
  int8_t moveX = gy / 4; // adjust sensitivity
  int8_t moveY = gx / 4;
  bleMouse.move(moveX, moveY);

  // --- Buttons for clicks and scrolling ---
  if (digitalRead(LEFT_BTN) == LOW) bleMouse.click(MOUSE_LEFT);
  if (digitalRead(RIGHT_BTN) == LOW) bleMouse.click(MOUSE_RIGHT);
  if (digitalRead(SCROLL_UP) == LOW) bleMouse.move(0, 0, 1);    // scroll up
  if (digitalRead(SCROLL_DOWN) == LOW) bleMouse.move(0, 0, -1); // scroll down

  delay(20); // smooth motion
}
