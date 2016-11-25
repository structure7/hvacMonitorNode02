/* Node02 responsibilities:
   - Reports KK's bedroom temperature as displays last 24-hours high/low temps in app display label.
   - Previously collected outside temp via Weather Underground API.
*/

#include <SimpleTimer.h>
#define BLYNK_PRINT Serial      // Comment this out to disable prints and save space
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#include <TimeLib.h>            // Used by WidgetRTC.h
#include <WidgetRTC.h>          // Blynk's RTC

#include <ESP8266mDNS.h>        // Required for OTA
#include <WiFiUdp.h>            // Required for OTA
#include <ArduinoOTA.h>         // Required for OTA

#define ONE_WIRE_BUS 0          // WeMos pin D3 w/ pull-up
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress ds18b20kk = { 0x28, 0xEE, 0x9D, 0xEF, 0x00, 0x16, 0x02, 0x56 }; // KK

const char auth[] = "fromBlnkApp";
char ssid[] = "ssid";
char pass[] = "pw";

SimpleTimer timer;

WidgetTerminal terminal(V26);
WidgetRTC rtc;
BLYNK_ATTACH_WIDGET(rtc, V8);

double tempKK;    // Room temp
int tempKKint;    // Room temp converted to int

int dailyHigh = 0;
int dailyLow = 200;

int last24high, last24low;    // Rolling high/low temps in last 24-hours.
int last24hoursTemps[288];    // Last 24-hours temps recorded every 5 minutes.
int arrayIndex = 0;

void setup()
{
  Serial.begin(9600);
  Blynk.begin(auth, ssid, pass);

  //WiFi.softAPdisconnect(true); // Per https://github.com/esp8266/Arduino/issues/676 this turns off AP

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
  sensors.setResolution(10);

  timer.setInterval(2000L, sendTemps);            // Temperature sensor reporting to app display
  timer.setInterval(1000L, uptimeReport);         // Records current minute
  timer.setInterval(300000L, recordTempToArray);  // Array updated ~5 minutes
  timer.setTimeout(5000, setupArray);             // Sets entire array to temp at startup for a "baseline"
  timer.setInterval(15000L, hiLoTemps);
  timer.setInterval(30000L, resetHiLoTemps);
}

void loop()
{
  Blynk.run();
  timer.run();
  ArduinoOTA.handle();
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

void uptimeReport() {
  if (second() > 2 && second() < 7)
  {
    Blynk.virtualWrite(102, minute());
  }
}

void sendTemps()
{
  sensors.requestTemperatures();         // Polls the sensors

  tempKK = sensors.getTempF(ds18b20kk);

  // Conversion of tempKK to tempKKint
  int xKKint = (int) tempKK;
  double xKK10ths = (tempKK - xKKint);
  if (xKK10ths >= .50)
  {
    tempKKint = (xKKint + 1);
  }
  else
  {
    tempKKint = xKKint;
  }

  // Send temperature to the app display
  if (tempKK > 0)
  {
    Blynk.virtualWrite(4, tempKK);
  }
  else
  {
    Blynk.virtualWrite(4, "ERR");
  }

  // Set the app display color based on temperature
  if (tempKK < 78)
  {
    Blynk.setProperty(V4, "color", "#04C0F8"); // Blue
  }
  else if (tempKK >= 78 && tempKK <= 80)
  {
    Blynk.setProperty(V4, "color", "#ED9D00"); // Yellow
  }
  else if (tempKK > 80)
  {
    Blynk.setProperty(V4, "color", "#D3435C"); // Red
  }
}

void setupArray()
{
  for (int i = 0; i < 289; i++)
  {
    last24hoursTemps[i] = tempKKint;
  }

  Blynk.setProperty(V4, "label", "Keaton");
}

void recordTempToArray()
{
  if (arrayIndex < 289)                   // Mess with array size and timing to taste!
  {
    last24hoursTemps[arrayIndex] = tempKKint;
    ++arrayIndex;
  }
  else
  {
    arrayIndex = 0;
  }

  for (int i = 0; i < 289; i++)
  {
    if (last24hoursTemps[i] > last24high)
    {
      last24high = last24hoursTemps[i];
    }

    if (last24hoursTemps[i] < last24low)
    {
      last24low = last24hoursTemps[i];
    }
  }

  Blynk.setProperty(V4, "label", String("Keaton ") + last24high + "/" + last24low);  // Sets label with high/low temps.
}

BLYNK_WRITE(V19)
{
  int pinData = param.asInt();

  if (pinData == 0)
  {
    Blynk.setProperty(V4, "label", String("Keaton ") + last24high + "/" + last24low);
  }

  if (pinData == 1)
  {
    Blynk.setProperty(V4, "label", String("Keaton ") + dailyHigh + "|" + dailyLow);
  }
}

void resetHiLoTemps()
{
  // Daily at 00:01, yesterday's high/low temps are reset,
  if (hour() == 00 && minute() == 01)
  {
    dailyHigh = 0;     // Resets daily high temp
    dailyLow = 200;    // Resets daily low temp
  }
}

void hiLoTemps()
{
  if (tempKKint > dailyHigh)
  {
    dailyHigh = tempKKint;
  }

  if (tempKKint < dailyLow)
  {
    dailyLow = tempKKint;
  }
}
