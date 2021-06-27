#include <Arduino.h>

#define LED D0

void setup()
{
    Serial.begin(76800);
    pinMode(LED, OUTPUT);
}

void loop()
{
    Serial.println("hello world");
    digitalWrite(LED, LOW);
    delay(500);
    digitalWrite(LED, HIGH);
    delay(1000);
}