/*
 *  This sketch demonstrates how to scan WiFi networks. 
 *  The API is almost the same as with the WiFi Shield library, 
 *  the most obvious difference being the different file you need to include:
 */
#include <ESP8266WiFi.h>          //ESP8266 Core WiFi Library (you most likely already have this in your sketch)

#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
 

const char* host      = "www.gothamrenters.com";              // website
const String id        = "apartment";
const unsigned long long_press_time = 50000; // is millis returning 0.1ms?


int sensorPin = A0; //the analog pin
int inputPin = D2;
float temperature;

bool long_press_flag=0;
unsigned long press_time=0;
unsigned long release_time=0;

// The amount of time (in milliseconds) to wait
#define TEST_DELAY   2000

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

void setup() {
  Serial.begin(115200);
  Serial.println("Setup starting");
  WiFiManager wifiManager;
  
  temperature = 0.0;
  pinMode(inputPin,INPUT_PULLUP);
  attachInterrupt(inputPin,press,FALLING);
  attachInterrupt(inputPin,release,RISING);
  wifiManager.autoConnect("SensorNetwork", "BkBoy#1980");

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
      connect();

      
      // Print SSID and RSSI for each network found
      Serial.print("Temperature: ");
      Serial.print(temperature);
      Serial.print(" (C)\n");
      delay(5000);
      
      }
  
}


void connect() {

  String url;
  String PostData;

  url = "/sensor/"+id;
  PostData = String("{\"value\": ")+temperature+String("}");
  Serial.println(PostData);
 
  // Use WiFiClient class to create TCP connections
  WiFiClient client;
 
  // port = 80 for web stuffs
  const int httpPort = 80;
 
  // try out that connection plz
  if (client.connect(host, httpPort)) {
    client.println("POST "+url+" HTTP/1.1");
    client.println("Host: "+String(host));
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

