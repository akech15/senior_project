#include "DHTesp.h" // connection of dht22 library
#include <Wire.h>
#include<HTTPClient.h>
#include "ArduinoJson.h"
#include <LiquidCrystal_I2C.h> //connection of LCD display library
#include <WiFi.h> //connection of ESP 32 wifi library
#include <Arduino.h>
#include <WebServer.h> //connection of ESP 32 server library


// relay shield pins
#define LIGHT_RELAY 5
#define MOIST_RELAY 18
#define TEMP_RELAY 19


LiquidCrystal_I2C lcd(0x3F,16,2); 

// values for condition
float downMoistLimit = 550;
float upMoistLimit = 800;
float downTempLimit = 22;
float upTempLimit = 25;
float downLightLimit = 500;
float upLightLimit = 900;


int dhtPin = 4; // define pin for DHT 22 data entrance 
int pinSoilMoisture = 35; // define soil humidity analog pin
int photoPin = 32; // define photoresistor analog pin
// creation of an object of dht
DHTesp dht;
// network credentials
const char* ssid     = "Cisco58834";         
const char* password = "qwertyui"; 
//const char* ssid     = "AndroidAPBF58";         
//const char* password = "csly1816";

long id = 1;

int lightOn = 1;
int conditioningOn = 1;
int irrigationSystemOn = 1;

float temp;
float hum;
float moist;
float light;

void setup() {
  
  Serial.begin(115200);
  lcd.init(); // initialize the lcd
  lcd.init();
  lcd.backlight();   // print a message to the LCD 
 
  // set relay mosule to low
  pinMode(LIGHT_RELAY, OUTPUT); digitalWrite(LIGHT_RELAY, LOW);
  pinMode(MOIST_RELAY, OUTPUT); digitalWrite(MOIST_RELAY, LOW);
  pinMode(TEMP_RELAY, OUTPUT); digitalWrite(TEMP_RELAY, LOW);
  
  dht.setup(dhtPin, DHTesp::DHT22);
  Serial.println("DHT initiated");
  Serial.println('\n');
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);             // Connect to the network
  Serial.print("Connecting to ");
  Serial.print(ssid);
 
  while (WiFi.status() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
    delay(500);
    Serial.print('.');
  }
 
  Serial.println('\n');
  Serial.println("Connection established!");  
  Serial.print("IP address:\t");
}


void loop() {
   moist = analogRead(pinSoilMoisture)/4; // soil humadity value
   light =  analogRead(photoPin)/4;// photoresistor value
   TempAndHumidity dhtValues = dht.getTempAndHumidity();
   temp = dhtValues.temperature;
   hum = dhtValues.humidity;


  postInfromation(moist, light, temp, hum);
   // display  values on lcd
   displayValuesOnLcd(moist, light, temp, hum);
   getChangedInformation();
   getSysChangeInformation();
  // make relays wors when it is needed
  if (1023- analogRead(photoPin)/4 < downLightLimit) {
    if (lightOn == 1){
      digitalWrite(LIGHT_RELAY, HIGH);
    }else{
      digitalWrite(LIGHT_RELAY, LOW);
    }
  } else if (1023- analogRead(photoPin)/4 > upLightLimit){
      digitalWrite(LIGHT_RELAY, LOW);
  }

  if (temp > upTempLimit) {
    if (conditioningOn == 1){
      digitalWrite(TEMP_RELAY, HIGH);
    }else{
      digitalWrite(TEMP_RELAY, LOW);
    }
  }else if (temp < downTempLimit ){
      digitalWrite(TEMP_RELAY, LOW);
  }

  if (analogRead(pinSoilMoisture)/4 < downMoistLimit ) {
    if (irrigationSystemOn == 1){
      digitalWrite(MOIST_RELAY, HIGH);
    }else{
      digitalWrite(MOIST_RELAY, LOW);
    }
  } else if (analogRead(pinSoilMoisture)/4 > upMoistLimit){
      digitalWrite(MOIST_RELAY, LOW);
  }
  delay(2000);
 
}

void postInfromation (float moist, float light, float temp, float hum){
    
    HTTPClient http;
    http.begin("http://192.168.1.103:8080/esp32");
    http.addHeader("Content-Type", "application/json");
    StaticJsonDocument<200> doc;
    doc["id"]= id; 
    doc["temperature"] = temp;
    doc["humidity"] = hum;
    doc["moisture"] = moist;
    doc["light"] = 1023 - light;
    String requestBody;
    serializeJson(doc, requestBody);
    int httpResponseCode= http.PUT(requestBody);
    if(httpResponseCode>0){
      String response = http.getString();                       
      Serial.println(httpResponseCode);   
      Serial.println(response);
    }
    
}
 void displayValuesOnLcd (float moist, float light, float temp, float hum){
    lcd.setCursor(0, 0);
    Serial.print("HumidityDHT22= "); Serial.print(hum); Serial.println(" %");
    lcd.print("H:"); lcd.print(hum); lcd.print("%");
    Serial.print("light= "); Serial.print(1023-light); Serial.println();
    lcd.print(" L:"); lcd.print(1023-light);

    lcd.setCursor(0, 1);

    Serial.print("TemperatureDHT22= "); Serial.print(temp); Serial.println(" C");
    lcd.print("T:"); lcd.print(temp); lcd.print("C");
    Serial.print("SoilMoisture= "); Serial.println(moist);
    lcd.print(" SM:"); lcd.print(moist);
 }

 void getChangedInformation(){
     HTTPClient http;
     String serverName = "http://192.168.1.103:8080/esp32/get-limits/";
     String ServerPath = serverName + id;
     http.begin(ServerPath);
     int httpCode = http.GET();
     if(httpCode == 200){
       String result = http.getString();
       if (result != null){
         StaticJsonDocument <300> doc;
         DeserializationError err  = deserializeJson(doc, result);
         downTempLimit =  doc["downTemperatureLimit"];
         upTempLimit =  doc["upTemperatureLimit"];
         downMoistLimit =  doc["downMoistureLimit"];
         upMoistLimit =  doc["upMoistureLimit"];
         downLightLimit =  doc["downLightLimit"];
         upLightLimit =  doc["upLightLimit"];
         Serial.println(downTempLimit);
         Serial.println(upTempLimit);
         Serial.println(downMoistLimit);
         Serial.println(upMoistLimit);
         Serial.println(downLightLimit);
         Serial.println(upLightLimit);
      }
    }
 }

 void getSysChangeInformation(){
     HTTPClient http;
     String serverName = "http://192.168.1.103:8080/esp32/get-systemInf/";
     String ServerPath = serverName + id;
     http.begin(ServerPath);
     int httpCode = http.GET();
     if(httpCode == 200){
       String result = http.getString();
       if (result != null){
         StaticJsonDocument <300> doc;
         DeserializationError err  = deserializeJson(doc, result);
         lightOn = doc["lightOn"];
         conditioningOn = doc["conditioningOn"];
         irrigationSystemOn = doc["irrigationSystemOn"];
       }
     }
     Serial.println(irrigationSystemOn);
 }
 
