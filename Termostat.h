#include <OneWire.h>
#include <DallasTemperature.h>
/*
Error codes:
1 - not according temp sensors count at start
9 - test error state
1X - sensor X not working properly
*/

#define OK_DELAY 1000
#define VALUE_CHANGE_STEP 0.25
#define PUMP_ON 1
#define SERVO_OPEN_ON 1
#define SERVO_CLOSE_ON 0

#define WINTER_ON 1
#define PUMP_OFF_HYST 0.2

class Termostat{
	public:
		Termostat(DallasTemperature senors, int servoOpenPin, int servoClosePin, int WinterPin, int errorLedPin, int servoTime, int furnacesCount, int *pumpPins, DeviceAddress *addresses);
		bool update();
		
		float* getTemperatures();
		int getTempCount();
		
		int getError();
		void setError(int error);
		
		float getWaterTemp();
		float getFurnaceTemp();
		
		int getActiveFurnace();

		void saveVariables(), loadVariables(), resetVariables();
		
		float getValueByScreen(int screen);
		void lowerValueByScreen(int screen), upperValueByScreen(int screen);

		bool isPumpActive(), isServoClosing(), isServoOpening(), isServoOpened();
		
	private:
		float temperatures[3],
			diffServoOpen,
			diffServoClose,
			tempPump
		;
		
		int tempPin, servoOpenPin, servoClosePin, winterPin, errorLedPin,
			servoTime,
			tempCount,	//sensors count for runtime check
			error,		//error code
			activeFurnace,
			furnacesCount,
			*pumpPins
		;

		bool activePump, servoState, firstRun;

		unsigned long servoCloseTime, servoOpenTime;

		DallasTemperature sensors;
		DeviceAddress *addresses;
		
		void setActiveFurnace();
		void errorState();
		void servoAction();
		void pumpAction();
		void checkTempCount();
		bool servoShouldStart(), servoShouldStop(), isWinterTime(), servoShouldStopBySeason(), servoShouldStartBySeason();
		void startPump(), stopPumps();
};
