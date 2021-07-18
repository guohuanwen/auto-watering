#include <Arduino.h>
#include "ESP8266WiFi.h"
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>
#include "html.h"
#include "WiFiUdp.h"
#include "NTPClient.h"
#include "Ticker.h"
#include <cstdlib>

#define LED D0
#define PUMP D1
#define WIFI_NAME "HUAWEI-2333-2.4G"
#define WIFI_PASSWORD "guohuanwen2333"
#define EEPROM_RUNTIME 4
#define EEPROM_REPEAT 5
#define EEPROM_WATERING 6
#define EEPROM_LAST_DAY 7
#define EEPROM_LAT_HOUR 8

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "ntp1.aliyun.com", 60 * 60 * 8, 60000);//东8区
Ticker ticker;

int runtime = 9;//需要几点浇水
int repeatTime = 24 * 3;//间隔几个小时浇水
int watering = 10;//电机开启时间秒
int lastWateringDay = -1;//上次浇水星期几
int lastWateringHour = -1;//上次浇水小时

ESP8266WebServer server(80);

void write(int addr, int time) {
    Serial.println("Start write");
    EEPROM.begin(addr + 1);//size为需要读写的数据字节最大地址+1，取值4~4096
    EEPROM.write(addr, time); //写数据
    EEPROM.commit(); //保存更改的数据
    Serial.println("End write");
}

int read(int addr) {
    Serial.println("Start read");
    EEPROM.begin(addr + 1);
    int data = EEPROM.read(addr); //读数据
    Serial.println(data);
    Serial.println("End read");
    return data;
}

void autoWatering() {
    Serial.println("autoWatering");
    digitalWrite(PUMP, 1);
    delay(watering * 1000);
    digitalWrite(PUMP, 0);
    lastWateringDay = timeClient.getDay();
    lastWateringHour = timeClient.getHours();
    write(EEPROM_LAST_DAY, lastWateringDay);
    write(EEPROM_LAT_HOUR, lastWateringHour);
}

void timerTask() {
    timeClient.update();
    int dayOfWeek = timeClient.getDay();//0-6
    int hours = timeClient.getHours();

    //还没浇水过
    if (lastWateringHour < 0 || lastWateringDay < 0) {
        if (hours == runtime) {
            autoWatering();
        }
        return;
    }
    //此刻是否需要浇水，一周按小时计算，24 * 7 = 168
    int nowHour = dayOfWeek * 24 + hours;
    int lastWaterHour = lastWateringDay * 24 + lastWateringHour;
    int interval = nowHour - lastWaterHour;
    if (interval < 0) {//跨周了
        interval = 24 * 7 - lastWaterHour + nowHour;
    }
    if (interval > runtime) {//需要浇水了
        autoWatering();
    }
}

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
    String water = server.arg("water");
    int timeInt;
    if (time == "00:00") {
        timeInt = 0;
    } else if (time == "09:00") {
        timeInt = 9;
    } else if (time == "12:00") {
        timeInt = 12;
    } else if (time == "15:00") {
        timeInt = 15;
    } else if (time == "18:00") {
        timeInt = 18;
    } else if (time == "21:00") {
        timeInt = 21;
    } else {
        timeInt = timeClient.getHours();
    }
    runtime = timeInt;
    write(EEPROM_RUNTIME, timeInt);

    int repeatHour;
    if (repeat == "6 hour") {
        repeatHour = 6;
    } else if (repeat == "12 hour") {
        repeatHour = 12;
    } else if (repeat == "1 day") {
        repeatHour = 24;
    } else if (repeat == "2 day") {
        repeatHour = 24 * 2;
    } else if (repeat == "3 day") {
        repeatHour = 24 * 3;
    } else if (repeat == "4 day") {
        repeatHour = 24 * 4;
    } else if (repeat == "5 day") {
        repeatHour = 24 * 5;
    } else if (repeat == "6 day") {
        repeatHour = 24 * 6;
    } else if (repeat == "7 day") {
        repeatHour = 24 * 7;
    } else {
        repeatHour = 24 * 3;
    }
    repeatTime = repeatHour;
    write(EEPROM_REPEAT, repeatHour);

    char** end;
    watering = std::strtol(reinterpret_cast<const char *>(&water), end, 10);
    Serial.println(watering);
    if (watering <= 0) {
        watering = 5;
    }
    write(EEPROM_WATERING, watering);

    //清理上次浇水时间
    lastWateringDay = -1;
    lastWateringHour = -1;
    write(EEPROM_LAST_DAY, lastWateringDay);
    write(EEPROM_LAT_HOUR, lastWateringHour);

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

void setup() {
    pinMode(LED, OUTPUT);
    pinMode(PUMP, OUTPUT);
    Serial.begin(76800);
    initWifi();
    initServer();
    ticker.attach(10, timerTask);
    timeClient.begin();

    runtime = read(EEPROM_RUNTIME);
    repeatTime = read(EEPROM_REPEAT);
    watering = read(EEPROM_WATERING);
    lastWateringDay = read(EEPROM_LAST_DAY);
    lastWateringHour = read(EEPROM_LAT_HOUR);
}

void loop() {
    loopServer();
}