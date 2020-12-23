/** @file
 * This is a library to control the intensity of dimmable AC lamps or other AC loads using triacs.
 *
 * Copyright (c) 2015 Circuitar
 * This software is released under the MIT license. See the attached LICENSE file for details.
 */

#include "Arduino.h"
#include "Dimmer.h"

// Dimmer registry
static Dimmer* dimmers[DIMMER_MAX_TRIAC]{nullptr};     // Pointers to all registered dimmer objects
static uint8_t dimmerCount = 0;                // Number of registered dimmer objects

// Triac pin and timing variables. Using global arrays to make ISR fast.
static volatile uint32_t* triacPinPorts[DIMMER_MAX_TRIAC]; // Triac ports for registered dimmers
static uint8_t triacPinMasks[DIMMER_MAX_TRIAC];           // Triac pin mask for registered dimmers
static uint8_t triacTimes[DIMMER_MAX_TRIAC];              // Triac time for registered dimmers

// Global state variables
bool Dimmer::started{false}; // At least one dimmer has started
static uint32_t sincelastCrossing{0}; // Record how many millis(0 have passed since last Zero Crossing detection, used for debouncing zero crossing circuit

// Zero cross interrupt
void ICACHE_RAM_ATTR callZeroCross() {
  if(millis() - sincelastCrossing > 3) {
    sincelastCrossing = millis();
    // Turn off all triacs and disable further triac activation before anything else
    for (uint8_t i = 0; i < dimmerCount; i++) {
      *triacPinPorts[i] &= ~triacPinMasks[i];
      triacTimes[i] = 255;
    }

    // Process each registered dimmer object
    for (uint8_t i = 0; i < dimmerCount; i++) {
      dimmers[i]->pwmtimer->interval(triacTimes[i]);
      dimmers[i]->pwmtimer->start();

      // If triac time was already reached, activate it
      if (sincelastCrossing >= triacTimes[i]) {
      *triacPinPorts[i] |= triacPinMasks[i];
      }
    }
  }
}

void zeroCross() {
  // Turn next half cycle on if number of pulses is low within the used buffer
  // Calculate triac time for the current cycle
  for (uint8_t i = 0; i < dimmerCount; i++) {
    if (dimmers[i]->pwmtimer->state() == STOPPED) { //timer has stopped for that Dimmer object, so we should trigger the Triac
      if (dimmers[i]->lampValue > 0 && dimmers[i]->lampState) {
        dimmers[i]->triggertimer->start();
//      digitalWrite(triacPin, !lampState);      
      }
    }

  // Increment the ramp counter until it reaches the total number of cycles for the ramp
//  if (operatingMode == DIMMER_RAMP && rampCounter < rampCycles) {
//    rampCounter++;
  }
}

void callTriac() {


}

// Constructor
Dimmer::Dimmer(uint8_t pin, uint8_t mode, double rampTime, uint8_t freq) :
  triacPin(pin),
  operatingMode(mode),
  lampState(false),
  lampValue(0),
  rampStartValue(0),
  rampCounter(1),
  rampCycles(1),
  acFreq(freq) {
    if (dimmerCount < DIMMER_MAX_TRIAC) {
    // Register dimmer object being created
    dimmerIndex = dimmerCount;
    dimmers[dimmerCount++] = this;
    triacPinPorts[dimmerIndex] = portOutputRegister(digitalPinToPort(pin));
    triacPinMasks[dimmerIndex] = digitalPinToBitMask(pin);
    if (mode == DIMMER_RAMP) {
      setRampTime(rampTime);
    }
  }
}

void Dimmer::begin(uint8_t value, bool on) {
  // Initialize lamp state and value
  set(value, on);
  pwmtimer = new Ticker(zeroCross, lampValue, 1, MILLIS);
  triggertimer = new Ticker(callTriac, DIMMER_TRIGGER, 1);

  // Initialize triac pin
  pinMode(triacPin, OUTPUT);
  digitalWrite(triacPin, HIGH);

  if (!started) {
    Serial.println("Dimmer::begin");
    // Start zero cross circuit if not started yet
    pinMode(DIMMER_ZERO_CROSS_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(DIMMER_ZERO_CROSS_PIN), callZeroCross, RISING);
    started = true;
  }
  Serial.println("Dimmer::begin finished");
}

void Dimmer::off() {
  lampState = false;
}

void Dimmer::on() {
  lampState = true;
}

void Dimmer::toggle() {
  lampState = !lampState;
}

bool Dimmer::getState() {
  return lampState;
}

uint8_t Dimmer::getValue() {
  return rampStartValue + ((int32_t) lampValue - rampStartValue) * rampCounter / rampCycles;
}

void Dimmer::set(uint8_t value) {
  if (value > 100) {
    value = 100;
  }

  if (value < minValue) {
    value = minValue;
  }
  if (value != lampValue) {
    if (operatingMode == DIMMER_RAMP) {
      rampStartValue = getValue();
      rampCounter = 0;
    } else if (operatingMode == DIMMER_COUNT) {

    }
    lampValue = value;
  }
}

void Dimmer::set(uint8_t value, bool on) {
  set(value);
  lampState = on;
}

void Dimmer::setMinimum(uint8_t value) {
  if (value > 100) {
    value = 100;
  }

  minValue = value;
  if (lampValue < minValue) {
    set(value);
  }
}

void Dimmer::setRampTime(double rampTime) {
  rampTime = rampTime * 2 * acFreq;
  rampCycles = rampTime > 0xFFFF ? 0xFFFF : rampTime;
  rampCounter = rampCycles;
}
