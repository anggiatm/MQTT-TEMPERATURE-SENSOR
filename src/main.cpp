#include <Arduino.h>
#include <SPI.h>
#include <EthernetENC.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Update these with values suitable for your network.

// Device settings
// 192, 168, 3, 111 ahu34_temp1 0xC5, 0x09, 0x1C, 0x98, 0x79, 0x17
// 192, 168, 3, 112 ahu34_temp2 0xC5, 0x09, 0x1C, 0x98, 0x79, 0x18
// 192, 168, 3, 113 ahu34_temp3 0xC5, 0x09, 0x1C, 0x98, 0x79, 0x19
// 192, 168, 3, 114 ahu34_temp4 0xC5, 0x09, 0x1C, 0x98, 0x79, 0x1A
// 192, 168, 3, 115 ahu34_temp5 0xC5, 0x09, 0x1C, 0x98, 0x79, 0x1B
byte mac[]    = { 0xC5, 0x09, 0x1C, 0x98, 0x79, 0x1B };
IPAddress ip(192, 168, 3, 115);
IPAddress server(192, 168, 3, 50);

const char* deviceId     = "ahu34_temp5";
const char* stateTopic   = "home-assistant/ahu34_temp5";
const char* username     = "mqtt-user";
const char* password     = "testing";

const int updateInterval = 10000;

unsigned long now = 0;
unsigned long lastReconnectAttempt = 0;
unsigned long lastPublish = 0;
int connectingAttempt = 0;

OneWire  oneWire(5);
DallasTemperature sensors(&oneWire);
String temperature = "";
char buf[7];

void(* resetFunc) (void) = 0;

void callback(char* topic, byte* payload, unsigned int length) {
     Serial.print("Message arrived [");
     Serial.print(topic);
     Serial.print("] ");
     for (int i=0;i<length;i++) {
          Serial.print((char)payload[i]);
     }
     Serial.println();
}

EthernetClient ethClient;
PubSubClient client(server, 1883, callback, ethClient);

boolean reconnect() {
     if (client.connect(deviceId, username, password)) {
          Serial.println("Connected");
     }
     return client.connected();
}

void setup()
{
     Serial.begin(9600);

     client.setServer(server, 1883);
     client.setCallback(callback);

     Ethernet.begin(mac, ip);

     reconnect();

     sensors.begin();

     Serial.print("IP ADD: ");
     Serial.println(Ethernet.localIP());
     Serial.print("MAC ADD: ");
     for (int i = 0; i < sizeof(mac); i++ ){
          char hexCar[2];
          sprintf(hexCar, "%02X", mac[i]);
          Serial.print(hexCar);
          if (i != sizeof(mac)-1){
               Serial.print(":");
          }
     }
     Serial.println();
     Serial.print("DEVICE ID: ");
     Serial.println(deviceId);
     Serial.print("STATE TOPIC: ");
     Serial.println(stateTopic);
     Serial.println();

     now = 0;
     lastReconnectAttempt = 0;
     lastPublish = 0;
     connectingAttempt = 0;
     delay(1500);
}

void loop(){
     now = millis();
     if (!client.connected()) {
          if (now - lastReconnectAttempt > 5000) {
               lastReconnectAttempt = now;
               connectingAttempt += 1;
               if (connectingAttempt >= 10){
                    resetFunc();
               }
               if (reconnect()) {
                    lastReconnectAttempt = 0;
                    connectingAttempt = 0;
               }
          }
     } else {
          // Client connected
          client.loop();
          if (now - lastPublish > updateInterval) {
               sensors.requestTemperatures();
               temperature = String(sensors.getTempCByIndex(0));
               temperature.toCharArray(buf, 7);
               Serial.print("Temperature: ");
               Serial.println(temperature);
               client.publish(stateTopic, buf);
               lastPublish = now;
          }
     }
}