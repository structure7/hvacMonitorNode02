#include <SimpleTimer.h>
#define BLYNK_PRINT Serial    // Comment this out to disable prints and save space
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <TimeLib.h>
#define ONE_WIRE_BUS 2

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

DeviceAddress ds18b20kk = { 0x28, 0xEE, 0x9D, 0xEF, 0x00, 0x16, 0x02, 0x56 }; // Bare sensor

char auth[] = "fromBlynkApp";

SimpleTimer timer;

void setup()
{
  Serial.begin(9600);
  Blynk.begin(auth, "ssid", "pw");

  sensors.begin();
  sensors.setResolution(ds18b20kk, 10);

  timer.setInterval(2000L, sendTemps); // Temperature sensor polling interval
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

void loop()
{
  Blynk.run();
  timer.run();
}
