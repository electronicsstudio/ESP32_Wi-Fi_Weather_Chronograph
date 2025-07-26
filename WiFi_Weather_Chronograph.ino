/*
@Electronics Studio
@file WiFi_Weather_Chronograph.ino
@date  15-07-2025
@url https:https: https://github.com/electronicsstudio
@url YouTube: https://www.youtube.com/@ElectronicsStudio-v7o/featured
*/

#include <WiFi.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

LiquidCrystal_I2C LCD(0x27, 16, 2); // I2C address 0x27, 16 columns, 2 rows

// WiFi Credentials
const char* ssid = "xxxxxxx";
const char* password = "xxxxxx";

// OpenWeatherMap Settings
const String apiKey = "API ID"; // Replace with your actual API key
const String city = "Bengaluru";
const String countryCode = "IN";
String weatherUrl = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "," + countryCode + "&units=metric&appid=" + apiKey;

// NTP Settings
#define NTP_SERVER "in.pool.ntp.org"  // India-specific NTP server for better accuracy
#define UTC_OFFSET 19800 // 5.5*3600 for IST
#define UTC_OFFSET_DST 0
#define NTP_SYNC_INTERVAL 3600000 // Sync time every 1 hour

// Display variables
float temperature = 0;
float humidity = 0;
unsigned long lastWeatherUpdate = 0;
const long weatherUpdateInterval = 600000; // Update every 10 minutes
unsigned long lastPageChange = 0;
const long pageChangeInterval = 2000; // Switch pages every 2 seconds
unsigned long lastNTPSync = 0;
bool showPage1 = true;

void syncTime() {
  if (millis() - lastNTPSync > NTP_SYNC_INTERVAL) {
    configTime(UTC_OFFSET, UTC_OFFSET_DST, NTP_SERVER);
    lastNTPSync = millis();
    Serial.println("Time re-synced with NTP server");
    
    // Force display update after sync
    updateDisplay();
  }
}

void setup() {
  Serial.begin(115200);
  
  // Initialize LCD
  LCD.begin();
  LCD.backlight();
  LCD.setCursor(0, 0);
  LCD.print("Connecting WiFi");
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    LCD.setCursor(0, 1);
    LCD.print(".");
  }
  
  LCD.clear();
  LCD.setCursor(0, 0);
  LCD.print("WiFi Connected!");
  delay(2000);

  // Configure NTP with aggressive timeout
  configTzTime("IST-5:30", "in.pool.ntp.org", "time.google.com", "pool.ntp.org");
  
  // Get initial weather data
  getWeather();
}

void updateDisplay() {
  LCD.clear();
  
  if (showPage1) {
    // Page 1: Time and Date
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
      // Line 1: Time
      LCD.setCursor(0, 0);
      LCD.print("Time: ");
      LCD.printf("%02d:%02d:%02d", 
                timeinfo.tm_hour, 
                timeinfo.tm_min, 
                timeinfo.tm_sec);
      
      // Line 2: Date
      LCD.setCursor(0, 1);
      LCD.print("Date: ");
      LCD.printf("%02d/%02d/%04d", 
                timeinfo.tm_mday, 
                timeinfo.tm_mon + 1, 
                timeinfo.tm_year + 1900);
    } else {
      LCD.print("Time Error");
    }
  } else {
    // Page 2: Weather
    LCD.setCursor(0, 0);
    LCD.print("BLR Temp: ");
    LCD.print(temperature, 1); // Show 1 decimal place
    LCD.print("C");
    
    LCD.setCursor(0, 1);
    LCD.print("BLR Humid: ");
    LCD.print(humidity, 0); // No decimal places
    LCD.print("%");
  }
}

void getWeather() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected");
    return;
  }

  HTTPClient http;
  http.begin(weatherUrl);
  http.setTimeout(5000); // 5-second timeout
  
  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    
    DynamicJsonDocument doc(2048); // Increased buffer size
    deserializeJson(doc, payload);
    
    temperature = doc["main"]["temp"];
    humidity = doc["main"]["humidity"];
    lastWeatherUpdate = millis();
    
    Serial.printf("Weather updated: %.1fC, %.0f%%\n", temperature, humidity);
  } else {
    Serial.printf("Weather API error: %d\n", httpCode);
    // Retry sooner if failed
    lastWeatherUpdate = millis() - weatherUpdateInterval + 60000; // Retry in 1 minute
  }
  
  http.end();
}

void loop() {
  // Sync time periodically
  syncTime();

  // Check if we need to update weather
  if (millis() - lastWeatherUpdate > weatherUpdateInterval) {
    getWeather();
  }

  // Check if we need to switch pages
  if (millis() - lastPageChange > pageChangeInterval) {
    showPage1 = !showPage1;
    lastPageChange = millis();
    updateDisplay();
  }

  // Update time display more frequently for seconds to update
  if (showPage1) {
    static time_t lastSecond = 0;
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
      if (timeinfo.tm_sec != lastSecond) {
        updateDisplay();
        lastSecond = timeinfo.tm_sec;
      }
    }
  }

  delay(100);
}


