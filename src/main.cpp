#include "init.h"

const uint8_t Led1{16};
const int SW1{13};
const uint8_t GPIO4{4};
const uint8_t TRIACPIN{12};
bool status{false};
OneButton Sw1(SW1, true);
Dimmer dimmer(TRIACPIN);
void Handleswitch();
//void ICACHE_RAM_ATTR cross();

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(300);
  Serial.println("\r\nInitialising");
  pinMode(Led1, OUTPUT);
  Sw1.attachClick([](){Handleswitch();});
  dimmer.begin(100);
}

void loop() {
  // put your main code here, to run repeatedly:
  Sw1.tick();
}

void Handleswitch() {
  digitalWrite(Led1, status=!status);
  status?dimmer.on():dimmer.off();
  Serial.println(status);
}
