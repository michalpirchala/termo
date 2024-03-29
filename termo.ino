#include <EEPROM.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <TM1638plus.h>
#include "Display.h"
#include "Debug.h"

#define TEMP_PIN 3
#define SERVO_OPEN_PIN 5
#define SERVO_CLOSE_PIN 4
#define WINTER_PIN 11
#define ERROR_LED_PIN 13

#define SERVO_TIME 300    //seconds
#define REFRESH_TIME 20   //seconds

#define FURNACES_COUNT 2

int pumpPins[FURNACES_COUNT] = {6, 10};
DeviceAddress addresses[FURNACES_COUNT+1] = {
    {0x28, 0x6D, 0xF3, 0xB4, 0x5E, 0x14, 0x01, 0xE6},//water
    {0x28, 0xFF, 0x78, 0x70, 0x54, 0x14, 0x00, 0x81},//oil
    {0x28, 0xA3, 0x00, 0x05, 0x06, 0x00, 0x00, 0x4B},//v
};

OneWire oneWire(TEMP_PIN);
DallasTemperature sensors(&oneWire);
TM1638plus TMmodul(7, 9, 8);

Termostat termostat(&sensors, SERVO_OPEN_PIN, SERVO_CLOSE_PIN, WINTER_PIN, ERROR_LED_PIN, SERVO_TIME, FURNACES_COUNT, pumpPins, addresses);
Display display(&TMmodul, &termostat);

unsigned long lastUpdateTime = 0;

void setup() {
    Serial.begin(9600);
    PRINTSLN("INFO: Startup");
    delay(1000);

    updateTermo();
}

void loop() {
    if (millis() - lastUpdateTime >= REFRESH_TIME*1000){
        updateTermo();
    }

    display.doAction();
}

void updateTermo(){
    display.setBlocked(1);
    termostat.update();
    lastUpdateTime = millis();
    display.setBlocked(0);
}
