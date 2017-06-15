/*
 *  This sketch demonstrates how to scan WiFi networks. 
 *  The API is almost the same as with the WiFi Shield library, 
 *  the most obvious difference being the different file you need to include:
 */
#include <FS.h> 
#include <ESP8266WiFi.h>          //ESP8266 Core WiFi Library (you most likely already have this in your sketch)

#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson

 

char hub[80]      = "www.gothamrenters.com";              // website
char label[80]    = "apartment";
//flag for saving data
bool shouldSaveConfig = false;



const unsigned long long_press_time = 50000; // is millis returning 0.1ms?


int sensorPin = A0; //the analog pin
int inputPin = D2;
float temperature;

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
    else{
      int reading = analogRead(sensorPin); // current voltage off the sensor
      float voltage = 1000*reading * (3.3/1024);       // using 3.3v input
      temperature = (voltage - 500)/10;  //converting from 10 mv per degree with 500 mV offset
                                                 //to degrees C ((voltage - 500mV) times 100)

      // this is the internet bit
      broadcast(hub,label,temperature);

      
      // Print SSID and RSSI for each network found
      Serial.print("Temperature: ");
      Serial.print(temperature);
      Serial.print(" (C)");
      Serial.println();
      delay(5000);
      
      }
  
}


void broadcast( char* hub, char* label,float value) {

  String url;
  String PostData;

  url = "/sensor/"+String(label);
  PostData = String("{\"value\": ")+String(value)+String("}");
  
 
  // Use WiFiClient class to create TCP connections
  WiFiClient client;
 
  // port = 80 for web stuffs
  const int httpPort = 80;
 
  // try out that connection plz
  if (client.connect(hub, httpPort)) {
    client.println("POST "+url+" HTTP/1.1");
    client.println("Host: "+String(hub));
    client.println("Content-Type: application/json");
    client.println("User-Agent: Arduino/1.0");
    client.println("Connection: close");
    client.print("Content-Length: ");
    client.println(PostData.length());
    client.println();
    client.println(PostData);
  }
 
  // tiny delay            
  delay(10);
  
}

