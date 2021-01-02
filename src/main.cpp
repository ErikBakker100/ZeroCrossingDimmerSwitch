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
//void ICACHE_RAM_ATTR cross();

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
  puls = new Ticker(pulsend, 100, 1, MICROS_MICROS);
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
  puls->start();
  digitalWrite(PULSPIN, HIGH);
}

void pulsend(){
  digitalWrite(PULSPIN, LOW);
}