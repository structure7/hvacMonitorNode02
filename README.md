# hvacMonitorNode02 (KK)
<p align="center"><img src="http://i.imgur.com/hzyOjQH.jpg"/></p>

## Lessons Learned
###DS18B20: Put a wire on it.
Sooooo... I was getting worried. I love everything about the DS18B20 temperature sensor. Power flexibility, addressing, and bells and whistles that I haven't touched yet. However, I was getting unreliable readings... usually too high. <a href="http://arduino.stackexchange.com/questions/789/my-ds18b20-is-reading-high-how-can-i-get-it-to-return-the-correct-temperature">After doing some reading</a> I soon understood my problem: Heat transmission.<br><br>f
I was very proud of my stubby little temp sensor plugged directly into my hacked baby monitor (above), but it was always giving me temps between 2-5°C higher than other sensors in the same space. I noticed I was getting the same on my solderless breadboard but figured my work area was a little warmer.<br><br>
Now check out this photo... I simply added about 8" of wire to my DS18B20 (CAT5e conductors to be exact):
<p align="center"><img src="http://i.imgur.com/o0QafJa.jpg"/></p><br>
I have reproduced this "phenomenon" with at least 3 sensors so far. Interesting! If adding a length of wire isn't possible for your project, experiment with correct the temperature in code. This should work as long as the "localized heating" of your project is consistent.<br>
## Libraries and Resources

Title | Include | Link | w/ IDE?
------|---------|------|----------
OneWire | OneWire.h | https://github.com/PaulStoffregen/OneWire | No
Arduino-Temperature-Control-Library | DallasTemperature.h | https://github.com/milesburton/Arduino-Temperature-Control-Library | No
Time | Timelib.h | https://github.com/PaulStoffregen/Time
SimpleTimer | SimpleTimer.h | https://github.com/jfturcot/SimpleTimer
ESP8266/Arduino | ESP8266WiFi.h | https://github.com/esp8266/Arduino | No
blynk-library | BlynkSimpleEsp8266.h | https://github.com/blynkkk/blynk-library | No
ESP8266 board mgr | N/A | [json](http://arduino.esp8266.com/stable/package_esp8266com_index.json) & [instructions](https://github.com/esp8266/Arduino#installing-with-boards-manager) | No

Many thanks to all of the people above. [How to edit this.](https://guides.github.com/features/mastering-markdown/)
