#include "init.h"

const uint8_t Led1{16};
const int SW1{13};
const uint8_t GPIO4{4};
const uint8_t TRIACPIN{12};
bool status{false};
OneButton Sw1(SW1, true);
void Handleswitch();
volatile uint32_t* PinPort;
uint16_t PinMask;
void printbits(uint16_t);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(300);
  Serial.println("\r\nInitialising");
  pinMode(Led1, OUTPUT);
  Sw1.attachClick([](){Handleswitch();});
  for(auto i=0; i<16; i++) { 
    Serial.printf("Portnr = %u, Portadres = 0x%08x ", digitalPinToPort(i), (uint32_t)portOutputRegister(digitalPinToPort(i)));
    PinMask = digitalPinToBitMask(i);
    Serial.printf("Mask = 0x%0x", PinMask);
    printbits(PinMask);
    Serial.println();
  }
  digitalWrite(Led1, HIGH);
  PinMask = digitalPinToBitMask(Led1);
  Serial.printf("Mask = 0x%0x\n", PinMask);
  PinPort = portOutputRegister(digitalPinToPort(Led1)); // digitalPinToPort retuns the port number 'pin' lives is. portOutputRegister returns the address of that specific port
  Serial.printf("Portadres van %u = 0x%x\n", Led1, PinPort);
  printbits(*PinPort);
  Serial.printf(" Portinhoud = 0x%x\n", *PinPort);
  digitalWrite(Led1, LOW);
  printbits(*PinPort);
  Serial.printf(" Portinhoud is nu = 0x%x\n", *PinPort);
}

void loop() {
  // put your main code here, to run repeatedly:
  Sw1.tick();
}

void Handleswitch() {
  status=!status;
  volatile uint32_t m = micros();
  digitalWrite(Led1, status);
  Serial.printf("dww %u\n", micros()-m);
  m = micros();
  status?(*PinPort |= PinMask):*PinPort &= ~PinMask; // Reset Triac gate
  Serial.printf("dwb %u\n", micros()-m);
  Serial.println(status);
}

void printbits(uint16_t x) {
  for(auto i=sizeof(x)<<3; i; i--) putchar('0' + ((x>>(i-1))&1));
}