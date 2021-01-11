#include "init.h"

const uint8_t Led1{16};
const int SW1{13};
const uint8_t GPIO4{4};
const uint8_t TRIACPIN{12};
bool status{false};
OneButton Sw1(SW1, true);
Dimmer dimmer(TRIACPIN, DIMMER_NORMAL, 2, 50);
void Handleswitch();

// Make sure to update this for your own WiFi network!
const char *ssid{"HG655D-14581D"};
const char *wifi_password{"ULW397E9"};
// Make sure to update this for your own MQTT Broker!
const char *mqtt_server{"192.168.1.21"};
const char *mqtt_topic_out{"domoticz/out"};
const char *mqtt_topic_in{"domoticz/in"};
const char *mqtt_username{"mqtt"};
const char *mqtt_password{"1Edereen"};
// The client id identifies the ESP8266 device. Think of it a bit like a hostname (Or just a name, like Erik).
const char *clientID{"Dimmer"};
// Identifier of the device in Domoticz
const uint16_t idx_dimmer{1385}; // Switch IDX generated bij Domoticz
// Port to which MQTT listens
const int mqtt_port{1883};

// Do not modify anything below this point.
// Initialise the WiFi and MQTT Client objects
WiFiClient wifiClient;
PubSubClient client(mqtt_server, mqtt_port, wifiClient);
void callback(const char *, uint8_t *, unsigned int); // to call when something via MQTT has been received
void Connect(); // Declaration to function to Initialise WiFi and MQTT

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(300);
  Serial.println("\r\nInitialising");
  pinMode(Led1, OUTPUT);
  Sw1.attachClick([](){Handleswitch();});
  dimmer.begin(25);
  dimmer.setMinimum(10);
  client.setServer(mqtt_server, mqtt_port);
  Serial.println("MQTT server set.");
  client.setCallback(callback);
  Serial.println("Callback function initialized");
}

void loop() {
  // put your main code here, to run repeatedly:
  Connect(); // Check if all connections are OK before going on
  client.loop(); // See if any command is recieved from MQTT

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

void callback(const char *topic, uint8_t *payload, unsigned int length)
{
#ifdef DEBUG
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
#endif
    String message;
    for (unsigned int i = 0; i < length; i++)
    {
        message += (char)payload[i];
    }
#ifdef DEBUG
    Serial.println(message);
    Serial.println();
#endif
    Json json;
    if (json.readJson(message))
    {
        Serial.print("IDX received : ");
        Serial.println(json.getidx());
        switch (json.getidx())
        {
            case idx_dimmer:
            if (json.getcommand() == "getdeviceinfo")
            {
                decode();
            }
            break;
        }
    }
}

void Connect()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.print("Connecting to ");
        Serial.println(ssid);
        // Connect to the WiFi
        WiFi.begin(ssid, wifi_password);
        while (WiFi.status() != WL_CONNECTED)
        {
            delay(500);
            Serial.print(".");
        }
        // Debugging - Output the IP Address of the ESP8266
        Serial.println("WiFi connected");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        // Signaalsterkte.
        long rssi = WiFi.RSSI();
        Serial.print("Signaal sterkte (RSSI): ");
        Serial.print(rssi);
        Serial.println(" dBm");
    }
    if (!client.connected())
    {
        Serial.print("connecting to MQTT :");
        while (!client.connect(clientID, mqtt_username, mqtt_password))
        {
            Serial.print(".");
        }
        Serial.println(" Connected to MQTT Broker!");
        while (!client.subscribe(mqtt_topic_out))
        {
            client.loop();
            delay(100);
        }
        Serial.print("Listening to topic : ");
        Serial.println(mqtt_topic_out);
    }
}
