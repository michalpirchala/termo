#include <EEPROM.h>
#include <OneWire.h>
#include <TM1638plus.h>
#include "sensorMock.h"
#include "Display.h"

#define TEMP_PIN 3
#define SERVO_OPEN_PIN 5
#define SERVO_CLOSE_PIN 4
#define WINTER_PIN 11
#define ERROR_LED_PIN 13

#define SERVO_TIME 2	//seconds
#define REFRESH_TIME 20	//seconds

#define FURNACES_COUNT 2
int pumpPins[FURNACES_COUNT] = {6, 10};
DeviceAddress addresses[FURNACES_COUNT+1] = {
	{0x28, 0xF7, 0x26, 0xF3, 0x05, 0x00, 0x00, 0x41},//water
	{0x28, 0xFF, 0x5F, 0x6F, 0x54, 0x14, 0x00, 0x8A},//oil
	{0x28, 0xA3, 0x00, 0x05, 0x06, 0x00, 0x00, 0x4B},//v
};

OneWire oneWire(TEMP_PIN);

DallasTemperature sensors(&oneWire);

TM1638plus TMmodul(7, 9, 8);

Termostat termostat(&sensors, SERVO_OPEN_PIN, SERVO_CLOSE_PIN, WINTER_PIN, ERROR_LED_PIN, SERVO_TIME, FURNACES_COUNT, pumpPins, addresses);

void setup() {
	Serial.begin(9600);
	delay(500);
	
	termostat.resetVariables();
	termostat.loadVariables();

	Serial.println("WAITING FOR WINTER TIME...");
	while (digitalRead(WINTER_PIN)!=1);
	delay(500);
	
	termostat.update();
	termostatState(&termostat);

	//test active furnace
	Serial.print("TEST01: ");Serial.println((termostat.getActiveFurnace()==1) ? "OK" : "NOOK");
	//test servo opening
	Serial.print("TEST02: ");Serial.println((termostat.isServoOpening() && termostat.isServoOpened()) ? "OK" : "NOOK");
	delay((SERVO_TIME+0.1)*1000);
	//servo opening done
	Serial.print("TEST03: ");Serial.println((!termostat.isServoOpening() && termostat.isServoOpened()) ? "OK" : "NOOK");
	Serial.print("TEST04: ");Serial.println((termostat.isPumpActive()) ? "OK" : "NOOK");
	//pump should be off after temp drops below 30 (default val) and so servo closing
	sensors.temperatures[2] = 29.0;
	termostat.update();
	Serial.print("TEST05: ");Serial.println((!termostat.isPumpActive()) ? "OK" : "NOOK");
	Serial.print("TEST06: ");Serial.println((termostat.isServoClosing()) ? "OK" : "NOOK");


	//water temp is now higher than furnace
	sensors.temperatures[0] = 40.0;
	sensors.temperatures[1] = 35.0;
	sensors.temperatures[2] = 5.0;
	Termostat *termostat2 = new Termostat(&sensors, SERVO_OPEN_PIN, SERVO_CLOSE_PIN, WINTER_PIN, ERROR_LED_PIN, SERVO_TIME, FURNACES_COUNT, pumpPins, addresses);
	termostat2->update();
	//termostatState(termostat2);
	Serial.print("TEST11: ");Serial.println((termostat2->getActiveFurnace()==0) ? "OK" : "NOOK");
	Serial.print("TEST12: ");Serial.println((termostat2->isServoClosing() && !termostat2->isServoOpened()) ? "OK" : "NOOK");
	delay((SERVO_TIME+0.1)*1000);
	Serial.print("TEST23: ");Serial.println((!termostat2->isServoClosing() && !termostat2->isServoOpened()) ? "OK" : "NOOK");

	sensors.temperatures[1] = 5.0;
	sensors.temperatures[2] = 36.0;
	termostat2->update();
	Serial.print("TEST24: ");Serial.println((termostat2->getActiveFurnace()==1) ? "OK" : "NOOK");
	Serial.print("TEST25: ");Serial.println((!termostat2->isServoOpening()) ? "OK" : "NOOK");
	Serial.print("TEST26: ");Serial.println((termostat2->isPumpActive()) ? "OK" : "NOOK");
	Serial.print("TEST27: ");Serial.println((!termostat2->isServoOpened()) ? "OK" : "NOOK");

	sensors.temperatures[2] = 46.0;
	termostat2->update();
	Serial.print("TEST28: ");Serial.println((termostat2->getActiveFurnace()==1) ? "OK" : "NOOK");
	Serial.print("TEST29: ");Serial.println((termostat2->isServoOpening()) ? "OK" : "NOOK");
	Serial.print("TEST210: ");Serial.println((termostat2->isPumpActive()) ? "OK" : "NOOK");
	Serial.print("TEST211: ");Serial.println((termostat2->isServoOpened()) ? "OK" : "NOOK");

	//test diff values
	//set servo close diff to -1
	termostat2->lowerValueByScreen(2);
	termostat2->lowerValueByScreen(2);
	termostat2->lowerValueByScreen(2);
	termostat2->lowerValueByScreen(2);
	//set servo open to +1
	termostat2->upperValueByScreen(1);
	termostat2->upperValueByScreen(1);
	termostat2->upperValueByScreen(1);
	termostat2->upperValueByScreen(1);

	//servo is opened, should be closed when furnace temp is < water - 1. water temp is 40
	sensors.temperatures[2] = 39.5;//test -0.5
	termostat2->update();
	termostatState(termostat2);
	Serial.print("TEST221: ");Serial.println((termostat2->isServoOpened()) ? "OK" : "NOOK");

	sensors.temperatures[2] = 38.9;//test -1.1
	termostat2->update();
	Serial.print("TEST222: ");Serial.println((!termostat2->isServoOpened()) ? "OK" : "NOOK");

	//should be open again when furnace temp  >= water +1
	sensors.temperatures[2] = 40.5;//test +0.5
	termostat2->update();
	termostatState(termostat2);
	Serial.print("TEST223: ");Serial.println((!termostat2->isServoOpened()) ? "OK" : "NOOK");

	sensors.temperatures[2] = 41.0;//test +1.0
	termostat2->update();
	Serial.print("TEST224: ");Serial.println((termostat2->isServoOpened()) ? "OK" : "NOOK");

	delete termostat2;
	termostat2 = NULL;

	Serial.println("WAITING FOR SUMMER TIME...");
	while (digitalRead(WINTER_PIN)!=0);
	delay(500);

	//water temp higher than furnace - servo should be closed after start
	sensors.temperatures[0] = 40.0;
	sensors.temperatures[1] = 5.0;
	sensors.temperatures[2] = 30.0;
	termostat2 = new Termostat(&sensors, SERVO_OPEN_PIN, SERVO_CLOSE_PIN, WINTER_PIN, ERROR_LED_PIN, SERVO_TIME, FURNACES_COUNT, pumpPins, addresses);
	termostat2->update();
	Serial.print("TEST21: ");Serial.println((termostat2->getActiveFurnace()==1) ? "OK" : "NOOK");
	Serial.print("TEST22: ");Serial.println((termostat2->isServoClosing() && !termostat2->isServoOpened()) ? "OK" : "NOOK");
	delay((SERVO_TIME+0.1)*1000);
	Serial.print("TEST23: ");Serial.println((!termostat2->isServoClosing() && !termostat2->isServoOpened()) ? "OK" : "NOOK");
	Serial.print("TEST24: ");Serial.println((!termostat2->isPumpActive()) ? "OK" : "NOOK");
	termostat2->update();
	Serial.print("TEST25: ");Serial.println((!termostat2->isPumpActive()) ? "OK" : "NOOK");
	//now furnace temp is higher - servo should be open, pump running after second update
	sensors.temperatures[2] = 50.0;
	termostat2->update();
	Serial.print("TEST26: ");Serial.println((termostat2->isServoOpening() && termostat2->isServoOpened()) ? "OK" : "NOOK");
	Serial.print("TEST27: ");Serial.println((!termostat2->isPumpActive()) ? "OK" : "NOOK");
	termostat2->update();
	Serial.print("TEST28: ");Serial.println((termostat2->isPumpActive()) ? "OK" : "NOOK");
	
	delete termostat2;
	termostat2 = NULL;

	//water temp is now lower than furnace 1
	sensors.temperatures[0] = 20.0;
	termostat2 = new Termostat(&sensors, SERVO_OPEN_PIN, SERVO_CLOSE_PIN, WINTER_PIN, ERROR_LED_PIN, SERVO_TIME, FURNACES_COUNT, pumpPins, addresses);
	termostat2->update();
	Serial.print("TEST31: ");Serial.println((termostat2->getActiveFurnace()==1) ? "OK" : "NOOK");
	Serial.print("TEST32: ");Serial.println((termostat2->isServoOpening() && termostat2->isServoOpened()) ? "OK" : "NOOK");
	delay((SERVO_TIME+0.1)*1000);
	Serial.print("TEST33: ");Serial.println((!termostat2->isServoOpening() && termostat2->isServoOpened()) ? "OK" : "NOOK");
	termostat2->update();
	//pump should be running after second update
	Serial.print("TEST34: ");Serial.println((termostat2->isPumpActive()) ? "OK" : "NOOK");
	//after furnace temperature lowers, servo should be closed and pump immediately turned off
	sensors.temperatures[2] = 19.0;
	termostat2->update();
	Serial.print("TEST35: ");Serial.println((termostat2->isServoClosing()) ? "OK" : "NOOK");
	Serial.print("TEST36: ");Serial.println((!termostat2->isServoOpened()) ? "OK" : "NOOK");
	Serial.print("TEST37: ");Serial.println((!termostat2->isPumpActive()) ? "OK" : "NOOK");

	Serial.println("TESTS DONE");
	for(;;);
}

void loop() {

}

void termostatState(Termostat *termostat){
	Serial.print("FURNACE TEMP:	");Serial.println(termostat->getFurnaceTemp());
	Serial.print("WATER TEMP:	");Serial.println(termostat->getWaterTemp());
	Serial.print("WINTER TIME:	");Serial.println(termostat->isWinterTime());

	Serial.print("SERVO CLOSING:	");Serial.println(termostat->isServoClosing());
	Serial.print("SERVO OPENING:	");Serial.println(termostat->isServoOpening());
	Serial.print("SERVO OPENED:	");Serial.println(termostat->isServoOpened());
	Serial.print("ACTIVE FURNACE:	");Serial.println(termostat->getActiveFurnace());
	Serial.print("ACTIVE PUMP:	");Serial.println(termostat->isPumpActive());

	Serial.println("==============================");
}