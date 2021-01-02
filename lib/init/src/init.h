
#pragma once

#include <Arduino.h>
#include <OneButton.h>
#include "Ticker.h"

#ifndef DEBUGGING
#define DEBUGGING(...) Serial.println( __VA_ARGS__ )
#endif
#ifndef DEBUGGING_L
#define DEBUGGING_L(...) Serial.print( __VA_ARGS__ )
#endif


/**********************************************************/
//Function Definitions

/**********************************************************/
