#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
//#include <vector>

static const int BUFFERSIZE{350}; // json default buffer size

class Json {
  public:
    Json();
    String addlogmessage();
    String switchlight(const bool);
    String getdeviceinfo();
    String getsceneinfo();
    String sendnotification();
    String setuservariable();
    String setcolbrightnessvalue();
    String switchlight();
    String switchscene();
    String udevice(const uint16_t, const float, const std::vector<float>*);
    bool readJson(const String my_string);
    float getnvalue();
    std::vector<float> getsvalue();
    uint16_t getidx();
    String getcommand();

  private : 
    uint16_t idx;
    float nvalue;
    float svalue;
    String command;
};