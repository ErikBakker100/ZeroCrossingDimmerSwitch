
#pragma once

#include <Arduino.h>
#include <OneButton.h>
#include <ESP8266WiFi.h>
#include "Dimmer.h"
#include "PubSubClient.h"
#include "Json.h"
#include "Receive.h"

#ifndef DEBUGGING
#define DEBUGGING(...) Serial.println( __VA_ARGS__ )
#endif
#ifndef DEBUGGING_L
#define DEBUGGING_L(...) Serial.print( __VA_ARGS__ )
#endif


/**********************************************************/
//Function Definitions

/**********************************************************/
