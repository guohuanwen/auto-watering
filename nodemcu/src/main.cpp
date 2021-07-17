#include <Arduino.h>
#include "ESP8266WiFi.h"
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>
#include "html.h"

#define LED D0
#define PUMP D1
#define WIFI_NAME "HUAWEI-2333-2.4G"
#define WIFI_PASSWORD "guohuanwen2333"

#define EEPROM_BEGIN 4096
int addr = 0; //EEPROM数据地址

ESP8266WebServer server(80);

void openPump() {
    digitalWrite(PUMP, 1);
    server.send(200, "text/plain", "ok");
}

void closePump() {
    digitalWrite(PUMP, 0);
    server.send(200, "text/plain", "ok");
}

void openLight() {
    digitalWrite(LED, 1);
    server.send(200, "text/plain", "ok");
}

void closeLight() {
    digitalWrite(LED, 0);
    server.send(200, "text/plain", "ok");
}

void handlerOpenLight() {
    openLight();
    server.send(200, "text/plain", "ok");
}

void handlerCloseLight() {
    closeLight();
    server.send(200, "text/plain", "ok");
}

void handlerOpenPump() {
    openPump();
    server.send(200, "text/plain", "ok");
}

void handlerClosePump() {
    closePump();
    server.send(200, "text/plain", "ok");
}

void handleRoot() {
    server.send(200, "text/html", HTML);
}

void handlerConfirm() {
    String time = server.arg("time");
    String repeat = server.arg("repeat");
    Serial.println(time);
    Serial.println(repeat);
    server.send(200, "text/plain", "ok");
}

void handleNotFound() {
    digitalWrite(LED, 1);
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += (server.method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";
    for (uint8_t i = 0; i < server.args(); i++) {
        message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
    server.send(404, "text/plain", message);
    digitalWrite(LED, 0);
}

void initServer() {
    if (MDNS.begin("esp8266")) {
        Serial.println("MDNS responder started");
    }

    server.on("/", handleRoot);
    server.on("/openLight", handlerOpenLight);
    server.on("/openPump", handlerOpenPump);
    server.on("/closeLight", handlerCloseLight);
    server.on("/closePump", handlerClosePump);
    server.on("/confirm", handlerConfirm);

    server.onNotFound(handleNotFound);

    server.begin();
    Serial.println("HTTP server started");
}

void initWifi() {
    WiFi.begin(WIFI_NAME, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");
}

void loopServer() {
    server.handleClient();
}


void write() {
    Serial.println("Start write");
    EEPROM.begin(EEPROM_BEGIN); //size为需要读写的数据字节最大地址+1，取值4~4096
    for (addr = 0; addr < EEPROM_BEGIN; addr++) {
        int data = addr % 256; //在该代码中等同于int data = addr;因为下面write方法是以字节为存储单位的
        EEPROM.write(addr, data); //写数据
    }
    EEPROM.commit(); //保存更改的数据
    Serial.println("End write");
}

void read() {
    EEPROM.begin(EEPROM_BEGIN); //申请操作到地址4095（比如你只需要读写地址为100上的一个字节，该处也需输入参数101）
    for (addr = 0; addr < EEPROM_BEGIN; addr++) {
        int data = EEPROM.read(addr); //读数据
        Serial.print(data);
        Serial.print(" ");
        delay(2);
        if ((addr + 1) % 256 == 0) {//每读取256字节数据换行
            Serial.println("");
        }
    }
}

void setup() {
    pinMode(LED, OUTPUT);
    pinMode(PUMP, OUTPUT);
    Serial.begin(76800);
    initWifi();
    initServer();
}

void loop() {
    loopServer();
}