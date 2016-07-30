#include <SimpleTimer.h>
#define BLYNK_PRINT Serial      // Comment this out to disable prints and save space
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS 0
#include <TimeLib.h>            // Used by WidgetRTC.h
#include <WidgetRTC.h>          // Blynk's RTC
#include <ArduinoJson.h>        // For parsing information from Weather Underground API

#include <ESP8266mDNS.h>        // Required for OTA
#include <WiFiUdp.h>            // Required for OTA
#include <ArduinoOTA.h>         // Required for OTA

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

DeviceAddress ds18b20kk = { 0x28, 0xEE, 0x9D, 0xEF, 0x00, 0x16, 0x02, 0x56 }; // KK

const char auth[] = "fromBlynkApp";
const char apiKey[] = "apiKey";
char ssid[] = "ssid";
char pass[] = "pw";

SimpleTimer timer;

WidgetTerminal terminal(V26);
WidgetRTC rtc;
BLYNK_ATTACH_WIDGET(rtc, V8);

int temp_f;                 // Current outdoor temp from WU API
int dailyOutsideHigh = 0;   // Today's high temp (resets at midnight)
int dailyOutsideLow = 200;  // Today's low temp (resets at midnight)
int today;
String currentWUsource = "KPHX.json"; // Fallback if unit resets!
int updateReady = 0;

// The original time entries that didn't like being removed.
#define DELAY_NORMAL    (10)
#define DELAY_ERROR     (10)

//#define WUNDERGROUND "api.wunderground.com"

void setup()
{
  Serial.begin(9600);
  Blynk.begin(auth, ssid, pw);

  WiFi.softAPdisconnect(true); // Per https://github.com/esp8266/Arduino/issues/676 this turns off AP

  while (Blynk.connect() == false) {
    // Wait until connected
  }

  // START OTA ROUTINE
  ArduinoOTA.setHostname("esp8266-Node02KK");

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("MAC address: ");
  Serial.println(WiFi.macAddress());
  // END OTA ROUTINE

  rtc.begin();

  sensors.begin();
  sensors.setResolution(ds18b20kk, 10);

  timer.setInterval(2000L, sendTemps); // Temperature sensor polling interval
  timer.setInterval(180000L, sendWU);  // ~3 minutes between Wunderground API calls.
  timer.setInterval(1000L, uptimeReport);

  daySetter();
}

void loop()
{
  Blynk.run();
  timer.run();
  ArduinoOTA.handle();
}

BLYNK_WRITE(V26)
{

  if (updateReady == 1 && String("X") != param.asStr() && String("D") != param.asStr()) {
    currentWUsource = param.asStr();
    terminal.println(""); terminal.println("");
    terminal.println("WU source is now:");
    terminal.println(currentWUsource);
    terminal.println("");
    terminal.println("       ~WU API update mode CLOSED~");
    terminal.println(""); terminal.println("");
    updateReady = 0;
    delay(10);
  }

  if (String("WU") == param.asStr()) {
    terminal.println(""); terminal.println("");
    terminal.println("           ~WU API update mode~");
    terminal.println("Current source is:");
    terminal.println(currentWUsource);
    terminal.println("");
    terminal.println("Enter new URL following '/q/' or");
    terminal.println("'X' to cancel, or 'D' for default.");
    updateReady = 1;
    delay(10);
  }
  else if (String("X") == param.asStr())
  {
    terminal.println(""); terminal.println(""); terminal.println("");
    terminal.println("     ~WU API update mode CANCELLED~");
    terminal.println(""); terminal.println("");
    updateReady = 0;
  }
  else if (String("D") == param.asStr())
  {
    currentWUsource = "KPHX.json";
    terminal.println("");
    terminal.println("Source set to:");
    terminal.println(currentWUsource);
    terminal.println("");
    terminal.println("       ~WU API update mode CLOSED~");
    terminal.println(""); terminal.println("");
    updateReady = 0;
  }

  terminal.flush();
}

BLYNK_WRITE(V27) // App button to report uptime
{
  int pinData = param.asInt();

  if (pinData == 0)
  {
    timer.setTimeout(6000L, uptimeSend);
  }
}

void uptimeSend()
{
  long minDur = millis() / 60000L;
  long hourDur = millis() / 3600000L;
  if (minDur < 121)
  {
    terminal.print(String("Node02 (KK): ") + minDur + " mins @ ");
    terminal.println(WiFi.localIP());
  }
  else if (minDur > 120)
  {
    terminal.print(String("Node02 (KK): ") + hourDur + " hrs @ ");
    terminal.println(WiFi.localIP());
  }
  terminal.flush();
}

void uptimeReport(){
  if (second() > 2 && second() < 7)
  {
    Blynk.virtualWrite(102, minute());
  }
}

void sendTemps()
{
  sensors.requestTemperatures(); // Polls the sensors

  float tempKK = sensors.getTempF(ds18b20kk); // Gets first probe on wire in lieu of by address

  if (tempKK > 0)
  {
    Blynk.virtualWrite(4, tempKK);
  }
  else
  {
    Blynk.virtualWrite(4, "ERR");
  }
}

static char respBuf[4096];
bool showWeather(char *json);

void sendWUtoBlynk()
{
  // Intended to screen out errors from Wunderground API
  if (temp_f > 10)
  {
    Blynk.virtualWrite(12, temp_f);
    Serial.println(" ");
    Serial.println(String("[") + millis() + "] Temp of " + temp_f + " sent to Blynk vPin 12.");
  }
  else
  {
    Blynk.virtualWrite(12, "ERR");
    //Blynk.tweet(String("WU API error reporting a temp value of ") + temp_f + " at " + hour() + ":" + minute() + ":" + second() + " " + month() + "/" + day() + "/" + year());
  }


  if (temp_f > dailyOutsideHigh)
  {
    dailyOutsideHigh = temp_f;
    Blynk.virtualWrite(5, dailyOutsideHigh);
  }

  if (temp_f < dailyOutsideLow && temp_f > 0) // "> 0" screens out API zero errors
  {
    dailyOutsideLow = temp_f;
    Blynk.virtualWrite(13, dailyOutsideLow);
  }

  if (today != day())
  {
    dailyOutsideHigh = 0;
    dailyOutsideLow = 200;
    daySetter();
  }
}

void daySetter()
{
  today = day();
}

void sendWU()
{
  // Open socket to WU server port 80
  Serial.print(F("Connecting to "));
  Serial.println("api.wunderground.com");

  // Use WiFiClient class to create TCP connections
  WiFiClient httpclient;
  const int httpPort = 80;
  if (!httpclient.connect("api.wunderground.com", httpPort)) {
    Serial.println(F("connection failed"));
    delay(DELAY_ERROR);
    return;
  }

  // This will send the http request to the server
  Serial.print("Sending request to Weather Underground API...");
  httpclient.print(String("GET /api/") + apiKey + "/conditions/q/" + currentWUsource + " HTTP/1.1\r\n"
                   "User-Agent: ESP8266/0.1\r\n"
                   "Accept: */*\r\n"
                   "Host: api.wunderground.com\r\n"
                   "Connection: close\r\n"
                   "\r\n");

  httpclient.flush();

  // Collect http response headers and content from Weather Underground
  // HTTP headers are discarded.
  // The content is formatted in JSON and is left in respBuf.
  int respLen = 0;
  bool skip_headers = true;
  while (httpclient.connected() || httpclient.available()) {
    if (skip_headers) {
      String aLine = httpclient.readStringUntil('\n');
      //Serial.println(aLine);
      // Blank line denotes end of headers
      if (aLine.length() <= 1) {
        skip_headers = false;
      }
    }
    else {
      int bytesIn;
      bytesIn = httpclient.read((uint8_t *)&respBuf[respLen], sizeof(respBuf) - respLen);
      Serial.print(F("bytesIn ")); Serial.println(bytesIn);
      if (bytesIn > 0) {
        respLen += bytesIn;
        if (respLen > sizeof(respBuf)) respLen = sizeof(respBuf);
      }
      else if (bytesIn < 0) {
        Serial.print(F("read error "));
        Serial.println(bytesIn);
      }
    }
    delay(1);
  }
  httpclient.stop();

  if (respLen >= sizeof(respBuf)) {
    Serial.print(F("respBuf overflow "));
    Serial.println(respLen);
    delay(DELAY_ERROR);
    return;
  }
  // Terminate the C string
  respBuf[respLen++] = '\0';
  Serial.print(F("respLen "));
  Serial.println(respLen);
  //Serial.println(respBuf);

  if (showWeather(respBuf)) {
    timer.setTimeout(5000L, sendWUtoBlynk); // Send update to Blynk app shortly after API update.
    delay(DELAY_NORMAL);
  }
  else {
    delay(DELAY_ERROR);
  }
}

bool showWeather(char *json)
{
  StaticJsonBuffer<3 * 1024> jsonBuffer;

  // Skip characters until first '{' found
  // Ignore chunked length, if present
  char *jsonstart = strchr(json, '{');
  //Serial.print(F("jsonstart ")); Serial.println(jsonstart);
  if (jsonstart == NULL) {
    Serial.println(F("JSON data missing"));
    return false;
  }
  json = jsonstart;

  // Parse JSON
  JsonObject& root = jsonBuffer.parseObject(json);
  if (!root.success()) {
    Serial.println(F("jsonBuffer.parseObject() failed"));
    return false;
  }

  // Extract weather info from parsed JSON
  JsonObject& current = root["current_observation"];
  temp_f = current["temp_f"];  // Was `const float temp_f = current["temp_f"];`
  //Serial.print(temp_f, 1); Serial.print(F(" F "));

  return true;
}
