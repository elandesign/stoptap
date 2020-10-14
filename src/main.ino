#include <ClickEncoder.h>
#include <TimerOne.h>
#include <everytime.h>
#include <LedControl_HW_SPI.h>

#include "pins.h"
#include "debug.h"

#define VOLUME_PER_PULSE 2.2222222222; // in ml

int16_t desiredVolume = 0; // in dl
float remainingVolume = 0; // in ml
bool running = false;

ClickEncoder encoder = ClickEncoder(ENC_A, ENC_B, BUTTON, 4);

LedControl_HW_SPI display = LedControl_HW_SPI();

void startFlow() {
  running = true;
  remainingVolume = desiredVolume * 100.0;
  digitalWrite(VALVE, HIGH);
  DEBUG_PRINTLN("Valve Open");
}

void stopFlow() {
  running = false;
  desiredVolume = (int)(remainingVolume / 100.0);
  digitalWrite(VALVE, LOW);
  DEBUG_PRINTLN("Valve Closed");
}

void timerInterrupt() {
  encoder.service();
}

void flowSensorInterrupt() {
  if (running) {
    remainingVolume -= VOLUME_PER_PULSE;
    if (remainingVolume <= 0) {
      remainingVolume = 0;
      stopFlow();
    }
  }
}

void updateDisplay() {
  int displayValue;

  if (running) {
    DEBUG_PRINT("Remaining: ");
    DEBUG_PRINT(remainingVolume);
    DEBUG_PRINTLN("mL");
    displayValue = (int)(remainingVolume / 100.0);
  } else {
    DEBUG_PRINT("Desired: ");
    DEBUG_PRINT(desiredVolume);
    DEBUG_PRINTLN("00mL");
    displayValue = desiredVolume;
  }

  display.setDigit(0, 3, (byte)((displayValue % 10000) / 1000), false);
  display.setDigit(0, 2, (byte)((displayValue % 1000) / 100), false);
  display.setDigit(0, 1, (byte)((displayValue % 100) / 10), true);
  display.setDigit(0, 0, (byte)(displayValue % 10), false);
}

void setup() {
  DEBUG_SETUP();

  pinMode(VALVE, OUTPUT);
  digitalWrite(VALVE, LOW);

  pinMode(FLOW_SENSOR, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR), flowSensorInterrupt, RISING);

  Timer1.initialize(1000);
  Timer1.attachInterrupt(timerInterrupt);

  encoder.setButtonHeldEnabled(true);
  encoder.setAccelerationEnabled(true);

  display.begin(DISPLAY_CS, 1);
  display.shutdown(0,false);
  display.setIntensity(0,8);
  display.clearDisplay(0);

  updateDisplay();
}

void loop() {
  if (!running) {
    int16_t encoderPosition = encoder.getValue();
    if (encoderPosition != 0) {
      desiredVolume = constrain(desiredVolume + encoderPosition, 0, 1000);
      updateDisplay();
    }
  } else {
    every(1000) {
      updateDisplay();
    }
  }

  ClickEncoder::Button button = encoder.getButton();

  if (button == ClickEncoder::Clicked) {
    if (running) {
      stopFlow();
    } else if (desiredVolume > 0) {
      startFlow();
    }
  }
}
