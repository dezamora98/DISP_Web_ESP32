#include "WebServices.h"

bool get_User_LORA(StaticJsonDocument<200> &i_doc)
{
    debugPrintln("preguntando si existe el usuario");
    StaticJsonDocument<200> doc;
    doc["command"] = "getUser";
    doc["username"] = i_doc["username"];
    serializeJson(doc, Serial1);
    doc.clear();

    unsigned long startTime = millis();
    while ((millis() - startTime < 1000))
    {
        if (Serial1.available() > 0)
        {
            String response = Serial1.readStringUntil('\n');
            debugPrintln(response.c_str());
            if (!deserializeJson(doc, response))
            {
                debugPrintln(response.c_str());
                if (i_doc["username"] == doc["username"] && i_doc["password"] == doc["password"])
                {
                    username = doc["username"].as<const char*>();
                    password = doc["password"].as<const char*>();
                    consumed = doc["consumed"].as<uint>();
                    return true;
                }
            }
        }
    }
    return false;
    // pedir datos de servidor por LORA
}

bool send_Consumed_LORA()
{
    debugPrintln("Eviando consumo actualizado por LORA");
    StaticJsonDocument<200> doc;
    doc["command"] = "setConsumed";
    doc["username"] = username;
    doc["consumed"] = consumed;
    serializeJson(doc, Serial1);

    unsigned long startTime = millis();
    while ((millis() - startTime < 1000) && pulseCounter <= adminDocFile["constant"])
    {
        if (Serial1.available() > 0)
        {
            String response = Serial1.readStringUntil('\n');
            if (response == "ack")
            {
                return true;
            }
        }
    }
    return false;
}

bool send_new_User_LORA(StaticJsonDocument<200> &i_doc)
{
    debugPrintln("Ecreando nuevo usuario");
    StaticJsonDocument<200> doc;
    doc["command"] = "addUser";
    doc["username"] = i_doc["username"];
    doc["password"] = i_doc["password"];
    serializeJson(doc, Serial1);

    unsigned long startTime = millis();
    while ((millis() - startTime < 1000) && pulseCounter <= adminDocFile["constant"])
    {
        if (Serial1.available() > 0)
        {
            String response = Serial1.readStringUntil('\n');
            if (response == "ack")
            {
                return true;
            }
        }
    }
    return false;
}

void set_admin(StaticJsonDocument<200> &doc)
{
    adminDocFile["A_user"] = doc["username"];
    adminDocFile["A_pass"] = doc["password"];
    if (!setAdminDocFile())
    {
        currentState = Error;
        send_State();
        return;
    }
    debugPrintln("creddenciales de administrador cambiadas");
}

void set_WifiMode(StaticJsonDocument<200> &doc)
{
    adminDocFile["wifi_SSID"] = doc["ssid"];
    adminDocFile["wifi_pass"] = doc["password"];
    adminDocFile["mode"] = doc["mode"];
    if (!setAdminDocFile())
    {
        currentState = Error;
        send_State();
        return;
    }
    WifiInit(doc["mode"].as<uint>(), doc["ssid"], doc["password"]);
    debugPrintln("configuración de wifi cambiada");
}

void set_constant(StaticJsonDocument<200> &doc)
{
    adminDocFile["constant"] = doc["constant"];
    if (!setAdminDocFile())
    {
        currentState = Error;
        send_State();
        return;
    }
    debugPrintln("constante cambiada");
}

void send_State()
{
    StaticJsonDocument<200> doc;
    doc["state"] = (uint32_t)currentState;
    if (currentState == Error || currentState == Authenticate)
    {
        username = "";
        password = "";
        consumed = 0;
        dosed = 0;
        quantity = 0;
        currentState = Authenticate;
    }
    doc["quantity"] = quantity;
    doc["dosumed"] = dosed;
    doc["consumed"] = consumed;
    doc["username"] = username;
    String stOut;
    serializeJson(doc, stOut);
    debugPrintln(("Response:\n" + stOut).c_str());
    ws.textAll(stOut);
}

void dosedTask(void *pvParameters)
{
    attachInterrupt(digitalPinToInterrupt(pinSensor), IntSensor, FALLING);
    digitalWrite(pinRelay,LOW);
    while (dosed < quantity && currentState == Start)
    {
        if (pulseCounter >= adminDocFile["constant"].as<uint>())
        {
            ++dosed;
            ++consumed;
            pulseCounter = 0;
            debugPrintln("dosed = " + (String)dosed);
            if (!send_Consumed_LORA())
            {
                digitalWrite(pinRelay,HIGH);
                currentState = Error;
                break;
            }
            send_State();
        }
    }
    digitalWrite(pinRelay,HIGH);
    detachInterrupt(digitalPinToInterrupt(pinSensor));
    dosed = 0;
    if (currentState == Start)
    {
        currentState = WaitForStart;
    }
    send_State();
    vTaskDelete(NULL);
}

bool check_admin(StaticJsonDocument<200> i_doc)
{
    if (!getAdminDocFile())
    {
        return false;
    }
    String A_user = adminDocFile["A_user"].as<const char *>();
    String A_pass = adminDocFile["A_pass"].as<const char *>();
    String i_user = i_doc["username"].as<const char *>();
    String i_pass = i_doc["password"].as<const char *>();

    if (A_user == i_user && A_pass == i_pass)
    {
        return true;
    }
    return false;
}

void get_State(AsyncWebServerRequest *request)
{
    StaticJsonDocument<200> doc;
    doc["state"] = (uint32_t)currentState;
    doc["quantity"] = quantity;
    doc["dosumed"] = dosed;
    doc["consumed"] = consumed;
    doc["username"] = username;
    String stOut;
    serializeJson(doc, stOut);

    request->send(200, "application/json", stOut);
}

void ProcessorAdminMode(StaticJsonDocument<200> &doc)
{
    switch (doc["option"].as<uint>())
    {
    case addUser:
        debugPrintln("Enviando nuevo usuario al servidor");
        if (!send_new_User_LORA(doc))
        {
            currentState = Error;
            debugPrintln("error al crear nuevo usuario");
            send_State();
        }
        break;
    case setAdmin:
        debugPrintln("cambiando usuario y contraseña");
        set_admin(doc);
        break;
    case setConstant:
        debugPrintln("cambiando constante de cálculo de flujo");
        set_constant(doc);
        break;
    case setWifiMode:
        debugPrintln("cambiando configuración de wifi");
        set_WifiMode(doc);
        break;
    default:
        currentState = Error;
        debugPrintln("Error: options");
        send_State();
        break;
    }
}

void F_Autenticate(StaticJsonDocument<200> &doc)
{
    if (currentState != Authenticate)
    {
        username = "";
        password = "";
        consumed = 0;
        dosed = 0;
        quantity = 0;
        currentState = Authenticate;
        send_State();
        return;
    } // si el cervidor no se encuentra en el estado Authenticate ir a error;
    debugPrintln("State: Authenticate");

    if (check_admin(doc))
    {
        debugPrintln("New State: Admin");
        currentState = Admin;
        send_State();
        return;
    }

    if (!get_User_LORA(doc))
    {
        debugPrintln("Error: Authenticate");
        send_State();
        return;
    }

    debugPrintln("New State: waitForStart");
    currentState = WaitForStart;
    send_State();
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    if (!(info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT))
    {
        debugPrint("\n 5- Error de formato JSON.\n");
        currentState = Error;
        ws.textAll("{\"state\":\"Error\",\"type\":\"msg\"}");
        return;
    }

    // data[len] = 0;
    debugPrintln("Data:");
    debugPrintln(String(data, len).c_str());

    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, String(data, len));
    if (error || !doc.containsKey("state"))
    {
        debugPrint("\n 5- Error de formato JSON.\n");
        currentState = Error;
        ws.textAll("{\"state\":\"Error\",\"type\":\"json\"}");
        return;
    }

    debugPrintln("WebSocket MSG OK:");
    debugPrintln(String(data, len).c_str());

    switch (doc["state"].as<uint>())
    {
    case Authenticate:
        F_Autenticate(doc);
        break;

    case WaitForStart:
        currentState = WaitForStart;
        if (doc.containsKey("quantity"))
        {
            quantity = doc["quantity"].as<uint>();
            send_State();
            break;
        }
        send_State();
        break;
    case Start:
        if (currentState == WaitForStart)
        {
            xTaskCreate(dosedTask, "dosedTask", 2048, NULL, 1, NULL);
            currentState = Start;
            send_State();
            break;
        }
        currentState = Error;
        debugPrintln("Error: currentState != WaitForStart");
        send_State();
        break;
    case Admin:
        ProcessorAdminMode(doc);
        break;
    default:
        currentState = Error;
        debugPrintln("Error: State");
        send_State();
        break;
    }
}