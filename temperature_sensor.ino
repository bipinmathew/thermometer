
/*
 *  This sketch demonstrates how to scan WiFi networks. 
 *  The API is almost the same as with the WiFi Shield library, 
 *  the most obvious difference being the different file you need to include:
 */
#include <FS.h> 
#include <PubSubClient.h>
#include <ESP8266WiFi.h>          //ESP8266 Core WiFi Library (you most likely already have this in your sketch)

#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson

#include "DHT.h"

#define DHTPIN 4     // what digital pin the DHT22 is conected to
#define DHTTYPE DHT22   // there are multiple kinds of DHT sensors
DHT dht(DHTPIN, DHTTYPE);
 

char hub[80]      = "bmathew-desktop";              // website
const int hub_port = 1883;

char label[80]    = "test";
//flag for saving data
bool shouldSaveConfig = false;
WiFiClient client;

char payload[1024];
PubSubClient mqtt_client;
byte mac[6];
char mac_string[18];


int timeSinceLastRead = 0;
const unsigned long long_press_time = 5000; // is millis returning 0.1ms?

// Use GPIO pin 5. 
// Note that on NodeMCU this should be D5.

static const int inputPin = 5;
float temperature;
char temperature_string[6];
char humidity_string[6];

bool long_press_flag=0;
unsigned long press_time=0;
unsigned long release_time=0;

// The amount of time (in milliseconds) to wait
#define TEST_DELAY   2000


//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}


void press(){
  press_time = millis();
}
void release(){
  release_time = millis();
  if((release_time-press_time)> long_press_time ){
    Serial.print("Time pressed");
    Serial.println(release_time-press_time);
    long_press_flag=1;
    press_time=0;
    release_time=0;
  }
}

WiFiManagerParameter custom_hub_text("<p>Hub URL</p>");
WiFiManagerParameter custom_hub("hub", "hub url", hub, 80);

WiFiManagerParameter custom_label_text("<p>Sensor Label</p>");
WiFiManagerParameter custom_label("label", "label", label, 80);



bool broadcast( PubSubClient *mqtt_client, const char* label, const char* payload) {
  return (mqtt_client->publish(label,payload) == false);
}


void setup() {
  Serial.begin(115200);
  Serial.println("Setup starting");


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

          strcpy(hub, json["hub"]);
          strcpy(label, json["label"]);
        

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
  WiFiManagerParameter custom_hub("hub", "hub", hub, 80);
  WiFiManagerParameter custom_label("label", "label", label, 80);

  
  WiFiManager wifiManager;

  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  //set static ip
  //wifiManager.setSTAStaticIPConfig(IPAddress(10,0,1,99), IPAddress(10,0,1,1), IPAddress(255,255,255,0));
  
  //add all your parameters here
  wifiManager.addParameter(&custom_hub_text);
  wifiManager.addParameter(&custom_hub);

  wifiManager.addParameter(&custom_label_text);
  wifiManager.addParameter(&custom_label);


  //TODO add some error checking around here...
  wifiManager.autoConnect("SensorNetwork", "BkBoy#1980");

   //read updated parameters
  strcpy(hub, custom_hub.getValue());
  strcpy(label, custom_label.getValue());

  //save the custom parameters to FS
  if (shouldSaveConfig) {
    Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["hub"] = hub;
    json["label"] = label;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
    //end save
  }

  WiFi.macAddress(mac);
  snprintf(mac_string,18,"%2x:%2x:%2x:%2x:%2x:%2x",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
  Serial.println("WiFi Mac Address: ");
  Serial.println(mac_string);
  Serial.println();
  
  mqtt_client.setClient(client);
  mqtt_client.setServer(hub,hub_port);
  
  temperature = 0.0;
  pinMode(inputPin,INPUT_PULLUP);
  attachInterrupt(inputPin,press,FALLING);
  attachInterrupt(inputPin,release,RISING);
  

  Serial.println("Setup done");
}

void loop() {   
  
  if(long_press_flag){
    long_press_flag=0;
    Serial.println("long press detected");
    WiFi.disconnect(true);
    ESP.restart();
    }
    else if(timeSinceLastRead>2000){
      timeSinceLastRead=0;
      while(!mqtt_client.connected()){
        if(mqtt_client.connect(mac_string)){
          Serial.println("...connected");
        }
        else{
          Serial.println("... no connect trying again");
          delay(5000);
        }
      }
      mqtt_client.loop();

      float h = dht.readHumidity();
      // Read temperature as Celsius (the default)
      float t = dht.readTemperature();
      

      // Check if any reads failed and exit early (to try again).
      if (isnan(h) || isnan(t)) {
        Serial.println("Failed to read from DHT sensor!");
        snprintf(payload,1024,"{\"status\": %d}",1);
        return;
      }
      else{
        Serial.print("Temperature: ");
        Serial.print(t);
        Serial.print(" (C)");
        Serial.println();

        // this is the internet bit
        dtostrf(t, 4, 2, temperature_string);
        dtostrf(h, 4, 2, humidity_string);

        snprintf(payload,1024,"{\"status\": %d, \"temperature\": %s, \"humidity\": %s}",0,temperature_string,humidity_string);

      }



      
      if(broadcast(&mqtt_client, label,payload)){
        Serial.print("mqtt publish failure :");
        Serial.print(mqtt_client.state());
        Serial.println();
      }

      
  }
  else{
    delay(100);
    timeSinceLastRead += 100;
  }
}


