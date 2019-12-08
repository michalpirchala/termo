#include "Termostat.h"
#include <EEPROM.h>

Termostat::Termostat(DallasTemperature senors, int servoOpenPin, int servoClosePin, int WinterPin, int errorLedPin, int servoTime, int furnacesCount, int *pumpPins, DeviceAddress *addresses){
	this->sensors = sensors;
	this->servoOpenPin = servoOpenPin;
	this->servoClosePin = servoClosePin;
	this->winterPin = winterPin;
	this->errorLedPin = errorLedPin;
	this->servoTime = servoTime;
	this->furnacesCount = furnacesCount;
	this->pumpPins = pumpPins;
	this->addresses = addresses;
	this->activePump = 0;
	this->servoOpenTime = 0;
	this->servoCloseTime = 0;
	this->servoState = 0;
	this->firstRun = 1;

	pinMode(this->errorLedPin, OUTPUT);
	pinMode(this->servoOpenPin, OUTPUT);
	pinMode(this->servoClosePin, OUTPUT);
	pinMode(this->winterPin, INPUT_PULLUP);

	//init - everything runs
	digitalWrite(this->errorLedPin, 0);
	digitalWrite(this->servoOpenPin, SERVO_OPEN_ON);
	digitalWrite(this->servoClosePin, !SERVO_CLOSE_ON);

	for (int i = 0; i < furnacesCount; i++){
		pinMode(pumpPins[i], OUTPUT);
		digitalWrite(pumpPins[i], PUMP_ON);
	}

	this->loadVariables();
	this->setError(0);
	this->sensors.begin();
	this->checkTempCount();
}

void Termostat::checkTempCount(){
	this->tempCount = this->sensors.getDeviceCount();
	
	if (this->tempCount!=this->furnacesCount+1){
		this->setError(1);
	} else {
		this->setError(0);
	}
}

bool Termostat::update(){
	int i;
	int hasError = 0;
	
	this->sensors.requestTemperatures();
	
	for (i=0;i<this->tempCount;i++){
		this->temperatures[i] = this->sensors.getTempC(this->addresses[i]);

		Serial.print("SENSOR ");Serial.print(i);Serial.print(": ");Serial.println(this->temperatures[i]);

		if (this->temperatures[i]==-127 || this->temperatures[i]==-85){
			hasError = 10+i;
		}
	}

	//if error occured at getting temperature, set error to 2
	if (hasError){
		this->setError(hasError);
		return false;
	} else if (this->getError()) {//no error occured but we are still in error state
		 this->setError(0);
	}

	//if we have an error, do not continue
	if (this->getError()){
		return false;
	}

	this->setActiveFurnace();

	if (this->firstRun){
		//set opposite servo state to make sure action happens at the beginning
		if (this->servoShouldStop()){
			this->servoState = 1;
		}
		if (this->servoShouldStart()){
			this->servoState = 0;
		}

		this->firstRun = 0;
	}

	this->pumpAction();
	this->servoAction();
	
	return true;
}

int Termostat::getTempCount(){
	return this->tempCount;
}

float* Termostat::getTemperatures(){
	return this->temperatures;
}

/*
 * returns temp of water we are heating
 */
float Termostat::getWaterTemp(){
	return this->temperatures[0];
}
/*
 * returns active furnace temp
 */
float Termostat::getFurnaceTemp(){
	return this->temperatures[this->activeFurnace+1];
}

/*
 * set active furnace to the warmest one
 */
void Termostat::setActiveFurnace(){
	this->activeFurnace = 0;	

	if (this->furnacesCount>1){
		float highestTemp = this->temperatures[1];

		for (int i = 2; i<=this->tempCount; i++){
			if (this->temperatures[i]>highestTemp) this->activeFurnace = i-1;
		}
	}	
}
int Termostat::getActiveFurnace(){
	return this->activeFurnace;
}

/*
 * servo action if needed
 */
void Termostat::servoAction(){
	if (this->servoShouldStartBySeason()){
		if (!this->servoState){
			this->servoState = 1;
			this->servoOpenTime = millis()+(unsigned long)this->servoTime*(unsigned long)1000;
			this->servoCloseTime = 0;
		}
	} else if (this->servoShouldStopBySeason()){
		if (this->servoState){
			this->servoState = 0;
			this->servoCloseTime = millis()+(unsigned long)this->servoTime*(unsigned long)1000;
			this->servoOpenTime = 0;

			//in summer time run pump action immediately to turn pump off
			if (!this->isWinterTime()){
				this->pumpAction();
			}
		}
	}

	if (this->isServoOpening()){
		digitalWrite(this->servoOpenPin, SERVO_OPEN_ON);
		digitalWrite(this->servoClosePin, !SERVO_CLOSE_ON);
	} if (this->isServoClosing()){
		digitalWrite(this->servoOpenPin, !SERVO_OPEN_ON);
		digitalWrite(this->servoClosePin, SERVO_CLOSE_ON);
	} else {
		digitalWrite(this->servoOpenPin, !SERVO_OPEN_ON);
		digitalWrite(this->servoClosePin, !SERVO_CLOSE_ON);
	}
}
bool Termostat::isServoOpened(){
	return this->servoState;
}
bool Termostat::isServoOpening(){
	return this->servoOpenTime > millis();
}
bool Termostat::isServoClosing(){
	return this->servoCloseTime > millis();
}
bool Termostat::servoShouldStartBySeason(){
	if (this->isWinterTime()){
		return this->servoShouldStart() && this->isPumpActive();
	} else {
		return this->servoShouldStart();//pump will be activated by servo state in summer
	}
}
bool Termostat::servoShouldStopBySeason(){
	if (this->isWinterTime()){
		return this->servoShouldStop() || !this->isPumpActive();
	} else {
		return this->servoShouldStop();
	}
}
bool Termostat::servoShouldStart(){
	return this->getFurnaceTemp()>=this->getWaterTemp()+this->diffServoOpen;
}
bool Termostat::servoShouldStop(){
	return this->getFurnaceTemp()<this->getWaterTemp()+this->diffServoClose;
}

/*
 * turn pump on/off
 */
void Termostat::pumpAction(){
	if (this->isWinterTime()){//in winter according to set tempPump
		if (this->getFurnaceTemp()>=this->tempPump){
			this->startPump();
		}
		if (this->getFurnaceTemp()<=this->tempPump-PUMP_OFF_HYST){
			this->stopPumps();
		}
	} else {//summer
		if (this->servoState){ //only if servo is open
			this->startPump();
		} else {
			this->stopPumps();
		}
	}
}
void Termostat::startPump(){
	this->activePump = 0;
	for (int i = 0; i < this->furnacesCount; i++){
		if (this->activeFurnace==i){
			digitalWrite(this->pumpPins[i], PUMP_ON);
			this->activePump = 1;
		} else {
			digitalWrite(this->pumpPins[i], !PUMP_ON);
		}
	}
}
void Termostat::stopPumps(){
	for (int i = 0; i < this->furnacesCount; i++){
		digitalWrite(this->pumpPins[i], !PUMP_ON);
	}
	this->activePump = 0;
}
bool Termostat::isPumpActive(){
	return this->activePump;
}

/*
 * funkcie pre chyby
 */
int Termostat::getError(){
	return this->error;
}
void Termostat::setError(int error){
	this->error = error;
	if (error) this->errorState();
}

/*
 * chybovy stav - zapni cerpadla, otvor servo
 */
void Termostat::errorState(){
	digitalWrite(this->errorLedPin, 1);

	for (int i = 0; i < this->furnacesCount; i++){
		digitalWrite(this->pumpPins[i], PUMP_ON);
	}

	this->servoState = 1;
	digitalWrite(this->servoOpenPin, SERVO_OPEN_ON);
	digitalWrite(this->servoClosePin, !SERVO_CLOSE_ON);
}

/*
 * funkcie pre pamat
 */
void Termostat::loadVariables(){
	EEPROM.get(0, this->diffServoOpen);
	EEPROM.get(10, this->diffServoClose);
	EEPROM.get(20, this->tempPump);
}
void Termostat::saveVariables(){
	EEPROM.put(0, this->diffServoOpen);
	EEPROM.put(10, this->diffServoClose);
	EEPROM.put(20, this->tempPump);
}
void Termostat::resetVariables(){
	EEPROM.put(0, 0.25);
	EEPROM.put(10, 0.0);
	EEPROM.put(20, 30.0);
}

/*
 * functions for editing values
 */
float Termostat::getValueByScreen(int screen){
	switch (screen){
		case 1:
			return this->diffServoOpen;
		case 2:
			return this->diffServoClose;
		case 3:
			return this->tempPump;
	}
}
void Termostat::lowerValueByScreen(int screen){
	switch (screen){
		case 1:
			if (this->diffServoOpen>this->diffServoClose)
				this->diffServoOpen -= VALUE_CHANGE_STEP;
			break;
		case 2:
			this->diffServoClose -= VALUE_CHANGE_STEP;
			break;
		case 3:
			this->tempPump -= VALUE_CHANGE_STEP;
			break;
	}
}
void Termostat::upperValueByScreen(int screen){
	switch (screen){
		case 1:
			this->diffServoOpen += VALUE_CHANGE_STEP;
			break;
		case 2:
			if (this->diffServoOpen>this->diffServoClose)
				this->diffServoClose += VALUE_CHANGE_STEP;
			break;
		case 3:
			this->tempPump += VALUE_CHANGE_STEP;
			break;
	}
}

bool Termostat::isWinterTime(){
	return (digitalRead(this->winterPin)==WINTER_ON);
}
