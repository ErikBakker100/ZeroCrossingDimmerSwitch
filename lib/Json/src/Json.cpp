#include "Json.h"

// See for explanation of controlling Domoticz via MQTT : https://www.domoticz.com/wiki/MQTT#Update_devices.2Fsensors
// for example : {"command": "switchlight", "idx": 2450, "switchcmd": "On" }

Json::Json() {}

String Json::switchlight(bool cmd)
{
    String json_tekst;
    StaticJsonDocument<1500> jsonBuffer;
    JsonObject root = jsonBuffer.createNestedObject();
    root["command"] = "switchlight";
    root["idx"] = idx;
    root["switchcmd"] = cmd?"On":"Off";
    serializeJson(root, json_tekst);
    return json_tekst;
}

String Json::udevice(uint16_t idx, const float nvalue, const std::vector<float>* svalue) {
    String json_tekst;
    StaticJsonDocument<1500> jsonBuffer;
    JsonObject root = jsonBuffer.createNestedObject();
    root["command"] = "udevice";
    root["idx"] = idx;
    root["nvalue"] = nvalue;
    String temp = String(svalue->at(0));
    for (uint8_t x = 1; x < svalue->size(); x++) {
        temp += ";";
        temp += svalue->at(x);
        }
    root["svalue"] = temp;
        serializeJson(root, json_tekst);
    return json_tekst;
}
bool Json::readJson(String my_string) {
    return readJson(my_string.c_str());
}

bool Json::readJson(unsigned char *my_string) {
    idx = 0;
    nvalue = 0;
    svalue = 0;
    command = "";
    StaticJsonDocument<JSON_OBJECT_SIZE(15)> jsonBuffer;
    DeserializationError err = deserializeJson(jsonBuffer, my_string);
    if (err) {
        Serial.print("json parseObject() failed wih code ");
        Serial.println(err.c_str());
        return false;
    }
    if (jsonBuffer.containsKey("command")) { 
        const char* Command = jsonBuffer["command"];
        command = String(Command);
    }
    if (jsonBuffer.containsKey("idx")) idx = (uint16_t)jsonBuffer["idx"];
    if (jsonBuffer.containsKey("nvalue")) nvalue = (float)jsonBuffer["nvalue"];
    if (jsonBuffer.containsKey("svalue")) svalue = (float)jsonBuffer["svalue"];
    if (jsonBuffer.containsKey("svalue1")) svalue1 = (float)jsonBuffer["svalue1"];
    return true;
}

float Json::getnvalue() {
    return nvalue;
}

float Json::getsvalue() {
    return svalue;
}

float Json::getsvalue1() {
    return svalue1;
}

uint16_t Json::getidx() {
    return idx;
}

String Json::getcommand() {
    return command;
}