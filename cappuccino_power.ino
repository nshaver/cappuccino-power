#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Servo.h>

// thermometer
#include <OneWire.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS D3
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// wifi
const char* ssid = "Tux24";
const char* password = "a47a47a47a";
MDNSResponder mdns;
ESP8266WebServer server(80);

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

// debug
boolean debug=true;

void setup(void){
	// led strip
	strip.Begin();
	strip.Show();

	if (debug) Serial.begin(115200);
	pinMode(BUILTIN_LED, OUTPUT);

	// start wifi, get IP
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
	
	if (mdns.begin("esp8266", WiFi.localIP())) {
		Serial.println("MDNS responder started");
	}
	
	server.on("/", handleTemp);
	server.on("/off", handleOn);
	server.on("/on", handleOn);
	server.on("/temp", handleTemp);
	
	server.onNotFound(handleNotFound);
	
	server.begin();
	Serial.println("HTTP server started");

	//delay a moment, 
	//for terminal to receive inf, such as IP address
	delay(1000);
	if (debug) Serial.end();

	myservo.attach(SERVO_PIN);
	myservo.write(0);
	delay(800);
	myservo.write(100);
	delay(800);
	myservo.detach();
}
 
void loop(void){
	server.handleClient();

	float temp=0;
	sensors.requestTemperatures();
	temp=sensors.getTempCByIndex(0);

	// write led
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

		if (false){
			// put test pixel set here
			strip.SetPixelColor(i, 0, 0, brightness);
		} else {
			if (i<=leds){
				if (leds>20) {
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
	}
	strip.Show();
} 

void refreshURL(String url){
	sensors.requestTemperatures();
	String message="";
	message +="<html><body style='font-size:80px;'>";
	message +="<a href='/temp'>Temperature: "+String(sensors.getTempCByIndex(0))+"</a><br>";
	message +="<a href='/off'><img src='https://d30y9cdsu7xlg0.cloudfront.net/png/151490-200.png' alt='click to cycle power'></a>";
	message +="<body></html>";
	server.send(200, "text/html", message);
}

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
	delay(500);
}

void handleTemp() {
	blinkLED(2);
	refreshURL("temp");
}

void handleOn() {
	blinkLED(2);
	refreshURL("on");
	// cycle servo
	myservo.attach(SERVO_PIN);
	delay(500);
	myservo.write(170);
	delay(800);
	digitalWrite(BUILTIN_LED, LOW);
	myservo.write(100);
	delay(800);
	myservo.detach();
	blinkLED(2);
}

void handleNotFound(){
	blinkLED(1);
	String message = "File Not Found\n\n";
	message += "URI: ";
	message += server.uri();
	message += "\nMethod: ";
	message += (server.method() == HTTP_GET)?"GET":"POST";
	message += "\nArguments: ";
	message += server.args();
	message += "\n";
	for (uint8_t i=0; i<server.args(); i++){
		message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
	}
	server.send(404, "text/plain", message);
}
 
