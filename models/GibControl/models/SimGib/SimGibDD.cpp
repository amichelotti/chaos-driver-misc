/*
SimGibDD.cpp
!CHAOS
Created by CUGenerator

Copyright 2013 INFN, National Institute of Nuclear Physics
Licensed under the Apache License, Version 2.0 (the "License")
you may not use this file except in compliance with the License.
      You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
#include "SimGibDD.h"
#include "driver/misc/models/GibControl/core/ChaosGibControlInterface.h"
#include <common/misc/GibControl/models/SimGib/SimGib.h>
#include <common/misc/driver/ConfigDriverMacro.h>
OPEN_CU_DRIVER_PLUGIN_CLASS_DEFINITION(SimGibDD,1.0.0, chaos::driver::gibcontrol::SimGibDD)
REGISTER_CU_DRIVER_PLUGIN_CLASS_INIT_ATTRIBUTE(chaos::driver::gibcontrol::SimGibDD, http_address/dnsname:port)
CLOSE_CU_DRIVER_PLUGIN_CLASS_DEFINITION
OPEN_REGISTER_PLUGIN
REGISTER_PLUGIN(chaos::driver::gibcontrol::SimGibDD)
CLOSE_REGISTER_PLUGIN
chaos::driver::gibcontrol::SimGibDD::SimGibDD() {
	devicedriver = NULL;
}
chaos::driver::gibcontrol::SimGibDD::~SimGibDD() {
}
#ifdef CHAOS
void chaos::driver::gibcontrol::SimGibDD::driverInit(const chaos::common::data::CDataWrapper& json) throw(chaos::CException) {
	DRLAPP<< "Initializing SimGibDD HL Driver with CDataWrapper "<<std::endl;
	devicedriver= new ::common::gibcontrol::models::SimGib(json);
	if (devicedriver==NULL)
	{
		throw chaos::CException(1,"Cannot allocate resources for SimGib","SimGibDD::driverInit");
	}
}
#endif
void chaos::driver::gibcontrol::SimGibDD::driverInit(const char* initParameter) throw(chaos::CException) {
	DRLAPP<< "Initializing SimGibDD HL Driver with string "<< initParameter <<std::endl;
	if (devicedriver) {
		throw chaos::CException(1,"Already Initialized ","SimGibDD::driverInit");
	}
	devicedriver= new ::common::gibcontrol::models::SimGib(initParameter);
	if (devicedriver==NULL)
	{
		throw chaos::CException(1,"Cannot allocate resources for SimGib","SimGibDD::driverInit");
	}
}

int chaos::driver::gibcontrol::SimGibDD::setPulse(int32_t channel,int32_t amplitude,int32_t width,int32_t state){
	return 0;
}
			 int chaos::driver::gibcontrol::SimGibDD::setChannelVoltage(int32_t channel,double Voltage){
				 return 0;
			 }
			 int chaos::driver::gibcontrol::SimGibDD::PowerOn(int32_t on_state){
				 return 0;
			 }
			 int chaos::driver::gibcontrol::SimGibDD::getState(int32_t* state,std::string& desc){
				 return 0;
			 }
			 int chaos::driver::gibcontrol::SimGibDD::getVoltages(std::vector<double>& voltages){
				 return 0;
			 }
			 int chaos::driver::gibcontrol::SimGibDD::getNumOfChannels(int32_t* numOfChannels){
				 return 0;
			 }
			 int chaos::driver::gibcontrol::SimGibDD::getPulsingState(std::vector<int32_t>& amplitudes,std::vector<int32_t>& widthChannels){
				 return 0;
			 }
			 int chaos::driver::gibcontrol::SimGibDD::getSupplyVoltages(double* HVSupply,double* P5V,double* N5V){
				 return 0;
			 }