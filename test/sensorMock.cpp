#include "sensorMock.h"

DallasTemperature::DallasTemperature(OneWire* _oneWire) {
	this->temperatures[0] = 10.10;
	this->temperatures[1] = 20.10;
	this->temperatures[2] = 30.10;
}
void DallasTemperature::begin(void) {}
void DallasTemperature::requestTemperatures() {}
uint8_t DallasTemperature::getDeviceCount(void) {
	return 3;
}
float DallasTemperature::getTempC(const uint8_t* deviceAddress) {
	if (deviceAddress[7]==0x41) return this->temperatures[0];//water
	if (deviceAddress[7]==0x8A) return this->temperatures[1];//oil
	if (deviceAddress[7]==0x4B) return this->temperatures[2];//
	return 0.0;
}