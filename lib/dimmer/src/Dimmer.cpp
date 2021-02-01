/** @file
 * This is a library to control the intensity of dimmable AC lamps or other AC loads using triacs.
 *
 * Adapted from the original from 2015 Circuitar
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
  acFreq(freq)
 {
    if (dimmerCount < DIMMER_MAX_TRIAC) {
      // Register dimmer object being created
      dimmerIndex = dimmerCount;
      dimmers[dimmerCount++] = this;
      }
    setRampTime(rampTime);
    // Initialize triac pin
    pinMode(triacPin, OUTPUT);
    digitalWrite(triacPin, TRIAC_NORMAL_STATE); // Turn Triac off to start with
    pwmtimer = new Ticker(std::bind(&Dimmer::callTriac, this), 0, 1, MICROS_MICROS);
  }

void Dimmer::begin(uint8_t value) {
  // Initialize lamp state and value
  set(value);
  if (!started) {
    // Start zero cross circuit if not started yet
    pinMode(DIMMER_ZERO_CROSS_PIN, INPUT);
    enableinterrupt();
    started = true;
  }
  halfcycletime = 500000 / acFreq; // 1sec/freq/2 = 1000000usec/2/ freq
}

void Dimmer::off() {
  rampCounter = 0;
  rampEndValue = minValue;
  rampStartValue = maxValue;
  }

void Dimmer::on() {
  rampCounter = 0;
  rampEndValue = maxValue;
  rampStartValue = minValue;
  }
  
void Dimmer::toggle() {
  (lampValue>minValue)?off():on();
  }

bool Dimmer::getState() {
  if (lampValue) return true;
  return false;
  }

uint8_t Dimmer::value() {
  return lampValue;
}

uint8_t Dimmer::getValue() {
  if (rampStartValue < rampEndValue) return rampStartValue + ((int32_t) rampEndValue - rampStartValue) * ((float)rampCounter / rampCycles);
  return rampStartValue - ((int32_t) rampStartValue - rampEndValue) * ((float)rampCounter / rampCycles);
  }

void Dimmer::set(uint8_t value) {
  if (value > 100) value = 100;
  if (value < minValue) value = minValue;
  rampStartValue = getValue(); // We start from the current brightness
  maxValue = value; // We have a new max value
  rampEndValue = maxValue; // We should end with the new maxvalue
  rampCounter = 0;
  if (operatingMode == DIMMER_COUNT) {
    pulseCount = 0;
    }
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
  rampTime = rampTime * 2 * acFreq + 1;  // = keren dat de zero crossing in tijd moet worden doorlopen
  rampCycles = rampTime > 0xFFFF ? 0xFFFF : rampTime;
  rampCounter = 0;
  }

void Dimmer::update() {
  pwmtimer->update();
  }

void Dimmer::zeroCross() {
  digitalWrite(triacPin, !TRIAC_NORMAL_STATE); // Reset Triac gate
  lampValue = getValue();
  if (operatingMode == DIMMER_COUNT) {
    /* Dimmer Count mode Use count mode to switch the load on and off only when the AC voltage crosses zero. In this
      * mode, the power is controlled by turning the triac on only for complete (half) cycles of the AC
      * sine wave. The power delivery is adjusted by counting the number of cycles that are activated.
      * This helps controlling higher, slower response loads (e.g. resistances) without introducing
      * any triac switching noise on the line.
      */
    zcCounter++;
    if (lampValue > ((float)pulseCount*100 / zcCounter)) {
      // Turn dimmer on at zero crossing time
      callTriac();
      pulseCount++;
    }
    if (zcCounter > (99 + (lampValue/zcCounter))) {
      zcCounter = 0;
      pulseCount = 0;
    }
  }
  else {
    pwmtimer->stop();  
    triacTime = halfcycletime-(lampValue*halfcycletime/100); // Wait time before triggering the Triac
    pwmtimer->interval(triacTime);
    pwmtimer->start();
  }
  // Increment the ramp counter until it reaches the total number of cycles for the ramp
  if (rampCounter < rampCycles) rampCounter++;
}

void Dimmer::callTriac() {
  digitalWrite(triacPin, TRIAC_NORMAL_STATE);
  }

void Dimmer::disableinterrupt(){
  detachInterrupt(digitalPinToInterrupt(DIMMER_ZERO_CROSS_PIN));
}

void Dimmer::enableinterrupt(){
  attachInterrupt(digitalPinToInterrupt(DIMMER_ZERO_CROSS_PIN), callZeroCross, RISING);
}