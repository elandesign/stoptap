#include <ClickEncoder.h>
#include <TimerOne.h>
#include <TM1637Display.h>
#include <everytime.h>

#include "pins.h"
#include "debug.h"

#define VOLUME_PER_PULSE 2.2222222222; // in ml

int16_t desiredVolume = 0; // in dl
float remainingVolume = 0; // in ml
bool running = false;

ClickEncoder encoder = ClickEncoder(ENC_A, ENC_B, BUTTON, 4);
TM1637Display display(DISPLAY_CLOCK, DISPLAY_DATA);

void startFlow() {
  running = true;
  remainingVolume = desiredVolume * 100;
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
  if (running) {
    DEBUG_PRINT("Remaining: ");
    DEBUG_PRINT(remainingVolume);
    DEBUG_PRINTLN("mL");
    display.showNumberDecEx((int)(remainingVolume / 100.0), 0b00100000);
  } else {
    DEBUG_PRINT("Desired: ");
    DEBUG_PRINT(desiredVolume);
    DEBUG_PRINTLN("00mL");
    display.showNumberDecEx(desiredVolume, 0b00100000);
  }
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
