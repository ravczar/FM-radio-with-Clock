# FM-radio-with-Clock

This is my own home made project based on:

->bare AVR Atmega328, 
->FM tuner TEA5767, 
->PAM8403 amplifier, 
->real time clock - tiny,  
->LCD 16x02a, 
->7 segment, 4 digit display. 

Offers:
->Scanning Up/Down, 
-> programmed frequencies Up/Down, 
-> memorizes last frequency and loads in when turned on again, 
-> lihium CR2032 3.3V for time keeping.
-> adjusting time (when radio is 4 main buttons adjust hours and minutes, each adjustment zeroes second's count)
-> Low power consumption
-> Logic schema is available both in Kicad file and PDF. 

Feel free to use it for your own non-commercial purposes. I tried to use the cheapest parts available. 
Please consider swapping LCD for nokia 5110 display.

PROS:
+Stereo / Mono auto adjust to reduce noises.
+Sound is clean, tuner feels best at 3.3-4.0 [V]
+Amp offers no distortion, it does not overheat at this setup.
+Radio is very responsive. Works with no problem since 02/2019 on daily basis.
+Clock is the cheapest real time clock module i found, but it is quite acurate as long as it has stable voltage 
and you dont maniupulate with putting your AVR into deep sleep
+Most of devices on I2C connection type. Reduced wiring to minimum.
+adjustable 4 digit display with just one value to be changed in the.

CONS:
-No RDS scanning - if required use another chip. E.G. FM RDA5807M (quality unknown)
-It sounds fine, but does not look good
-Needs external programmer for the chip - best USBASP so plan your ICSP port on the motherboard or goldpins next to the ATMEL chip

Have fun and feel free to share/use/build/edit code.
