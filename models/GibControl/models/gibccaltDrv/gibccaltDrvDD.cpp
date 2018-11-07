/*
gibccaltDrvDD.cpp
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
#include "gibccaltDrvDD.h"
#include "driver/misc/models/GibControl/core/ChaosGibControlInterface.h"
#include <common/misc/GibControl/models/gibccaltDrv/gibccaltDrv.h>
#include <common/misc/driver/ConfigDriverMacro.h>
OPEN_CU_DRIVER_PLUGIN_CLASS_DEFINITION(gibccaltDrvDD,1.0.0, chaos::driver::gibcontrol::gibccaltDrvDD)
REGISTER_CU_DRIVER_PLUGIN_CLASS_INIT_ATTRIBUTE(chaos::driver::gibcontrol::gibccaltDrvDD, http_address/dnsname:port)
CLOSE_CU_DRIVER_PLUGIN_CLASS_DEFINITION
OPEN_REGISTER_PLUGIN
REGISTER_PLUGIN(chaos::driver::gibcontrol::gibccaltDrvDD)
CLOSE_REGISTER_PLUGIN
chaos::driver::gibcontrol::gibccaltDrvDD::gibccaltDrvDD() {
	devicedriver = NULL;
}
chaos::driver::gibcontrol::gibccaltDrvDD::~gibccaltDrvDD() {
}
#ifdef CHAOS
void chaos::driver::gibcontrol::gibccaltDrvDD::driverInit(const chaos::common::data::CDataWrapper& json) {
	DRLAPP<< "Initializing gibccaltDrvDD HL Driver with CDataWrapper "<<std::endl;
	if (devicedriver) {
		throw chaos::CException(1,"Already Initialized ","gibccaltDrvDD::driverInit");
	}
	devicedriver= new ::common::gibcontrol::models::gibccaltDrv(json);
	if (devicedriver==NULL)
	{
		throw chaos::CException(1,"Cannot allocate resources for gibccaltDrv","gibccaltDrvDD::driverInit");
	}
}
#endif
void chaos::driver::gibcontrol::gibccaltDrvDD::driverInit(const char* initParameter) {
	DRLAPP<< "Initializing gibccaltDrvDD HL Driver with string "<< initParameter <<std::endl;
	if (devicedriver) {
		throw chaos::CException(1,"Already Initialized ","gibccaltDrvDD::driverInit");
	}
	devicedriver= new ::common::gibcontrol::models::gibccaltDrv(initParameter);
	if (devicedriver==NULL)
	{
		throw chaos::CException(1,"Cannot allocate resources for gibccaltDrv","gibccaltDrvDD::driverInit");
	}
}
