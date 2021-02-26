#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino.h>
#include <Time.h>
#include <FS.h>
#include <SPIFFS.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif


// Include the libraries we need
#include <OneWire.h>
#include <DallasTemperature.h>

#define uS_TO_S_FACTOR 1000000  
#define TIME_TO_SLEEP  600

// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS A10

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

// arrays to hold device address
DeviceAddress insideThermometer;

// Import required libraries
#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"


// Replace with your network credentials
//const char* ssid = "Sivak";
//const char* password = "xxx";

/* Put IP Address details */
IPAddress local_ip(192, 168, 1, 2);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
// Set LED GPIO
const int ledPin = 16;
// Stores LED state
String ledState;
int ADC_VALUE = 0;

String networkname = "";

float Temperature = 0;
//Your Domain name with URL path or IP address with path
const char* serverName = "http://192.168.1.1:80/sendtemp";


void print_wakeup_reason() {
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason); break;
  }
}



/*
  Method to print the touchpad by which ESP32
  has been awaken from sleep
*/
void print_wakeup_touchpad() {
  //touch_pad_t pin;
  touch_pad_t touchPin;
  touchPin = esp_sleep_get_touchpad_wakeup_status();

  switch (touchPin)
  {
    case 0  : Serial.println("Touch detected on GPIO 4"); break;
    case 1  : Serial.println("Touch detected on GPIO 0"); break;
    case 2  : Serial.println("Touch detected on GPIO 2"); break;
    case 3  : Serial.println("Touch detected on GPIO 15"); break;
    case 4  : Serial.println("Touch detected on GPIO 13"); break;
    case 5  : Serial.println("Touch detected on GPIO 12"); break;
    case 6  : Serial.println("Touch detected on GPIO 14"); break;
    case 7  : Serial.println("Touch detected on GPIO 27"); break;
    case 8  : Serial.println("Touch detected on GPIO 33"); break;
    case 9  : Serial.println("Touch detected on GPIO 32"); break;
    default : Serial.println("Wakeup not by touchpad"); break;
  }
}


const char* wl_status_to_string(wl_status_t status) {
  switch (status) {
    case WL_NO_SHIELD: return "WL_NO_SHIELD";
    case WL_IDLE_STATUS: return "WL_IDLE_STATUS";
    case WL_NO_SSID_AVAIL: return "WL_NO_SSID_AVAIL";
    case WL_SCAN_COMPLETED: return "WL_SCAN_COMPLETED";
    case WL_CONNECTED: return "WL_CONNECTED";
    case WL_CONNECT_FAILED: return "WL_CONNECT_FAILED";
    case WL_CONNECTION_LOST: return "WL_CONNECTION_LOST";
    case WL_DISCONNECTED: return "WL_DISCONNECTED";
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(35, INPUT);
  pinMode(16, OUTPUT);
digitalWrite(16,LOW);

  analogSetClockDiv(32);
  analogSetPinAttenuation(35, ADC_11db);
ADC_VALUE = analogRead(35);
 Serial.println(ADC_VALUE); 
 WiFi.mode(WIFI_STA);
//  WiFi.softAP("ESPThermometer", "12345678");
//  WiFi.softAPConfig(local_ip, gateway, subnet);

   int n = WiFi.scanNetworks();

  // Serial.println("scan done");

  if (n == 0) {

    // Serial.println("no networks found");

  } else {

    // Serial.print(n);

    //Serial.println(" networks found");

    for (int i = 0; i < n; ++i) {

      // Print SSID and RSSI for each network found

      //Serial.print(i + 1);

      //Serial.print(": ");

      //Serial.print(WiFi.SSID(i));
      if (WiFi.SSID(i) == "ESPThermostatv3") networkname = "TermostatControl";
      if (WiFi.SSID(i) == "nextpoint") networkname = "nexpointnet";
      if (WiFi.SSID(i) == "thirdpoint") networkname = "thirdpoint";
      //Serial.print(" (");

      //Serial.print(WiFi.RSSI(i));

      //Serial.println(")");

      // Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?" ":"*");

      //delay(10);

    }

  }

  if (networkname == "TermostatControl")
    WiFi.begin("ESPThermostatv3", "12345678");       //connect to thermostat
int counter=0;
    while (WiFi.status() != WL_CONNECTED) {
      delay(100);
      Serial.print(".");

      counter++;
      if (counter > 100) break;
    }
    if (WiFi.status() == WL_CONNECTED)
    {
      Serial.print("Connected to Control...");
      Serial.println(networkname);
      // Print ESP32 Local IP Address
      Serial.println(WiFi.localIP());
    }
  else
  {
    Serial.println("Nenajdena Wifi!!!");  //nocheck
  }
  
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed");
    return;
  }

  File root = SPIFFS.open("/");
  if (!root) {
    Serial.println("- failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println(" - not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      //    if(0){ //levels
      //      listDir(SPIFFS, file.name(), 0); //levels
      // }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("\tSIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
  Serial.print("Wifi Status: ");
  Serial.println(WiFi.status());

   // locate devices on the bus
  Serial.print("Locating devices...");
  sensors.begin();
  Serial.print("Found ");
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(" devices.");

  // report parasite power requirements
  Serial.print("Parasite power is: ");
  if (sensors.isParasitePowerMode()) Serial.println("ON");
  else Serial.println("OFF");


  if (!sensors.getAddress(insideThermometer, 0)) Serial.println("Unable to find address for Device 0");


  Serial.print("Device 0 Address: ");
  for (uint8_t i = 0; i < 8; i++)
  {
    if (insideThermometer[i] < 16) Serial.print("0");
    Serial.print(insideThermometer[i], HEX);
  }
  // printAddress();
  Serial.println();

  // set the resolution to 9 bit (Each Dallas/Maxim device is capable of several different resolutions)
  sensors.setResolution(insideThermometer, 9);

 // Serial.print("Device 0 Resolution: ");
 // Serial.print(sensors.getResolution(insideThermometer), DEC);
 // Serial.println();
  

 sensors.requestTemperatures(); // Send the command to get temperatures
 delay(1000);
  float tempC = sensors.getTempC(insideThermometer);
  if (tempC == -127) tempC = 99;
  Serial.println(tempC);

  if (WiFi.status() == WL_CONNECTED)
    {
     HTTPClient http;
      
      // Your Domain name with URL path or IP address with path
      http.begin(serverName);

      // Specify content-type header
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");
      // Data to send with HTTP POST
      float volt=(float)ADC_VALUE*0.0008056640625*2.1;
      Serial.print("Voltage=");
      Serial.println( roundf(volt*100.0)/100.0 );
      String httpRequestData = "Temp="+ String(tempC)+"&ADC="+String(ADC_VALUE)+"&Volt="+String(roundf(volt*100.0)/100.0);   
      //Serial.println(httpRequestData);        
      // Send HTTP POST request
      int httpResponseCode = http.POST(httpRequestData);
      
      // If you need an HTTP request with a content type: application/json, use the following:
      //http.addHeader("Content-Type", "application/json");
      //int httpResponseCode = http.POST("{\"api_key\":\"tPmAT5Ab3j7F9\",\"sensor\":\"BME280\",\"value1\":\"24.25\",\"value2\":\"49.54\",\"value3\":\"1005.14\"}");

      // If you need an HTTP request with a content type: text/plain
      //http.addHeader("Content-Type", "text/plain");
      //int httpResponseCode = http.POST("Hello, World!");
     
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
       
      // Free resources
      http.end();
    }

  
    Serial.println("Going to sleep now");
       digitalWrite(16, HIGH);
    
 esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
    esp_deep_sleep_start();
  
}

void loop() {
  // put your main code here, to run repeatedly:

}
