#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <Wire.h>
#include <wifisettings.h> //own Wifi-Settings

#ifndef STASSID
#error STASSID not set //remove if entered correct values

#define STASSID "..."
#define STAPSK  "..."
#endif

enum SLEEP_STATE {SLEEPSTATE_SLEEP_ALL = 0, SLEEPSTATE_SLEEP_AVR = 1 ,SLEEPSTATE_WAKE = 3};
enum KEYS {
  KEY_NONE = 0,
  
  KEY_ROW1_MINUS = 1, 
  KEY_ROW1_PLUS = 2,
  
  KEY_ROW2_MINUS = 3,
  KEY_ROW2_PLUS = 4,
  
  KEY_ROW3_SIZE1 = 17,
  KEY_ROW3_SIZE2 = 18,
  KEY_ROW3_SIZE3 = 19,
  KEY_ROW3_SIZE4 = 20,
  
  KEY_ROW4_IN = 33,
  KEY_ROW4_VENTILATOR = 34,
  KEY_ROW4_LEFT = 35,
  KEY_ROW4_RIGHT = 36,
  
  KEY_ROENTGEN = 7, 
};

const char* ssid = STASSID;
const char* password = STAPSK;

const int avr_irq = 14;
bool mdns_setup = false;

uint8_t key;
ESP8266WebServer server(80);
HTTPClient http;  //Declare an object of class HTTPClient

uint32_t lastkey = 0;
void handleRoot() {
  server.send(200, "text/plain", "CMD-Line Roentgen\n"); 
}

void setSleepState(int state) {
  Wire.begin();
  Wire.beginTransmission(0x50>>1);
  Wire.write(0);
  Wire.write(state);
  Wire.endTransmission();
}

void setLEDs(int leds) {
  Wire.beginTransmission(0x28);
  Wire.write(2);
  Wire.write(leds);
  Wire.endTransmission();
}

int checkKeys() {
  if(!digitalRead(avr_irq)) {
    Wire.beginTransmission(0x50>>1);
    Wire.write(1);
    Wire.endTransmission(false);
    Wire.requestFrom(0x50>>1,1);
    if(Wire.available()) {
      return Wire.read();
    }
  }
  return 0;
}

void setup() {
  
  //Disable Shutdown
  setSleepState(SLEEPSTATE_WAKE);
  
  pinMode(avr_irq, INPUT_PULLUP);
 
  //WiFi Interface
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  server.on("/", handleRoot);
  server.begin();
  ArduinoOTA.setHostname("roentgen");
  ArduinoOTA.begin();

  //Debug
  Serial.begin(115200);
  Serial.println("Start");
}
int delays = 0;
int httpCode = 0;

void loop() {
  ArduinoOTA.handle();
  server.handleClient();
  if(WiFi.status() == WL_CONNECTED && !mdns_setup) {
    mdns_setup = true;
    MDNS.begin("roentgen",WiFi.localIP());
    MDNS.addService("http", "tcp", 80);
    setLEDs(1);
  }
  else if(WiFi.status() == WL_CONNECTED) {
    MDNS.update();
  }

  if(mdns_setup) {
    key = checkKeys();
  }
  
  if(key) {
    Serial.println(key);
    switch(key & 0x3F) {
      case KEY_ROW4_LEFT:
      Serial.println("Left");
        http.begin("http://192.168.178.30/pos?-10");  //Specify request destination
        httpCode = http.GET();
        http.end();   //Close connection
      break;
      case KEY_ROW4_RIGHT: 
       http.begin("http://192.168.178.30/pos?10");  //Specify request destination
        httpCode = http.GET();
        http.end();   //Close connection
      break;
    }
    lastkey = millis();
  }

  
  if(millis()-lastkey > 5000) {
    setLEDs(0);
    setSleepState(SLEEPSTATE_SLEEP_ALL);
  }
 }
