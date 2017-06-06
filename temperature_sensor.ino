/*
 *  This sketch demonstrates how to scan WiFi networks. 
 *  The API is almost the same as with the WiFi Shield library, 
 *  the most obvious difference being the different file you need to include:
 */
#include "ESP8266WiFi.h"
 
const char* ssid      = "2R";       // SSID
const char* password  = "BkBoy#1980";   // Password
const char* host      = "www.gothamrenters.com";              // website
const String id        = "apartment";
int sensorPin = A0; //the analog pin
float temperature;

// The amount of time (in milliseconds) to wait
#define TEST_DELAY   2000

void setup() {
  Serial.begin(115200);
  Serial.println("Setup starting");
  
  temperature = 0.0;
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("Attempting to connect to WiFi\n");  
    delay(500);
  }

  Serial.println("Setup done");
}

void loop() {


                                                 
   // not really necessary, just feels right to me to have a nice clean temperature variable :)

   
  
  Serial.println("scan start");
    while (1)
    {
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
  

  // Wait a bit before scanning again
  
}


void connect() {

  String url;
  String PostData;

  url = "/sensor/"+id;
  PostData = String("{\"value\": ")+temperature+String("}");
  Serial.print(PostData);
 
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

