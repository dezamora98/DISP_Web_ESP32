#include <freertos/FreeRTOS.h>
#include <DNSServer.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <SPIFFS.h>
#include "Debug_custom.h"

#define AP_MODE_C 0
#define WIFI_MODE_C 1

typedef enum
{
    Authenticate = 0,
    WaitForStart = 1,
    Start = 2,
    Error = 3,
    Admin = 4,
} State;

typedef enum
{
    addUser = 0,
    setAdmin = 1,
    setConstant = 2,
    setWifiMode = 3,
} Option;

TaskHandle_t taskHandle; // tasks manager

String username = "";
String password = "";
uint32_t constant_PL = 300;
uint32_t dosed = 0;
uint32_t quantity = 0;
uint32_t consumed = 0;
uint32_t currentState = Authenticate;

DNSServer dnsServer;
String AP_ssid = "Kraftstoffservice";
String AP_password = "";

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
StaticJsonDocument<200> adminDocFile;
const uint pinSensor = 23;
volatile uint32_t pulseCounter = 0;
const uint pinRelay = 22;

const long timeoutInterval = 60000; // 60 segundos
unsigned long lastGetRequestTime = 0;

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);
void send_State();

void IRAM_ATTR IntSensor()
{
    ++pulseCounter;
}

inline void WifiInit(uint ap_wifi = AP_MODE_C, String Wifi_SSID = "Kraftstoffservice", String Wifi_Password = "12345678")
{
    if (ap_wifi == WIFI_MODE_C)
    {
    }
    else
    {
        WiFi.softAPConfig(IPAddress(192, 168, 1, 1), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0));
        WiFi.softAP(Wifi_SSID, Wifi_Password, 1, 0, 1);
    }
}

bool getAdminDocFile()
{
    File admin = SPIFFS.open("/admin.json", FILE_READ);
    if (!admin)
    {
        debugPrintln("ERROR en fichero de credenciales");
        admin.close();
        return false;
    }

    String s_admin;
    while (admin.available())
    {
        s_admin += (char)admin.read();
    }

    admin.close();

    debugPrintln("ficehro de credenciales:");
    s_admin.remove(s_admin.length());
    debugPrintln(s_admin.c_str());
    DeserializationError error = deserializeJson(adminDocFile, s_admin);
    if (error)
    {
        debugPrint("\n 5- Error de formato JSON de fichero de credenciales.\n");
        return false;
    }
    return true;
}
bool setAdminDocFile()
{
    File file = SPIFFS.open("/admin.json", "w");
    if (!file)
    {
        debugPrintln("Error al abrir el archivo para escritura");
        return false;
    }

    String stDoc;
    serializeJson(adminDocFile, stDoc);
    debugPrintln("Escribiendo fichero de configuración:");
    debugPrintln(stDoc);
    if (file.print(stDoc))
    {
        Serial.println("Archivo escrito");
    }
    else
    {
        Serial.println("Error al escribir en el archivo");
        return false;
    }
    file.close();
    return true;
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len)
{
    lastGetRequestTime = millis();
    switch (type)
    {
    case WS_EVT_CONNECT:
        Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
        send_State();
        break;
    case WS_EVT_DISCONNECT:
        Serial.printf("WebSocket client #%u disconnected\n", client->id());
        break;
    case WS_EVT_DATA:
        handleWebSocketMessage(arg, data, len);
        break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
        break;
    }
}

inline void WebManagerInit()
{
    server.onNotFound([](AsyncWebServerRequest *request)
                      { debugPrintln(("Redireccionando a = http://"+WiFi.softAPIP().toString()).c_str());
                        request->redirect("http://" + WiFi.softAPIP().toString() + "/"); });

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                lastGetRequestTime = millis();
                if (currentState == Error || currentState == Authenticate)
                {
                    request->send(SPIFFS, "/authPage.html", String());
                }
                else if (currentState == Admin)
                {
                     request->send(SPIFFS, "/adminPage.html", String());
                }
                else
                {
                    request->send(SPIFFS, "/mainPage.html", String());
                } });

    server.on("/authScript.js", HTTP_GET, [](AsyncWebServerRequest *request)
              { 
                lastGetRequestTime = millis();                  
                if (currentState == Error || currentState == Authenticate)
                {
                    request->send(SPIFFS, "/authScript.js", "text/javascript");          
                }
                else
                {
                    request->send(404, "text/plain", "Not found");
                } });

    server.on("/mainScript.js", HTTP_GET, [](AsyncWebServerRequest *request)
              { 
                lastGetRequestTime = millis();
                if (currentState == Start || currentState == WaitForStart)
                {
                    request->send(SPIFFS, "/mainScript.js", "text/javascript");
                }
                else
                {
                    request->send(404, "text/plain", "Not found");
                } });

    server.on("/adminScript.js", HTTP_GET, [](AsyncWebServerRequest *request)
              { 
                lastGetRequestTime = millis();
                if (currentState == Admin)
                {
                    request->send(SPIFFS, "/adminScript.js", "text/javascript");
                }
                else
                {
                    request->send(404, "text/plain", "Not found");
                } });

    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/style.css", "text/css"); });

    server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/favicon.ico", "image/x-icon"); });

    ws.onEvent(onEvent);
    server.addHandler(&ws);
    server.begin();
}
