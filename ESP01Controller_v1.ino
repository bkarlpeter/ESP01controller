#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <OneWire.h>
#include <DallasTemperature.h>
/*
LEDController mit ESP01 (Quelle: Github Copilot)
Aufruf des lokalen Netzes: http://ledcontrol1.local/
Danach die WLAN credentials setzen
Ab jetzt mit OTA updaten

Anschluss:
Ein FET für die LED Contrller
Ein Tempsensor für temp kontrolle

Versionen:
v0 - erste funktionfähige Version, einfache Website
*/

// Pins
const int fetPin = 0;      // GPIO0 (FETs)
const int tempPin = 2;     // GPIO2 (DS18B20)

// DS18B20 Setup
OneWire oneWire(tempPin);
DallasTemperature sensors(&oneWire);

// Einstellungen
const char* apName = "LEDcontrol1";
const char* mdnsName = "ledcontrol1";
int pwmValue = 0;     // 0–1023
float tempC = 0.0;

const char* ssid = "DEIN_SSID";
const char* password = "DEIN_PASSWORT";


ESP8266WebServer server(80);

void updateFET() {
  analogWrite(fetPin, pwmValue);
  Serial.println("Set PWM: " + String(pwmValue));
}

void handleRoot() {
  String html = "<html><head><title>LEDcontrol1</title></head><body>";
  html += "<h2>Helligkeit (PWM) aller FETs</h2>";
  html += "<form action='/set' method='get'>";
  html += "PWM: <input type='range' min='0' max='1023' name='pwm' value='" + String(pwmValue) + "' onchange='this.nextElementSibling.value=this.value'><output>"+String(pwmValue)+"</output><br>";
  html += "<input type='submit' value='Set'>";
  html += "</form>";
  html += "<h2>Temperatur</h2>";
  html += "Aktuell gemessen: <b>" + String(tempC, 1) + " &deg;C</b><br>";
  html += "<br><a href=\"/wifimanager\">WLAN-Einstellungen ändern</a>";
  html += "<br><b>OTA:</b> Firmware-Update im Arduino IDE unter 'Werkzeuge → Port → " + String(mdnsName) + " (OTA)'";
  html += "<script>var out=document.querySelector('output');document.querySelector('input[type=range]').oninput=function(){out.value=this.value;}</script>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleSet() {
  if (server.hasArg("pwm")) pwmValue = constrain(server.arg("pwm").toInt(), 0, 1023);
  updateFET();
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleWiFiManager() {
  WiFiManager wm;
  wm.startConfigPortal(apName);
}

void setup() {
  Serial.begin(115200);
  pinMode(fetPin, OUTPUT);
  updateFET();

  WiFiManager wm;
  wm.autoConnect(apName);

  if (MDNS.begin(mdnsName)) {
    Serial.print("mDNS gestartet: http://");
    Serial.print(mdnsName);
    Serial.println(".local/");
  }

  // OTA initialisieren
  ArduinoOTA.setHostname(mdnsName);
  ArduinoOTA.begin();

  // DS18B20 initialisieren
  sensors.begin();

  server.on("/", handleRoot);
  server.on("/set", handleSet);
  server.on("/wifimanager", handleWiFiManager);
  server.begin();
  Serial.println("Setup finished, wait for WebData");
}

void loop() {
  server.handleClient();
  MDNS.update();
  ArduinoOTA.handle();

  static unsigned long lastTemp = 0;
  if (millis() - lastTemp > 2000) { // Alle 2 Sekunden Temperatur abfragen
    sensors.requestTemperatures();
    tempC = sensors.getTempCByIndex(0);
    Serial.print("Temp: ");
    Serial.println(String(tempC) + "°C");
    lastTemp = millis();
  }
}