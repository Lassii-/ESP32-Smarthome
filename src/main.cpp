#include <Arduino.h>
#include <ArduinoJson.h>
#include <DallasTemperature.h>
#include <HTTPClient.h>
#include <LiquidCrystal_I2C.h>
#include <NTPClient.h>
#include <OneWire.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <chars.h>
#include <cmath>
#include <creds.h>

const String ssid = wifissid;
const String password = wifipassword;
const String endpoint = owmendpoint;
const String key = owmapikey;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

String formattedTime;

OneWire oneWire(23);
DallasTemperature sensors(&oneWire);
int8_t temperature;
const char *weathertype;
const char *owmwtype;
int16_t owmtemp;
unsigned long lastMeasurement = 0;
unsigned long currentMillis;
std::pair<int, const char *> OWM;

LiquidCrystal_I2C lcd(0x27, 16, 2);

int8_t getDallasTemperature() {
  int8_t temp;
  sensors.requestTemperatures();
  temp = round(sensors.getTempCByIndex(0));
  return temp;
}

std::pair<int16_t, const char *> getOWMData() {
  if ((WiFi.status() == WL_CONNECTED)) {
    HTTPClient http;
    http.begin(endpoint + key);
    int httpCode = http.GET();
    if (httpCode > 0) {
      String payload = http.getString();
      StaticJsonDocument<5000> root;
      deserializeJson(root, payload);
      JsonObject main = root["main"];
      JsonArray weatherArray = root["weather"];
      JsonObject weather = weatherArray[0];
      owmwtype = weather["main"];
      owmtemp = main["temp"];
      owmtemp = round(owmtemp - 273.15);
      return std::make_pair(owmtemp, owmwtype);
    } else {
      Serial.println("Error on HTTP request.");
    }
    http.end();
  }
}

void drawLCD(int8_t apartment, String time,
             std::pair<int16_t, const char *> OWMData) {
  lcd.setCursor(0, 0);
  lcd.write(byte(0));
  lcd.print(apartment);
  lcd.write(byte(2));
  lcd.setCursor(5, 0);
  lcd.println(time.c_str());
  lcd.setCursor(0, 1);
  lcd.write(byte(1));
  lcd.print(OWMData.first);
  lcd.write(byte(2));
  lcd.setCursor(5, 1);
  lcd.print(OWMData.second);
}

void setup() {
  lcd.init();
  lcd.backlight();
  lcd.createChar(0, house);
  lcd.createChar(1, outside);
  lcd.createChar(2, celcius);
  Serial.begin(9600);
  sensors.begin();
  WiFi.begin(ssid.c_str(), password.c_str());
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to wifi.");
  }
  Serial.println("Connected to to wifi.");
  timeClient.begin();
  timeClient.setTimeOffset(10800);
}

void loop() {
  while (!timeClient.update()) {
    timeClient.forceUpdate();
  }
  formattedTime = timeClient.getFormattedTime();
  unsigned long currentMillis = millis();
  if (currentMillis - lastMeasurement > 60000) {
    lcd.clear();
    temperature = getDallasTemperature();
    OWM = getOWMData();
    lastMeasurement = millis();
  }
  drawLCD(temperature, formattedTime, OWM);
  delay(1000);
}