#include "init.h"

const uint8_t Led1{16};
const int SW1{13};
const uint8_t GPIO4{4}; //Triggerpin
const uint8_t PULSPIN{14};
bool status{false};
OneButton Sw1(SW1, true);
Ticker* puls{nullptr};
void pulsend();
void Handleswitch();
void callZeroCross();
static volatile uint32_t sincelastCrossing{0}; // Record how many millis(0 have passed since last Zero Crossing detection, used for debouncing zero crossing circuit and to calculate when triac needs to be fired.
static bool zerocrossiscalled{false}; // If zerocross is detected and handled the first time we should not handle again till next crossing

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(300);
  Serial.println("\r\nInitialising");
  pinMode(Led1, OUTPUT);
  Sw1.attachClick([](){Handleswitch();});
  // Start zero cross circuit if not started yet
  pinMode(GPIO4, INPUT); //Zero Cross input
  pinMode(PULSPIN, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(GPIO4), callZeroCross, RISING);
  puls = new Ticker(pulsend, 25, 1, MICROS_MICROS);
}

void loop() {
  // put your main code here, to run repeatedly:
  Sw1.tick();
  puls->update();
}

void Handleswitch() {
  status=!status;
  digitalWrite(Led1, status);
  Serial.println(status);
}

// Zero cross interrupt
void ICACHE_RAM_ATTR callZeroCross() {
  if(micros() - sincelastCrossing > 5000) {// It can not be a bounce because of the amount of time passed, this must be the first time
  zerocrossiscalled = false;
  sincelastCrossing = micros();  }
  if(zerocrossiscalled) return; // If we have run this IRS before it must be a bounce
  zerocrossiscalled = true; // Remember that we have run this ISR before
  puls->start();
  digitalWrite(PULSPIN, HIGH);
}

void pulsend(){
  digitalWrite(PULSPIN, LOW);
}