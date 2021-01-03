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
static uint8_t dimmerCount{0};                // Number of registered dimmer objects

// Global state variables
bool Dimmer::started{false}; // At least one dimmer has started
static volatile uint32_t sincelastCrossing{0}; // Record how many micros have passed since last Zero Crossing detection, used for debouncing zero crossing circuit and to calculate when triac needs to be fired.
static bool zerocrossiscalled{false}; // If zerocross is detected and handled the first time we should not handle again till next crossing

// Zero cross interrupt
void ICACHE_RAM_ATTR callZeroCross() {
  if(micros() - sincelastCrossing > 5000) {// It can not be a bounce because of the amount of time passed, this must be the first time
    zerocrossiscalled = false;
    sincelastCrossing = micros();  }
  if(zerocrossiscalled) return; // If we have run this IRS before it must be a bounce
  zerocrossiscalled = true; // Remember that we have run this ISR before
  // Process each registered dimmer object
  for (uint8_t i = 0; i < dimmerCount; i++) {
    dimmers[i]->zeroCross();
  }
}

// Constructor
Dimmer::Dimmer(uint8_t pin, uint8_t mode, double rampTime, uint8_t freq) :
  triacPin(pin),
  operatingMode(mode),
  lampValue(0),
  maxValue(100),
  rampStartValue(0),
  rampCounter(1),
  rampCycles(1),
  acFreq(freq),
  pulseCount(0),
  pulsesUsed(0),
  pulsesHigh(0),
  pulsesLow(0)
 {
    if (dimmerCount < DIMMER_MAX_TRIAC) {
      // Register dimmer object being created
      dimmerIndex = dimmerCount;
      dimmers[dimmerCount++] = this;
    }
    if (mode == DIMMER_RAMP) {
      setRampTime(rampTime);
    }
  }

void Dimmer::begin(uint8_t value) {
  // Initialize triac pin
  pinMode(triacPin, OUTPUT);
  digitalWrite(triacPin, TRIAC_NORMAL_STATE); // Turn Triac off to start with
  // Initialize lamp state and value
  set(value);
  pwmtimer = new Ticker(std::bind(&Dimmer::callTriac, this), 0, 1, MICROS_MICROS);

  if (!started) {
    Serial.println("Dimmer::begin");
    // Start zero cross circuit if not started yet
    pinMode(DIMMER_ZERO_CROSS_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(DIMMER_ZERO_CROSS_PIN), callZeroCross, RISING);
    started = true;
  }
  halfcycletime = 500000 / acFreq; // 1sec/freq/2 = 1000000usec/2/ freq
  Serial.println("Dimmer::begin finished");
}

void Dimmer::off() {
  rampCounter = 0;
  lampValue = minValue;
  }

void Dimmer::on() {
  rampCounter = 0;
  lampValue = maxValue;
  }

void Dimmer::toggle() {
  (lampValue == maxValue)?off():on();
  }

bool Dimmer::getState() {
  if (lampValue) return true;
  return false;
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
  }
  maxValue = value;
  lampValue = maxValue;
}

void Dimmer::setMinimum(uint8_t value) {
  if (value > 100) {
    value = 100;
    }
  minValue = value;
  if (maxValue < minValue) {
    set(value);
    }
  }

void Dimmer::setRampTime(double rampTime) {
  rampTime = rampTime * 2 * acFreq;
  rampCycles = rampTime > 0xFFFF ? 0xFFFF : rampTime;
  rampCounter = rampCycles;
  }

void Dimmer::update() {
  pwmtimer->update();
  }

void ICACHE_RAM_ATTR Dimmer::zeroCross() {
  digitalWrite(triacPin, !TRIAC_NORMAL_STATE); // Reset Triac gate
  pwmtimer->stop();  
  // can be called by zero crossing detector.
  if (operatingMode == DIMMER_COUNT) {
    /* Dimmer Count mode Use count mode to switch the load on and off only when the AC voltage crosses zero. In this
      * mode, the power is controlled by turning the triac on only for complete (half) cycles of the AC
      * sine wave. The power delivery is adjusted by counting the number of cycles that are activated.
      * This helps controlling higher, slower response loads (e.g. resistances) without introducing
      * any triac switching noise on the line.
      */
   // Remove MSB from buffer and decrement pulse count accordingly
    if (pulseCount > 0 && (pulsesHigh & (1ULL << 35))) {
      pulsesHigh &= ~(1ULL << 35);
      if (pulseCount > 0) {
        pulseCount--;
      }
    }

    // Shift 100-bit buffer to the right
    pulsesHigh <<= 1;
    if (pulsesLow & (1ULL << 63)) {
      pulsesHigh++;
    }
    pulsesLow <<= 1;

    // Turn next half cycle on if number of pulses is low within the used buffer
    if (lampValue > ((uint16_t)(pulseCount) * 100 / (pulsesUsed))) {  
      // Turn dimmer on at zero crossing time, @10 ticks (500us)
      triacTime = 10;
      pulsesLow++;
      pulseCount++;
    }

    // Update number of bits used in the buffer
    if (pulsesUsed < 100) {
      pulsesUsed++;
    }

  } else {
    // Calculate triac time for the current cycle
    uint8_t value = getValue();
    if (value) {
      triacTime = halfcycletime-(value*halfcycletime/100); // Wait time before triggering the Triac
      pwmtimer->interval(triacTime);
      pwmtimer->start();
    }
    // Increment the ramp counter until it reaches the total number of cycles for the ramp
    if (operatingMode == DIMMER_RAMP && rampCounter < rampCycles) {
      rampCounter++;
    }
  }
}

void Dimmer::callTriac() {
  digitalWrite(triacPin, TRIAC_NORMAL_STATE); // Reset Triac gate
  }
