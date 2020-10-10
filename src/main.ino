#include <ClickEncoder.h>
#include <TimerOne.h>
#include <TM1637Display.h>

#include "pins.h"

#define VOLUME_PER_PULSE 0.002083297;

float volume = 0;
bool running = false;

ClickEncoder encoder = ClickEncoder(ENC_A, ENC_B, BUTTON, 1);
TM1637Display display(SR_CLOCK, SR_DATA);

void timerInterrupt() {
  encoder.service();
}

void flowSensorInterrupt() {
  if (running) {
    volume -= VOLUME_PER_PULSE;
    if (volume <= 0) {
      digitalWrite(VALVE, LOW);
      volume = 0;
    }
    updateDisplay();
  }
}

void updateDisplay() {
  display.showNumberDecEx((int)(volume * 10), 0b00100000);
}

void setup() {
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
  static int16_t lastPosition = 0;

  if (!running) {
    int16_t encoderPosition = encoder.getValue();
    if (encoderPosition != lastPosition) {
      volume = constrain(volume + (encoderPosition - lastPosition) / 10, 0, 100);
      lastPosition = encoderPosition;
      updateDisplay();
    }
  }

  ClickEncoder::Button button = encoder.getButton();

  if (button == ClickEncoder::Held) {
    running = !running;
    if (!running) {
      digitalWrite(VALVE, LOW);
    } else if (volume > 0) {
      digitalWrite(VALVE, HIGH);
    }
  }
}
