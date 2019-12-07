#include "Termostat.h"
#include <EEPROM.h>

Termostat::Termostat(DallasTemperature sensors, int servoOpenPin, int servoClosePin, int servoTime, int pumpPin, int pump2Pin, int winterPin){
	//nastavenie objektu
	this->sensors = sensors;
	this->servoOpenPin = servoOpenPin;
	this->servoClosePin = servoClosePin;
	this->servoTime = servoTime;
	this->pumpPin = pumpPin;
	this->pump2Pin = pump2Pin;
	this->winterPin = winterPin;
	this->activePump = 0;
	this->servoOpenTime = 0;
	this->servoCloseTime = 0;
	this->servoState = 0;
	this->firstRun = 1;
	
	this->loadVariables();

	this->setError(0);
	
	this->sensors.begin();
	
	this->controllTempCount();
}

void Termostat::controllTempCount(){
	this->tempCount = this->sensors.getDeviceCount();
	
	if (this->tempCount<2 && this->tempCount>3){
		this->setError(1);
	} else {
		this->setError(0);
	}
}

bool Termostat::update(){
	int i;
	bool hasError = 0;
	
	//ak mame chybu 1, tak skusime ci sa opravila, chyba 2 sa opravi v tejto metode
	if (this->getError() && this->getError()!=2){
		if (this->getError()==1) this->controllTempCount();

		if (this->getError()) return false;
	}

	//ziskanie teploty
	this->sensors.requestTemperatures();
	
	for (i=0;i<this->tempCount;i++){
		this->temperatures[i] = this->sensors.getTempCByIndex(i);
		Serial.print("SENZOR ");Serial.print(i);Serial.print(": ");Serial.println(this->temperatures[i]);
		if (this->temperatures[i]==-127){
			hasError = 1;
		}
	}

	//zle poradie senzorov DOMA
	// 0 - viadrus
	// 1 - voda
	// 2 - olejovy	
	float temporary = this->temperatures[0];
	this->temperatures[0] = this->temperatures[1];
	this->temperatures[1] = this->temperatures[2];
	this->temperatures[2] = temporary;

	//ak mame chybu, tak vyhodime chybu 2
	if (hasError){
		this->setError(2);
		return false;
	} else {//ak nie a mali sme, tak ju zrusime
		if (this->getError()==2) this->setError(0);
	}

	//nepokracujeme dalej, termostat ostava v error state
	if (this->getError()){
		return false;
	}

	this->setActiveBoiler();

	if (this->firstRun){
		//servo sa musi vypnut alebo zapnut na zaciatku, takze mu nastavime akoze opacnu hodnotu ako by malo mat, cim sa zabezpeci ze sa vypne/zapne
		if (this->servoShouldStop()){
			this->servoState = 1;
		}
		if (this->servoShouldStart()){
			this->servoState = 0;
		}

		this->firstRun = 0;
	}

	this->pumpAction();//najprv cerpadlo
	this->servoAction();//ak ide cerpadlo tak aj servo
	
	
	return true;
}

/*
 * Vrati pocet senzorov
 */
int Termostat::getTempCount(){
	return this->tempCount;
}

float* Termostat::getTemperatures(){
	return this->temperatures;
}

/*
 * vrati teplotu vody, ktoru ohrievame
 */
float Termostat::getWaterTemp(){
	return this->temperatures[0];
}
/*
 * Vrati teplotu peca (teplejsieho)
 */
float Termostat::getBoilerTemp(){
	return this->temperatures[this->activeBoiler];
}
/*
 * nastavi aktivny pec podla toho ktory je teplejsi
 */
void Termostat::setActiveBoiler(){
	if (this->tempCount==2){
		this->activeBoiler = 1;	
	} else {
		if (this->temperatures[1]>this->temperatures[2]){
			this->activeBoiler = 1;
		} else {
			this->activeBoiler = 2;
		}
	}	
}
int Termostat::getActiveBoiler(){
	return this->activeBoiler;
}
/*
 * urobi akciu serva - ak je potrebna
 */
void Termostat::servoAction(){
	if (this->servoShouldStartBySeason()){
		if (!this->servoState){
			this->servoState = 1;
			this->servoOpenTime = millis()+(unsigned long)this->servoTime*(unsigned long)1000;
			this->servoCloseTime = 0;
		}
	} else {//istota
		if (this->servoShouldStopBySeason()){
			if (this->servoState){
				this->servoState = 0;
				this->servoCloseTime = millis()+(unsigned long)this->servoTime*(unsigned long)1000;
				this->servoOpenTime = 0;
				if (!this->isWinterTime()){//cerpadlo vypneme hned, aby sa necakalo na dalsi update
					this->pumpAction();
				}
			}
		} else {//hystereza, nerobime nic
			//this->servoOpenTime = 0;
			//this->servoCloseTime = 0;
		}
	}

	if (this->isServoOpening()){
		digitalWrite(this->servoOpenPin, SERVO_OPEN_ON);
		digitalWrite(this->servoClosePin, !SERVO_CLOSE_ON);
	} else {//istota
		if (this->isServoClosing()){
			digitalWrite(this->servoOpenPin, !SERVO_OPEN_ON);
			digitalWrite(this->servoClosePin, SERVO_CLOSE_ON);
		} else {
			digitalWrite(this->servoOpenPin, !SERVO_OPEN_ON);
			digitalWrite(this->servoClosePin, !SERVO_CLOSE_ON);
		}
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
		return this->servoShouldStart();//nepozerame ci je cerpadlo zapnute, lebo ono sa zapne podla serva v lete
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
	return (this->getBoilerTemp()>=this->getWaterTemp()+this->diffServoOpen);
}
bool Termostat::servoShouldStop(){
	return (this->getBoilerTemp()<this->getWaterTemp()+this->diffServoClose);
}

/*
 * zapne/vypne cerpadlo - ak je potrebne
 */
void Termostat::pumpAction(){
	if (this->isWinterTime()){ //v zime podla nastavenej teploty
		if (this->getBoilerTemp()>=this->tempPump){
			if (this->activeBoiler==1){
				this->startPump1();
			} else if (this->activeBoiler==2){
				this->startPump2();
			}
		}
		if (this->getBoilerTemp()<=this->tempPump-PUMP_OFF_HYST){
			this->stopPumps();
		}
	} else {//v lete
		if (this->servoState){ //ak je otvorene servo zapneme cerpadlo
			if (this->activeBoiler==1){
				this->startPump1();
			} else if (this->activeBoiler==2){
				this->startPump2();
			}
		} else {
			this->stopPumps();
		}
	}
}
void Termostat::startPump1(){
	this->activePump = 1;
	digitalWrite(this->pumpPin, PUMP_ON);
	digitalWrite(this->pump2Pin, !PUMP_ON);
}
void Termostat::startPump2(){
	this->activePump = 1;
	digitalWrite(this->pumpPin, !PUMP_ON);
	digitalWrite(this->pump2Pin, PUMP_ON);
}
void Termostat::stopPumps(){
	this->activePump = 0;
	digitalWrite(this->pumpPin, !PUMP_ON);
	digitalWrite(this->pump2Pin, !PUMP_ON);
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
	digitalWrite(13, 1);

	digitalWrite(this->pumpPin, PUMP_ON);
	digitalWrite(this->pump2Pin, PUMP_ON);

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
 * funkcie pre pracu s hodnotami
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
