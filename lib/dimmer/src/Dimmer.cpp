/** @file
 * This is a library to control the intensity of dimmable AC lamps or other AC loads using triacs.
 *
 * Copyright (c) 2015 Circuitar
 * This software is released under the MIT license. See the attached LICENSE file for details.
 */

#include "Arduino.h"
#include "Dimmer.h"

// Time table to set triacs (50us tick)
//   This is the number of ticks after zero crossing that we should wait before activating the
//   triac, where the array index is the applied power from 1 to 100, minus one. This array is
//   calculated based on the equation of the effective power applied to a resistive load over
//   time, assuming a 60HZ sinusoidal wave.
static const uint8_t powerToTicks[] PROGMEM = {
  147, 142, 139, 136, 133, 131, 129, 127, 125, 124,
  122, 121, 119, 118, 116, 115, 114, 113, 112, 111,
  110, 108, 107, 106, 105, 104, 103, 102, 102, 101,
  100,  99,  98,  97,  96,  95,  94,  93,  93,  92,
   91,  90,  89,  88,  88,  87,  86,  85,  84,  83,
   83,  82,  81,  80,  79,  78,  77,  77,  76,  75,
   74,  73,  72,  71,  71,  70,  69,  68,  67,  66,
   65,  64,  63,  62,  61,  60,  59,  58,  57,  56,
   55,  54,  53,  51,  50,  49,  48,  46,  45,  43,
   42,  40,  38,  36,  34,  31,  28,  24,  19,  0
};

// Dimmer registry
static Dimmer* dimmmers[DIMMER_MAX_TRIAC];     // Pointers to all registered dimmer objects
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
      dimmmers[i]->zeroCross();

      // If triac time was already reached, activate it
      if (sincelastCrossing >= triacTimes[i]) {
      *triacPinPorts[i] |= triacPinMasks[i];
      }
    }
  }
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
  pulsesHigh(0),
  pulsesLow(0),
  pulsesUsed(0),
  acFreq(freq) {
    if (dimmerCount < DIMMER_MAX_TRIAC) {
    // Register dimmer object being created
    dimmerIndex = dimmerCount;
    dimmmers[dimmerCount++] = this;
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
      pulsesHigh = 0;
      pulsesLow = 0;
      pulseCount = 0;
      pulsesUsed = 0;
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

void Dimmer::zeroCross() {
  // Turn next half cycle on if number of pulses is low within the used buffer
  // Calculate triac time for the current cycle
  uint8_t value = getValue();
  if (value > 0 && lampState) {
      digitalWrite(triacPin, !lampState);
  }
  // Increment the ramp counter until it reaches the total number of cycles for the ramp
  if (operatingMode == DIMMER_RAMP && rampCounter < rampCycles) {
    rampCounter++;
  }
}
