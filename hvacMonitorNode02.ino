#include <SimpleTimer.h>
#define BLYNK_PRINT Serial    // Comment this out to disable prints and save space
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <TimeLib.h>
#define ONE_WIRE_BUS 0

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

DeviceAddress ds18b20kk = { 0x28, 0xEE, 0x9D, 0xEF, 0x00, 0x16, 0x02, 0x56 }; // KK

char auth[] = "fromBlynkApp";

SimpleTimer timer;

WidgetLED led1(V10);

void setup()
{
  Serial.begin(9600);
  Blynk.begin(auth, "ssid", "pw");

  WiFi.softAPdisconnect(true); // Per https://github.com/esp8266/Arduino/issues/676 this turns off AP

  sensors.begin();
  sensors.setResolution(ds18b20kk, 10);

  timer.setInterval(2000L, sendTemps); // Temperature sensor polling interval
  timer.setInterval(5000L, heartbeatOn); // Temperature sensor polling interval
}

void heartbeatOn()  // Blinks a virtual LED in the Blynk app to show the ESP is live and reporting.
{
  led1.on();
  timer.setTimeout(2500L, heartbeatOff);
}

void heartbeatOff()
{
  led1.off();  // The OFF portion of the LED heartbeat indicator in the Blynk app
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
  led1.off();
}

void loop()
{
  Blynk.run();
  timer.run();
}
