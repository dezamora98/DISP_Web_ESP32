#include <WiFi.h>
#include "WebManager.h"
#include <SPI.h>
#include <LoRa.h>

void setup()
{
	// Initialize SPIFFS
	if (!SPIFFS.begin(true))
	{
		debugPrintln("An Error has occurred while mounting SPIFFS");
		return;
	}
	if (!getAdminDocFile())
	{
		return;
	}

	Serial.begin(115200);
	Serial1.setTimeout(1000);
	Serial1.begin(9600, SERIAL_8N1, 16, 17);

	pinMode(pinSensor, INPUT_PULLUP);
	pinMode(pinRelay, OUTPUT);
	digitalWrite(pinRelay,HIGH);

	getAdminDocFile();

	WifiInit(adminDocFile["mode"].as<uint>(), adminDocFile["wifi_SSID"], adminDocFile["wifi_pass"]);
	dnsServer.start(53, "*", WiFi.softAPIP());

	// Route for root / web page
	WebManagerInit();
}

void loop()
{
	if ((WiFi.softAPgetStationNum() != 1) || ((millis() - lastGetRequestTime >= timeoutInterval) && currentState != Start))
	{
		username = "";
		password = "";
		consumed = 0;
		dosed = 0;
		quantity = 0;
		currentState = Authenticate;
		lastGetRequestTime = millis();
	}
	dnsServer.processNextRequest();
}