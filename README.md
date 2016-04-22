# hvacMonitorNode02 (KK)
<p align="center"><img src="http://i.imgur.com/hzyOjQH.jpg"/></p>

## Lessons Learned
###DS18B20: Put a wire on it.
Sooooo... I was getting worried. I love everything about the DS18B20 temperature sensor. Power flexiblity, addressing, and bells and whistles that I haven't touched yet. However, I was getting unreliable readings... usually too high. After doing some reading I soon understood (or believe to have understood) my problem: Heat transmission.<br>
I was very proud of my stubby little temp sensor plugged directly into my hacked baby monitor above, but it was always giving me temps between 2-5C°. I noticed I was getting the same on my solderless breadboard but figured my work area was a little warmer.<br>
Now check out this photo... I simply added about 8" of wire to my DS18B20 (CAT5e conductors to be exact):
<p align="center"><img src="http://i.imgur.com/o0QafJa.jpg"/></p>
