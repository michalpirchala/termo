#include <Arduino.h>
#include <TM1638plus.h>
#include "Termostat.h"

class Display{
	public:
		Display(TM1638plus *module, Termostat *termostat);
		void doAction(), setActive(bool activity), setBlocked(bool blocked);
		void showOnScreen(char *message);

	private:
		byte prevButtons, buttons;
		TM1638plus *module;

		int intensity, active, mode;
		Termostat *termostat;
		void showScreen(int screen);
		void doActionByButtons(), okMsg();
		void formatValueMsg(char *message, char *valueName, float value);
};
