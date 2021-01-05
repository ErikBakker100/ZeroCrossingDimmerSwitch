#include "init.h"

const uint8_t Led1{16};
const int SW1{13};
const uint8_t GPIO4{4};
const uint8_t TRIACPIN{12};
bool status{false};
OneButton Sw1(SW1, true);
Dimmer dimmer(TRIACPIN, DIMMER_NORMAL, 2, 50);
void Handleswitch();

uint16_t remember{0};

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(300);
  Serial.println("\r\nInitialising");
  pinMode(Led1, OUTPUT);
  Sw1.attachClick([](){Handleswitch();});
  dimmer.begin(25);
  dimmer.setMinimum(10);
}

void loop() {
  // put your main code here, to run repeatedly:
  Sw1.tick();
/*  if (remember+5 <= analogRead(A0) || remember-5 >= analogRead(A0)){
    remember = analogRead(A0);
    dimmer.set(remember/10); 
  Serial.println(remember);
  }
*/  dimmer.update();
}

void Handleswitch() {
  status=!status;
  digitalWrite(Led1, status);
  dimmer.toggle();
}
