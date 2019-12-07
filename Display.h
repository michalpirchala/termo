#include <Arduino.h>
#include <TM1638.h>
#include "Termostat.h"

class Display{
	public:
		Display(TM1638 *module, Termostat *termostat);
		void doAction(), setActive(bool activity), setBlocked(bool blocked);
		void showOnScreen(char *message, byte dots);

	private:
		byte prevButtons, buttons;
		TM1638 *module;

		int intensity, active, mode;
		Termostat *termostat;
		void showScreen(int screen);
		void doActionByButtons();
};