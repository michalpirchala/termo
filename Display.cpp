#include "Display.h"

Display::Display(TM1638plus *module, Termostat *termostat){
    this->module = module;
    this->termostat = termostat;
    this->intensity = 0;
    this->prevButtons = 0;
    this->mode = 0;
    this->active = 1;

    this->module->displayBegin();
    this->module->displayText("TERMOMP3");
    this->module->brightness(this->intensity);
}

void Display::doAction(){
    this->buttons = this->module->readButtons();

    if (this->buttons!=this->prevButtons && this->prevButtons==0b00000000){
        delay(50);
        this->buttons = this->module->readButtons();
        this->doActionByButtons();
    }

    this->prevButtons = this->buttons;

    this->showScreen(this->mode);
}

void Display::doActionByButtons(){
    switch(this->buttons){
        case 0b00000001://turn display on/off
            this->setActive(!this->active);
            break;
        case 0b00000010://next screen
            this->mode++;
            if (this->mode==4) this->mode = 0;
            break;
        case 0b00000011://prev screen
            this->mode--;
            if (this->mode==-1) this->mode = 3;
            break;
        case 0b00000100://actual value -
            this->termostat->lowerValueByScreen(this->mode);
            break;
        case 0b00001000://actual value +
            this->termostat->upperValueByScreen(this->mode);
            break;
        case 0b00010000://save values to memory
            this->termostat->saveVariables();
            this->okMsg();
            break;
        case 0b00100000://load values from memory
            this->termostat->loadVariables();
            this->okMsg();
            break;
        case 0b00110000://reset values in memory
            this->termostat->resetVariables();
            this->okMsg();
            break;
        case 0b01000000://

            break;
        case 0b10000000://show last error
            char message[10];
            sprintf(message, "L.ERR %3d", this->termostat->getLastError());
            this->showOnScreen(message);
            delay(1000);
            break;
        case 0b11000000://test error state
            this->termostat->setError(9);
            break;
    }
}

void Display::showScreen(int screen){
    char message[11];

    if (this->termostat->getError()!=0){
        sprintf(message, "ERR %4d", this->termostat->getError());
        this->showOnScreen(message);
        return;
    }

    if (screen==0){
        char str_ft[6], str_wt[6];
        dtostrf(this->termostat->getActiveFurnaceTemp(), 5, 2, str_ft);
        dtostrf(this->termostat->getWaterTemp(), 5, 2, str_wt);
        sprintf(message, "%s%s", str_ft, str_wt);
    } else if (screen==1){
        this->formatValueMsg(message, "RO", this->termostat->getValueByScreen(screen));
    } else if (screen==2){
        this->formatValueMsg(message, "RZ", this->termostat->getValueByScreen(screen));
    } else if (screen==3){
        this->formatValueMsg(message, "TC", this->termostat->getValueByScreen(screen));
    } else {
        sprintf(message, "%8d", screen);
    }

    //LEDs
    this->module->setLED(7, ((screen >> 1) & 1));
    this->module->setLED(6, ((screen >> 0) & 1));
    this->module->setLED(5, this->termostat->isServoClosing());
    this->module->setLED(4, this->termostat->isServoOpening());
    this->module->setLED(3, this->termostat->isServoOpened());
    this->module->setLED(2, this->termostat->getActiveFurnace()==1);
    this->module->setLED(1, this->termostat->getActiveFurnace()==0);
    this->showOnScreen(message);
}
void Display::showOnScreen(char *message){
    this->module->displayText(message);
}
void Display::setBlocked(bool blocked){
    this->module->setLED(0, blocked);
}
void Display::setActive(bool activity){
    this->active = activity;
    this->module->brightness(activity ? 0x02 : 0x00);
}
void Display::okMsg(){
    this->showOnScreen("OK      ");
    delay(OK_DELAY);
}
void Display::formatValueMsg(char *message, char *valueName, float value){
    char str_temp[6];
    dtostrf(value, 5, 2, str_temp);
    sprintf(message, "%s %6s", valueName, str_temp);
}
