/*
 * IOT Cappuccino machine webpage and automation
 *
 * reads temperature via a Dallas temperature sensor /temp
 * reflects temperature via a neopixel LED ring
 * depresses power button via a servo, /on
 * serves a webpage for reading temperature and cycling power
 * allows update of arduino program via ESP8266HttpUpdateServer /update
 */
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include <Servo.h>

// thermometer
#include <OneWire.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS D3
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
float temp=0;
unsigned long lastGetTemp=0;

// wifi
const char* host= "cappuccino";
const char* ssid = "Tux24";
const char* password = "a47a47a47a";
MDNSResponder mdns;
ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;
unsigned long lastHandleClient=0;

// servo
#define SERVO_PIN D4
Servo myservo;

// led ring
#include <NeoPixelBus.h>
#define pixelCount 16 
#define LED_STRIP_PIN D2
int brightness=255;
int min_brightness=50;
int max_brightness=255;
int step_brightness=2;
NeoPixelBus strip = NeoPixelBus(pixelCount, LED_STRIP_PIN);
RgbColor red = RgbColor(255, 0, 0);
RgbColor green = RgbColor(0, 255, 0);
RgbColor blue = RgbColor(0, 0, 255);
RgbColor white = RgbColor(255);
RgbColor black = RgbColor(0);

// debug, set to false to turn off all serial messages
boolean debug=false;

void setup(void){
	// led strip
	strip.Begin();
	strip.Show();

	if (debug) Serial.begin(115200);
	pinMode(BUILTIN_LED, OUTPUT);

	// start wifi, get IP
	WiFi.mode(WIFI_AP_STA);
	WiFi.begin(ssid, password);
	
	if (debug) Serial.println("");

	// start thermometer
	sensors.begin();
	
	// Wait for wifi connection
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		if (debug) Serial.print(".");
	}
	if (debug) {
		Serial.println("");
		Serial.print("Connected to ");
		Serial.println(ssid);
		Serial.print("IP address: ");
		Serial.println(WiFi.localIP());
	}

	// register hostname on network
	if (mdns.begin(host, WiFi.localIP())) {
		if (debug) Serial.println("MDNS responder started");
	}

	// establish http bootloader updater
	httpUpdater.setup(&server);

	server.on("/", handleRoot);
	server.on("/on", handleOn);
	server.on("/temp", handleTemp);
	server.onNotFound(handleNotFound);
	
	server.begin();
	if (debug) Serial.println("HTTP server started");

	// move servo so reboot will be recognized
	myservo.attach(SERVO_PIN);
	myservo.write(0);
	delay(800);
	myservo.write(100);
	delay(800);
	myservo.detach();

	// get an initial temp reading
	sensors.requestTemperatures();
	temp=sensors.getTempCByIndex(0);
}
 
void loop(void){
	if (WiFi.status() != WL_CONNECTED) {
		WiFi.begin(ssid, password);
		delay(500);
	} else {
		// handle webpages every x ms
		if (millis() - lastHandleClient > 10){
			server.handleClient();
			lastHandleClient=millis();
		}

		// update temp every x seconds, it takes a second or so
		if (millis() > lastGetTemp+3000){
			sensors.requestTemperatures();
			temp=sensors.getTempCByIndex(0);
			lastGetTemp=millis();

			// update led to reflect new temp
			for (int i=0; i<16; i++){
				int leds=map(temp, 0, 100, 0, 16);

				brightness=brightness+step_brightness;
				if (brightness>max_brightness){
					brightness=max_brightness;
					step_brightness=step_brightness*-1;
				}
				if (brightness<min_brightness){
					brightness=min_brightness;
					step_brightness=step_brightness*-1;
				}

				if (i<=leds){
					if (leds>10) {
						// red=hot
						strip.SetPixelColor(i, 255, 0, 0);
					} else if (leds>5) {
						// green=medium
						strip.SetPixelColor(i, 0, 255, 0);
					} else {
						// blue=cold
						strip.SetPixelColor(i, 0, 0, 255);
					}
				} else {
					strip.SetPixelColor(i, 0, 0, 0);
				}
			}
			strip.Show();
		}
	}
} 

/*
 * handleRoot
 *
 * create and transmit a webpage that has a temperature gauge.
 * the gauge should automatically refresh every x seconds via a JS setInterval
 * the gauge gets its refresh temperature value from an ajax call back to /temp
 * the entire gauge div is a hyperlink to /on, which should then redirect back to the root
 */
void handleRoot() {
	String mytemp=String(temp);
	String o="<html><head><title>Cappuccino Machine</title>\n\
<script src=\"https://cdnjs.cloudflare.com/ajax/libs/raphael/2.1.4/raphael-min.js\"></script>\n\
<script src=\"https://cdnjs.cloudflare.com/ajax/libs/justgage/1.2.2/justgage.min.js\"></script>\n\
</head>\n\
<body style='font-size:80px;'>\n\
<a href='/on'><div id=\"gauge\" class=\"200x160px\"></div></a>\n\
<script>\n\
	// create temperature gauge from open-source library at justgage.com\n\
	var g=new JustGage({\
							id:\"gauge\",\
							customSectors: [{\
								color:\"#00ff00\",\
								lo:0,\
								hi:50\
							},{\
								color:\"#ff0000\",\
								lo:50,\
								hi:100}],\
							counter:true,\
							value:"+mytemp+",\
							min:0,\
							max:100,\
							title:\"Cappuccino Temp (C)\"\
						});\n\
	setInterval(function() {\n\
		var xmlhttp;\n\
		if (window.XMLHttpRequest){\n\
			xmlhttp=new XMLHttpRequest();\n\
			xmlhttp.onreadystatechange=function(){\n\
				if (xmlhttp.readyState==4 && xmlhttp.status==200){\n\
					g.refresh(xmlhttp.responseText);\n\
				}\n\
			}\n\
			xmlhttp.open(\"GET\", \"/temp\", true);\n\
			xmlhttp.send();\n\
		}\n\
	}, 2000);\n\
</script>\n\
<body></html>";
	server.send(200, "text/html", o);
}

/*
 * handleTemp
 *
 * send the latest temperature reading back to browser. this is used by the ajax in handleRoot
 */
void handleTemp() {
	String o=String(temp);
	server.send(200, "text/html", o);
}

/*
 * handleOn
 *
 * send the browser back to the root page, and cycle the servo to depress the power button
 */
void handleOn() {
	// send browser back to root
	String o="<html><head><meta http-equiv=\"Refresh\" content=\"0; url=/\" /></head></html>";
	server.send(200, "text/html", o);

	// cycle servo
	myservo.attach(SERVO_PIN);
	delay(500);
	myservo.write(170);
	delay(800);
	digitalWrite(BUILTIN_LED, LOW);
	myservo.write(100);
	delay(800);
	myservo.detach();
}

/*
 * handleNotFound
 *
 * tell the browser that the page is not found
 */
void handleNotFound(){
	server.send(404, "text/plain", "File not found");
}
 
/*
 * blinkLED
 *
 * blink the LED x times
 */
void blinkLED(int blinkcount=1) {
	int i=0;
	for (i=0; i<16; i++) strip.SetPixelColor(i, 0, 0, 0); strip.Show();
	delay(500);
	for (int thisblink=1; thisblink<blinkcount; thisblink++){
		for (i=0; i<16; i++) strip.SetPixelColor(i, 255, 255, 255); strip.Show();
		delay(250);
		for (i=0; i<16; i++) strip.SetPixelColor(i, 0, 0, 0); strip.Show();
		delay(250);
	}
}
