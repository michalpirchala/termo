#include "Display.h"

Display::Display(TM1638 *module, Termostat *termostat){
	this->module = module;
	this->termostat = termostat;
	this->intensity = 0;
	this->prevButtons = 0;
	this->mode = 0;
	this->active = 1;

	this->module->setupDisplay(true, this->intensity);
	this->module->clearDisplay();
	this->module->setDisplayToString("TERMOMP3");
}

void Display::doAction(){
	this->buttons = this->module->getButtons();

	if (this->buttons!=this->prevButtons && (this->prevButtons==0b00000000 || this->buttons & this->prevButtons==0b00000000)){
		delay(50);
		this->buttons = this->module->getButtons();
		this->doActionByButtons();
	}

	this->prevButtons = this->buttons;

	this->showScreen(this->mode);
}

void Display::doActionByButtons(){
	switch(this->buttons){
		case 0b00000001://zap vypnutie displeja
			this->setActive(!this->active);
			break;
		case 0b00000010://dalsia obrazovka
			this->mode++;
			if (this->mode==4) this->mode = 0;
			break;
		case 0b00000011://predosla obrazovka
			this->mode--;
			if (this->mode==-1) this->mode = 3;
			break;
		case 0b00000100://hodnota -
			this->termostat->lowerValueByScreen(this->mode);
			break;
		case 0b00001000://hodnota +
			this->termostat->upperValueByScreen(this->mode);
			break;
		case 0b00010000://ulozit
			this->termostat->saveVariables();
			this->showOnScreen("JO SEFE ", 0b00000000);
			delay(OK_DELAY);
			break;
		case 0b00100000://nacitat
			this->termostat->loadVariables();
			this->showOnScreen("ALLRIGHT", 0b00000000);
			delay(OK_DELAY);
			break;
		case 0b00110000://reset v pamati
			this->termostat->resetVariables();
			this->showOnScreen("NOPROBLE", 0b00000000);
			delay(OK_DELAY);
			break;
		case 0b01000000://

			break;
		case 0b10000000://

			break;
		case 0b11000000://test chyboveho stavu
			this->termostat->setError(1);
			break;
	}
}

void Display::showScreen(int screen){
	char message[8];
	byte dots = 0b00000000;
	if (this->termostat->getError()!=0){
		sprintf(message, "ERR %4d", this->termostat->getError());
		this->showOnScreen(message, dots);
	} else {
		if (screen==0){
			int tempBoiler = (int)(this->termostat->getBoilerTemp()*100);
			int tempWater = (int)(this->termostat->getWaterTemp()*100);
			
			sprintf(message, "%4d%4d", tempBoiler, tempWater);
			dots = 0b01000100;
		} else if (screen==1){
			int value = (int)(this->termostat->getValueByScreen(screen)*100);
			//kvoli problemu so zobrazenim 3 rozne formaty
			if (value<100 && value>=0) sprintf(message, "RO   %03d", value);
			else if (value<0 && value>-100) sprintf(message, "RO  %04d", value);
			else sprintf(message, "RO%6d", value);
			dots = 0b00000100;
		} else if (screen==2){
			int value = (int)(this->termostat->getValueByScreen(screen)*100);
			if (value<100 && value>=0) sprintf(message, "RZ   %03d", value);
			else if (value<0 && value>-100) sprintf(message, "RZ  %04d", value);
			else sprintf(message, "RZ%6d", value);
			dots = 0b00000100;
		} else if (screen==3){
			int value = (int)(this->termostat->getValueByScreen(screen)*100);
			if (value<100 && value>=0) sprintf(message, "TC   %03d", value);
			else if (value<0 && value>-100) sprintf(message, "TC  %04d", value);
			else sprintf(message, "TC%6d", value);
			dots = 0b00000100;
		} else {
			sprintf(message, "%8d", screen);
		}

		//LEDky
		this->module->setLED( ((screen >> 1) & 1) ? TM1638_COLOR_GREEN + TM1638_COLOR_RED : TM1638_COLOR_NONE, 7);
		this->module->setLED( ((screen >> 0) & 1) ? TM1638_COLOR_GREEN + TM1638_COLOR_RED : TM1638_COLOR_NONE, 6);
		this->module->setLED((this->termostat->isServoClosing()) ? TM1638_COLOR_GREEN + TM1638_COLOR_RED : TM1638_COLOR_NONE, 5);
		this->module->setLED((this->termostat->isServoOpening()) ? TM1638_COLOR_GREEN + TM1638_COLOR_RED : TM1638_COLOR_NONE, 4);
		this->module->setLED((this->termostat->isServoOpened()) ? TM1638_COLOR_GREEN + TM1638_COLOR_RED : TM1638_COLOR_NONE, 3);
		this->module->setLED((this->termostat->getActiveBoiler()==2) ? TM1638_COLOR_GREEN + TM1638_COLOR_RED : TM1638_COLOR_NONE, 2);
		this->module->setLED((this->termostat->getActiveBoiler()==1) ? TM1638_COLOR_GREEN + TM1638_COLOR_RED : TM1638_COLOR_NONE, 1);
		this->showOnScreen(message, dots);
	}
}
void Display::showOnScreen(char *message, byte dots){
	this->module->setDisplayToString(message, dots);
}
void Display::setBlocked(bool blocked){
	this->module->setLED((blocked) ? TM1638_COLOR_GREEN + TM1638_COLOR_RED : TM1638_COLOR_NONE, 0);
}
void Display::setActive(bool activity){
	//this->mode = 3;
	this->active = activity;
	this->module->setupDisplay(activity, this->intensity);
}