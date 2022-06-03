#include <Arduino.h>
#include <stdio.h>
#include <stdlib.h>
#include <String.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <virtuabotixRTC.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <DS1302.h>
#include <ThreeWire.h>  
#include <RtcDS1302.h>

#include "DHT.h"
// Adafruit Server Key
#define IO_USERNAME  "tronghuynhhuyhung"
#define IO_KEY       "aio_FjqQ87Ba7wUYvMcVgMoJ03ZmV7Jf"

// WiFi SSID & Password
#define WIFI_SSID "THINKPADACOME 7470"
#define WIFI_PASSWORD "tronghuynh"

// PINS for Soil Humidity sensor
#define ANALOG_SENSOR_VALUE_PIN  32

// PIN for Motor
#define MOTOR_PIN                 27

// PIN for DHT11
#define DHT_PIN                   26

// SSD1306
#define SCREEN_WIDTH              128   // srceen width
#define SCREEN_HEIGHT             32    // srceen height
#define SSD1306_RESET             -1

// DS1302
#define CLK_PIN                   5
#define DATA_PIN                  4
#define RST_PIN                   2
#define countof(a) (sizeof(a) / sizeof(a[0]))
ThreeWire myWire(DATA_PIN,CLK_PIN, RST_PIN); // DATA, SCLK, RST
RtcDS1302<ThreeWire> Rtc(myWire);

// Variables for Soil Humidity sensor
int Digital_Sensor_value = 0;
int Percent = 0;
int map_low = 4095, map_high = 800;

// Variables for DHT11
float DHT_Value_temperature = 0;

// Variables for motor
// bool isSet = false;

// Others variables
int Publish_count = 0, Display_count = 0;

WiFiClient    Esp_Client;
PubSubClient  Client(Esp_Client);
// PubSubClient  Client_Get_HoursForWater(Esp_Client);

DHT  DHT_Sensor(DHT_PIN, DHT11);  // Setup DHT11 Sensor

// Setup SSD1306
Adafruit_SSD1306  display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, SSD1306_RESET);  // Setup SSD1306

void CallBack(char* topic, byte* payload, unsigned int length) {
  Serial.println("Topic upadated");

  // Print message
  Serial.print("payload: ");
  for(int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  if ((char)payload[0] == 'o' || (char)payload[0] == 'f') {  
    Serial.print("Remote: ");
    Serial.println((char)payload[0]);
    Server_Remote_Motor(payload, length);
  }
}

//   Server_Remote_Motor
void Server_Remote_Motor(byte* payload, unsigned int length) {
  if ((char)payload[0] == 'o') {
    digitalWrite(MOTOR_PIN, HIGH);
    delay(3000);
  }
  else if ((char)payload[0] == 'f'){
    digitalWrite(MOTOR_PIN, LOW);
  }
}
//Publishing Temperature to the server
void Publishing_Temperature(float value) {
  char temp[4]; 
  itoa(value, temp, 10);
  char* message = temp;
  Client.publish("tronghuynhhuyhung/feeds/Temperature", message, 4);
  delay(2000);  
}

//Publishing SoilHumidity to the server
void Publishing(int value) {
  //Serial.println(value);
  //delay(1000);
  
  char temp[4];
  itoa(value, temp, 10);
  char* message = temp;
  Client.publish("tronghuynhhuyhung/feeds/SoilHumidity", message, 4);
  delay(2000);
}

// WiFi connect set up
void WiFiConnect() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while(WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting");
  }
  Serial.println(WiFi.localIP());
}

// Set up Adafruit Server
void Setup_Adafruit_Server() {
  Client.setServer("io.adafruit.com", 1883);
  Client.setCallback(CallBack);
  Client.connect("AutomaticWatering", IO_USERNAME, IO_KEY);
  Client.subscribe("tronghuynhhuyhung/feeds/+");  
  Serial.println("Adafruit Server has been set up");
}

// Try to reconnect when server broken
void Adafruit_Server_reconnect()
{
  while(!Client.connected()) {
    if(Client.connect("AutomaticWatering", IO_USERNAME, IO_KEY)) {
      Serial.println("Connected (re)");
      Client.subscribe("tronghuynhhuyhung/feeds/+");
    }
    else {
      Serial.println("Waiting reconnected");
      delay(4000);
    } 
  }
}

// Print temp to OLED
void SSD1306_Print_Temp(float temperature) {
  display.setCursor(0, 0);
  display.clearDisplay();
  display.display();
  display.println("TEMPERATUR");
  display.print(temperature);
  display.print(" oC");
  display.display();
}

// Print Soil Humidity to OLED
void SSD1306_Print_Soil_Humidity(int Soil_Sensor_value) {
  display.setCursor(0, 0);
  display.clearDisplay();
  display.display();
  display.println("HUMIDITY");
  display.print(Soil_Sensor_value);
  display.print(" %");
  display.display();
}


void  Setup_RTC_DS1302() 
{
  Rtc.Begin();
  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);  

 if (!Rtc.IsDateTimeValid()) 
    {
        // Common Causes:
        //    1) first time you ran and the device wasn't running yet
        //    2) the battery on the device is low or even missing

        Serial.println("RTC lost confidence in the DateTime!");
        Rtc.SetDateTime(compiled);
    }

    if (Rtc.GetIsWriteProtected())
    {
        Serial.println("RTC was write protected, enabling writing now");
        Rtc.SetIsWriteProtected(false);
    }

    if (!Rtc.GetIsRunning())
    {
        Serial.println("RTC was not actively running, starting now");
        Rtc.SetIsRunning(true);
    }

    RtcDateTime now = Rtc.GetDateTime();
    if (now < compiled) 
    {
        Serial.println("RTC is older than compile time!  (Updating DateTime)");
        Rtc.SetDateTime(compiled);
    }
    else if (now > compiled) 
    {
        Serial.println("RTC is newer than compile time. (this is expected)");
    }
    else if (now == compiled) 
    {
        Serial.println("RTC is the same as compile time! (not expected but all is fine)");
    }
}

// Print real time to OLED
void SSD1306_Print_Realtime (const RtcDateTime& dt)
{
  display.setCursor(0, 0);
  display.clearDisplay();
  display.display();
  
  display.println("TIME:");
  display.print(dt.Hour());
  display.print(":");
  display.print(dt.Minute());
  display.print(":");
  display.println(dt.Second());
  display.display();
  //delay(10000);   
}

void Setup_SSD1306() {
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  delay(2000);

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
  display.display();  
}

void setup() {
  Serial.begin(9600);

  // Setup Soil Humidity sensor
  pinMode(ANALOG_SENSOR_VALUE_PIN, INPUT);
  pinMode(MOTOR_PIN, OUTPUT);
 
  WiFiConnect();
  Setup_Adafruit_Server();
  DHT_Sensor.begin();
  Setup_RTC_DS1302();
  Setup_SSD1306();
}

void loop() {
  Client.loop();
  RtcDateTime now = Rtc.GetDateTime();
  
  if (!Client.connected()) {
    Adafruit_Server_reconnect();
  }

  Analog_Sensor_value = analogRead(ANALOG_SENSOR_VALUE_PIN);
  
  DHT_Value_temperature = DHT_Sensor.readTemperature();
  Percent = map(Analog_Sensor_value, map_low, map_high, 0, 100) ;
  int HighHumi = 50; // set Soil Humidity threshold
  if (Percent >= HighHumi)
  {
     digitalWrite(MOTOR_PIN, LOW);
     Serial.println("off");
  }
  else
  {
     digitalWrite(MOTOR_PIN, HIGH);
     Serial.println("on");
     delay(3000);
     digitalWrite(MOTOR_PIN, LOW);
     Serial.println("off");
  }

  // use count variable so as not to affect other tasks (Publish_count, Display_count)
  if(Publish_count == 20) {  // upload data to the server after 20s
    if (!isnan(Digital_Sensor_value)) {
      Publishing(Percent);
      Publishing_Temperature(DHT_Value_temperature);
    }
    //printDateTime(now);
    Publish_count = 0;
  } 
  Publish_count++;

  if (Display_count >= 0 && Display_count < 3) {  // display time 3s
    //printDateTime();
    SSD1306_Print_Realtime(now);
    Display_count++;
  }
  else if (Display_count >= 3 && Display_count < 6){ // display Soil Humidity 3s
    SSD1306_Print_Soil_Humidity(Percent);
    Display_count++;
  }
  else if (Display_count >= 6 && Display_count < 9){ // display temperature 3s
    SSD1306_Print_Temp(DHT_Value_temperature);
    Serial.println(DHT_Value_temperature);
    Display_count++;
  }
  else {
    Display_count = 0;
  }

  delay(1000);  
}