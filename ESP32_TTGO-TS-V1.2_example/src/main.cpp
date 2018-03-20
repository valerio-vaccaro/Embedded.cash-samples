const char firmware_name[]     = "Embedded.cash-test";
const char firmware_version[]  = "0.0.1";
const char source_filename[] = __FILE__;
const char compile_date[] = __DATE__ " " __TIME__;

#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <SPIFFS.h>
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson

#if defined(ESP8266)
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#else
#include <WiFi.h>          //https://github.com/esp8266/Arduino
#endif

//needed for library
#include <DNSServer.h>
#if defined(ESP8266)
#include <ESP8266WebServer.h>
#else
#include <WebServer.h>
#endif
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#define TFT_CS 16
#define TFT_RST 9
#define TFT_DC 17
#define TFT_SCLK 5
#define TFT_MOSI 23
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

#include "notes.h"
#include "logos.h"
#include "qrcode.h"
#include "free_fonts.h"

// MQTT
#include <PubSubClient.h>
#define SERVER      "vaccaro.tech"
#define SERVERPORT  1883
#define USERNAME    "guest"
#define KEY         "guest"
char topic_deploy_send[100];
char topic_sensor_send[100];
char topic_sensor_receive[100];
char topic_update_receive[100];
// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;
PubSubClient mqttclient(client);

//define your default values here, if there are different values in config.json, they are overwritten.
char mqtt_server[40];
char mqtt_port[6] = "8080";
char blynk_token[34] = "YOUR_BLYNK_TOKEN";

//flag for saving data
bool shouldSaveConfig = false;

String UID;

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void drawBitmap(int16_t x, int16_t y,
  const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color) {
  int16_t i, j, byteWidth = (w + 7) / 8;
  uint8_t byte;
  for(j=0; j<h; j++) {
    for(i=0; i<w; i++) {
      if(i & 7) byte <<= 1;
      else      byte   = pgm_read_byte(bitmap + j * byteWidth + i / 8);
      if(byte & 0x80) tft.drawPixel(x+i, y+j, color);
    }
  }
}

/***
  Update firmware function
***/
void updateFirmware() {
  Serial.println(F("Update command received"));

}

void reconnect() {
  // Loop until we're reconnected
  while (!mqttclient.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqttclient.connect("ESP8266Client2")) {
      Serial.println("connected");
      // esubscribe
      mqttclient.subscribe(topic_sensor_receive);
      mqttclient.subscribe(topic_update_receive);
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttclient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

/***
  MQTT receiver callback
***/
void mqttReceiveCallback(char* topic, byte* payload, unsigned int length) {
  Serial.println(F("=================="));
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  Serial.println(F("=================="));
  Serial.println(F("Kind of message:"));

  /* update message */
  if (strcmp(topic, topic_update_receive) == 0) {
    updateFirmware();
  }
  /* received message */
  else if (strcmp(topic, topic_sensor_receive) == 0) {
    /* update leds */
     if (((char)payload[0]=='O') && ((char)payload[1]=='K')) {
        Serial.println("OK");

        // Create JSON message
        StaticJsonBuffer<500> jsonBuffer2;
        char sens_buff2[500];
        JsonObject& root2 = jsonBuffer2.createObject();
        root2["Type"] = firmware_name;
        root2["Version"] = firmware_version;
        root2["ChipId"] = UID;
        JsonObject& data = root2.createNestedObject("d");
        data["Result"] = "Ok";
        data["Battery"] = (unsigned int) analogRead(A0);
        root2.printTo(sens_buff2, 500);
        Serial.print(F("Topic: "));
        Serial.println(topic_sensor_send);
        Serial.print(F("Message: "));
        Serial.println(sens_buff2);
        if (! mqttclient.publish(topic_sensor_send, sens_buff2)) {
            Serial.println(F("Send: Failed"));
        } else {
            Serial.println(F("Send: OK!"));
        }
      }
      else {
        Serial.println("Not OK");


        // Create JSON message
        StaticJsonBuffer<500> jsonBuffer2;
        char sens_buff2[500];
        JsonObject& root2 = jsonBuffer2.createObject();
        root2["Type"] = firmware_name;
        root2["Version"] = firmware_version;
        root2["ChipId"] = UID;
        JsonObject& data = root2.createNestedObject("d");
        data["Result"] = "Not Ok";
        data["Battery"] = (unsigned int) analogRead(A0);
        root2.printTo(sens_buff2, 500);
        Serial.print(F("Topic: "));
        Serial.println(topic_sensor_send);
        Serial.print(F("Message: "));
        Serial.println(sens_buff2);
        if (! mqttclient.publish(topic_sensor_send, sens_buff2)) {
            Serial.println(F("Send: Failed"));
        } else {
            Serial.println(F("Send: OK!"));
        }

      }

  }
  /*default*/
  else {

  }

}



void setup() {
  unsigned long long1 = (unsigned long)((ESP.getEfuseMac() & 0xFFFF00000000) >> 32 );
  unsigned long long2 = (unsigned long)((ESP.getEfuseMac() & 0x0000FFFF0000) >> 16 );
  unsigned long long3 = (unsigned long)((ESP.getEfuseMac() & 0x00000000FFFF));
  UID = String(long1, HEX) + String(long2, HEX) + String(long3, HEX);
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println(UID);
  // Use this initializer if you're using a 1.8" TFT
  tft.initR(INITR_BLACKTAB);   // initialize a ST7735S chip, black tab
  tft.fillScreen(ST7735_BLACK);
  delay(500);
  //clean FS, for testing //SPIFFS.format();
  //read configuration from FS json
  Serial.println("mounting FS...");
  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);
        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("\nparsed json");
          strcpy(mqtt_server, json["mqtt_server"]);
          strcpy(mqtt_port, json["mqtt_port"]);
          strcpy(blynk_token, json["blynk_token"]);
        } else {
          Serial.println("failed to load json config");
        }
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
  //end read

  // The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  // id/name placeholder/prompt default length
  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 6);
  WiFiManagerParameter custom_blynk_token("blynk", "blynk token", blynk_token, 32);

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  //set static ip
  //wifiManager.setSTAStaticIPConfig(IPAddress(10,0,1,99), IPAddress(10,0,1,1), IPAddress(255,255,255,0));
  //add all your parameters here
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_blynk_token);

  //reset settings - for testing
  //wifiManager.resetSettings();

  //set minimu quality of signal so it ignores AP's under that quality
  //defaults to 8%
  //wifiManager.setMinimumSignalQuality();

  //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep
  //in seconds
  //wifiManager.setTimeout(120);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect("Embedded.cash", "satoshi")) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.restart();
    delay(5000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");
  tft.fillScreen(ST7735_BLACK);
  tft.setCursor(0, 0);
  tft.setTextColor(ST7735_WHITE);
  tft.setTextWrap(true);
  tft.print("\n connected...yeey :)");
  tft.setTextColor(ST7735_GREEN);
  tft.print("\n HELLO");
  delay(1000);

  //read updated parameters
  strcpy(mqtt_server, custom_mqtt_server.getValue());
  strcpy(mqtt_port, custom_mqtt_port.getValue());
  strcpy(blynk_token, custom_blynk_token.getValue());
  //save the custom parameters to FS
  if (shouldSaveConfig) {
    Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["mqtt_server"] = mqtt_server;
    json["mqtt_port"] = mqtt_port;
    json["blynk_token"] = blynk_token;
    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }
    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
    //end save
  }
  Serial.println("local ip");
  Serial.println(WiFi.localIP());
  // mqtt_port
  // MQTT
  mqttclient.setServer(SERVER, SERVERPORT);
  mqttclient.setCallback(mqttReceiveCallback);
  String buf0 = String("iot/T/esp8266/I/") + UID + String("/D/sensor/F/json");
  buf0.toCharArray(topic_sensor_send, 100);
  String buf1 = String("iot/T/esp8266/I/") + UID + String("/D/deploy/F/json");
  buf1.toCharArray(topic_deploy_send, 100);
  String buf2 = String("iot/T/esp8266/I/") + UID + String("/C/sensor/F/json");
  buf2.toCharArray(topic_sensor_receive, 100);
  String buf3 = String("iot/T/esp8266/I/") + UID + String("/C/update/F/json");
  buf3.toCharArray(topic_update_receive, 100);

  if (! mqttclient.connected() ) {
    reconnect();
  }
  // Create JSON message
  StaticJsonBuffer<500> jsonBuffer;
  char sens_buff[500];
  JsonObject& root = jsonBuffer.createObject();
  root["Type"] = firmware_name;
  root["Version"] = firmware_version;
  root["Filename"] = source_filename;
  root["CompilationTime"] = compile_date;
  root["ChipId"] = UID;
  root["Deploy"] = topic_deploy_send;
  root["Update"] = topic_update_receive;
  JsonArray& Pub = root.createNestedArray("Pub");
  Pub.add(topic_sensor_receive);
  JsonArray& Sub = root.createNestedArray("Sub");
  Sub.add(topic_sensor_send);
  root.printTo(sens_buff, 500);
  Serial.print(F("Topic: "));
  Serial.println(topic_deploy_send);
  Serial.print(F("Message: "));
  Serial.println(sens_buff);
  if (! mqttclient.publish(topic_deploy_send, sens_buff)) {
    Serial.println(F("Send: Failed"));
  } else {
    Serial.println(F("Send: OK!"));
  }


}


void loop() {
  drawBitmap(20,40,btcLogo,85,85,ST7735_WHITE);
  delay(1000);
}
