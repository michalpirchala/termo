#ifndef DallasTemperature_h
#define DallasTemperature_h

#include <OneWire.h>

typedef uint8_t DeviceAddress[8];

class DallasTemperature{
	public:
		DallasTemperature(OneWire *ow);
		void begin(void);
		uint8_t getDeviceCount(void);
		void requestTemperatures(void);
		float getTempC(const uint8_t*);

		float temperatures[3];
};

#endif