#include <EEPROM.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <TM1638.h>
#include "Display.h"

#define TEMP_PIN 3
#define PUMP_PIN 6
#define PUMP2_PIN 10
#define SERVO_OPEN_PIN 5
#define SERVO_CLOSE_PIN 4

#define SERVO_TIME 300
#define WINTER_PIN 11

#define REFRESH_TIME 20

OneWire oneWire(TEMP_PIN);
DallasTemperature sensors(&oneWire);
TM1638 TMmodul(8, 9, 7);

Termostat termostat(sensors, SERVO_OPEN_PIN, SERVO_CLOSE_PIN, SERVO_TIME, PUMP_PIN, PUMP2_PIN, WINTER_PIN);
Display display(&TMmodul, &termostat);

void(* resetFunc) (void) = 0;

unsigned long lastUpdateTime;

void setup() {
	Serial.begin(9600);
	Serial.print("Free RAM ");
	Serial.println(freeRam());

	delay(1000);

	pinMode(13, OUTPUT);
	pinMode(PUMP_PIN, OUTPUT);
	pinMode(PUMP2_PIN, OUTPUT);
	pinMode(SERVO_OPEN_PIN, OUTPUT);
	pinMode(SERVO_CLOSE_PIN, OUTPUT);

	pinMode(WINTER_PIN, INPUT_PULLUP);

	digitalWrite(13, 0);

	//init - everything runs
	digitalWrite(PUMP_PIN, PUMP_ON);
	digitalWrite(PUMP2_PIN, PUMP_ON);
	digitalWrite(SERVO_OPEN_PIN, SERVO_OPEN_ON);
	digitalWrite(SERVO_CLOSE_PIN, !SERVO_CLOSE_ON);

	updateTermo();
}

void loop() {
  //reset if time overflows
	if (millis()<lastUpdateTime) resetFunc();

	Serial.print("Free RAM ");Serial.println(freeRam());

	if (millis()>lastUpdateTime+REFRESH_TIME*1000){
		updateTermo();
	}
	
	display.doAction();

	if (termostat.getError()){
		Serial.print("error:");Serial.println(termostat.getError());
	}
}

void updateTermo(){
	display.setBlocked(1);
	termostat.update();
	lastUpdateTime = millis();
	display.setBlocked(0);
}

int freeRam()
{
	extern unsigned int __heap_start;
	extern void *__brkval;
	int free_memory;
	int stack_here;
	if (__brkval == 0)
		free_memory = (int) &stack_here - (int) &__heap_start;
	else
		free_memory = (int) &stack_here - (int) __brkval; 
	return (free_memory);
}
