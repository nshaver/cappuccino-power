# cappuccino-power
Remote control via web (IOT) for cappuccino machine using ESP8266 and standard servo.

Demonstration video: https://www.youtube.com/watch?v=_8TL1clHK6k

This project makes use of the following hardware:
ESP8266 - example: http://www.amazon.com/HiLetgo-Version-NodeMCU-Internet-Development/dp/B010O1G1ES/ref=sr_1_1?s=pc&ie=UTF8&qid=1465259310&sr=1-1&keywords=esp8266

Dallas temperature sensor - example: http://www.amazon.com/DROK-Temperature-Waterproof-Thermometer-Thermistor/dp/B00KLZQ0P8/ref=pd_sim_sbs_201_4?ie=UTF8&dpID=41whJsaK1QL&dpSrc=sims&preST=_AC_UL160_SR160%2C160_&refRID=1WG2Q29GA3V2RPBDKQ77

Adafruit Neopixel LED ring - example: http://www.amazon.com/NeoPixel-LED-Ring-Integrated-driver/dp/B0137JU7OE/ref=sr_1_3?s=hi&ie=UTF8&qid=1465259484&sr=8-3&keywords=adafruit+neopixel+ring

PWM servo, a Hitec perhaps - example: http://www.amazon.com/Hitec-31311S-HS-311-Standard-Universal/dp/B0006O3WVE/ref=sr_1_7?ie=UTF8&qid=1465259620&sr=8-7&keywords=hitec+servo

Be sure to setup the ssid and password for your wifi in the ssid and password variables near the top of the program.

Use serial debugging to determine the IP address once you have it uploaded. That is the IP you'll need to visit with your web browser.

The code supports updates via WiFi using the ESP8266HttpUpdateServer library. To update, visit http://cappuccino.local/update and browse for the bin file from the latest compile (usually in /tmp/buildxxxx).
