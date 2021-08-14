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
#include <DHT.h>
#include "LiquidCrystal_I2C.h"

#define LED D0
#define SCREEN_SCL D1
#define SCREEN_SDA D2
#define SOIL_AO A0
#define SOIL_DO D4
#define PUMP D5
#define DHT_PIN D3

#define WIFI_NAME "HUAWEI-2333-2.4G"
#define WIFI_PASSWORD "guohuanwen2333"
#define EEPROM_RUNTIME 4
#define EEPROM_REPEAT 5
#define EEPROM_WATERING 6
#define EEPROM_LAST_DAY 7
#define EEPROM_LAT_HOUR 8

static int DEFAULT_TIME = 255;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "ntp1.aliyun.com", 60 * 60 * 8, 60000);//东8区
Ticker ticker;

Ticker dhtTicker;
DHT dht(DHT_PIN, DHT11);
float temperature, humidity;

Ticker screenTicker;
LiquidCrystal_I2C screen(0x27, 16, 2);
bool openScreen = true;

Ticker wateringTicker;
int startTime = 9;//第一次几点浇水
int repeatTime = 24 * 3;//间隔几个小时浇水
int watering = 10;//电机开启时间秒
int lastWateringDay = 0;//上次浇水星期几
int lastWateringHour = 0;//上次浇水小时
int initTimeCount = 0;

int soil;

ESP8266WebServer server(80);

void write(int addr, int time) {
    EEPROM.begin(addr + 1);//size为需要读写的数据字节最大地址+1，取值4~4096
    EEPROM.write(addr, time); //写数据
    EEPROM.commit(); //保存更改的数据
}

int read(int addr) {
    EEPROM.begin(addr + 1);
    int data = EEPROM.read(addr); //读数据
    return data;
}

int readSoilDO() {
    return digitalRead(SOIL_DO);
}

int readSoilAO() {
    soil = analogRead(SOIL_AO);
    return soil;
}

void refreshScreen() {
    screen.clear();
    screen.setCursor(0, 0);
    screen.print(readSoilAO());
    screen.setCursor(0, 1);
    screen.print(readSoilDO());

    screen.setCursor(5, 0);
    screen.print(temperature);
    screen.setCursor(5, 1);
    screen.print(humidity);

    screen.setCursor(11, 0);
    screen.print(lastWateringDay);
    screen.setCursor(11, 1);
    screen.print(lastWateringHour);
}

void initDht11() {
    dht.begin();
}

void closeWatering() {
    Serial.println(String() + "closeWatering");
    digitalWrite(PUMP, 0);
    wateringTicker.detach();
}

void autoWatering() {
    Serial.println(String() + "autoWatering" + watering);
    digitalWrite(PUMP, 1);
    wateringTicker.attach(watering, closeWatering);
    lastWateringDay = timeClient.getDay();
    lastWateringHour = timeClient.getHours();
    write(EEPROM_LAST_DAY, lastWateringDay);
    write(EEPROM_LAT_HOUR, lastWateringHour);
}

void timerTask() {
    timeClient.update();

    //开始时间会不准，5次调用之后再开始使用
    if (initTimeCount < 6) {
        initTimeCount++;
        Serial.println(String() + " initTimeCount " + initTimeCount);
        return;
    }

    int dayOfWeek = timeClient.getDay();//0-6
    int hours = timeClient.getHours();
    Serial.println(String() + "startTime: " + startTime + " ,repeatHour: " + repeatTime);
    Serial.println(String() + "current dayOfWeek: " + dayOfWeek + " ,hours: " + hours);
    Serial.println(String() + "last dayOfWeek: " + lastWateringDay + " ,hours: " + lastWateringHour);

    //还没浇水过，等待设置的浇水时间
    if (lastWateringHour == DEFAULT_TIME || lastWateringDay == DEFAULT_TIME) {
        if (hours == startTime) {
            autoWatering();
        }
        return;
    }
    //此刻是否需要浇水，一周按小时计算，24 * 7 = 168
    int nowHour = dayOfWeek * 24 + hours;
    int lastWaterHour = lastWateringDay * 24 + lastWateringHour;
    int interval = nowHour - lastWaterHour;
    if (interval < 0) {//跨周了
        interval = 24 * 7 + (nowHour - lastWaterHour);
    }
    if (interval >= repeatTime) {//需要浇水了
        autoWatering();
    }
}

void openPump() {
    Serial.println("openPump");
    digitalWrite(PUMP, 1);
    server.send(200, "text/plain", "ok");
}

void closePump() {
    Serial.println("closePump");
    digitalWrite(PUMP, 0);
    server.send(200, "text/plain", "ok");
}

void openLight() {
    Serial.println("openLight");
    digitalWrite(LED, 1);
    openScreen = true;
    screen.backlight();
    server.send(200, "text/plain", "ok");
}

void closeLight() {
    Serial.println("closeLight");
    digitalWrite(LED, 0);
    openScreen = false;
    screen.noBacklight();
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
    startTime = timeInt;
    write(EEPROM_RUNTIME, timeInt);

    int repeatHour;
    if (repeat == "1 hour") {
        repeatHour = 1;
    } else if (repeat == "2 hour") {
        repeatHour = 2;
    } else if (repeat == "3 hour") {
        repeatHour = 3;
    } else if (repeat == "6 hour") {
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
    lastWateringDay = DEFAULT_TIME;
    lastWateringHour = DEFAULT_TIME;
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

void handlerGetAll() {
    String msg = "";
    msg += "{\"start_time\":";
    msg += startTime;
    msg += ",";
    msg += "\"repeat_time\":";
    msg += repeatTime;
    msg += ",";
    msg += "\"watering\":";
    msg += watering;
    msg += ",";
    msg += "\"temperature\":";
    msg += temperature;
    msg += ",";
    msg += "\"humidity\":";
    msg += humidity;
    msg += ",";
    msg += "\"soil\":";
    msg += soil;
    msg += "}";
    server.send(200, "text/plain", msg);
}

void initSoil() {
    pinMode(SOIL_DO, INPUT);
    pinMode(SOIL_AO, INPUT);
}

void initScreen() {
    screen.init();                      // initialize the lcd
    screen.backlight();
    screen.print("hello bigwen");
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
    server.on("/getAll", handlerGetAll);

    server.onNotFound(handleNotFound);

    server.begin();
    Serial.println("HTTP server started");
}

void initWifi() {
    screen.clear();
    screen.setCursor(0, 0);
    screen.print("WiFi connecting");
    WiFi.begin(WIFI_NAME, WIFI_PASSWORD);
    int index = 0;
    String connecting = "";
    screen.setCursor(0, 1);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        connecting+=".";
        if (connecting.length() > 6) {
            connecting = ".";
        }
        screen.print(connecting);
        index++;
    }
    Serial.println("WiFi connected");
    screen.clear();
    screen.setCursor(0, 0);
    screen.print("WiFi connected");
    screen.setCursor(0, 1);
    screen.print(WiFi.localIP());
}

void loopServer() {
    server.handleClient();
}

void readDht() {
    temperature = dht.readTemperature();
    humidity = dht.readHumidity();
}

void setup() {
    Serial.begin(76800);
    pinMode(LED, OUTPUT);
    pinMode(PUMP, OUTPUT);
    digitalWrite(PUMP, 0);
    initDht11();
    initScreen();
    initSoil();
    initWifi();
    initServer();
    dhtTicker.attach_scheduled(2, readDht);
    screenTicker.attach(4, refreshScreen);
    ticker.attach_scheduled(4, timerTask);
    timeClient.begin();

    startTime = read(EEPROM_RUNTIME);
    repeatTime = read(EEPROM_REPEAT);
    watering = read(EEPROM_WATERING);
    lastWateringDay = read(EEPROM_LAST_DAY);
    lastWateringHour = read(EEPROM_LAT_HOUR);
}

void loop() {
    loopServer();
}