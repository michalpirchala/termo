#include <OneWire.h>
#include <DallasTemperature.h>
/*
Kody chyb:
1 - menej ako 2 senzory teploty
2 - vypadol senzor

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
		Termostat(DallasTemperature senors, int servoOpenPin, int servoClosePin, int servoTime, int pumpPin, int pump2Pin, int WinterPin);
		bool update();
		
		float* getTemperatures();
		int getTempCount();
		
		int getError();
		void setError(int error);
		
		float getWaterTemp();
		float getBoilerTemp();
		
		int getActiveBoiler();

		void saveVariables(), loadVariables(), resetVariables();
		
		float getValueByScreen(int screen);
		void lowerValueByScreen(int screen), upperValueByScreen(int screen);

		bool isPumpActive(), isServoClosing(), isServoOpening(), isServoOpened();
		
	private:
		float temperatures[3],//teploty zo senzorov
			diffServoOpen,
			diffServoClose,
			tempPump
		;
		
		int tempPin, servoOpenPin, servoClosePin, pumpPin, pump2Pin, winterPin,
			servoTime,	//ako dlho sa ma otvarat/zatvarat servo
			tempCount,	//pocet senzorov pre kontrolu pocas behu
			error,		//kod chyby
			activeBoiler
		;

		bool activePump, servoState, firstRun;

		unsigned long servoCloseTime, servoOpenTime;

		DallasTemperature sensors;
		
		void setActiveBoiler();
		void errorState();
		void servoAction();
		void pumpAction();
		void controllTempCount();
		bool servoShouldStart(), servoShouldStop(), isWinterTime(), servoShouldStopBySeason(), servoShouldStartBySeason();
		void startPump1(), startPump2(), stopPumps();

};
