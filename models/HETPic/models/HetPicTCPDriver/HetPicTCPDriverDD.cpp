/*
HetPicTCPDriverDD.cpp
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
#include "HetPicTCPDriverDD.h"
#include "driver/misc/models/HETPic/core/ChaosHETPicInterface.h"
#include <common/misc/HETPic/models/HetPicTCPDriver/HetPicTCPDriver.h>
#include <common/misc/driver/ConfigDriverMacro.h>
OPEN_CU_DRIVER_PLUGIN_CLASS_DEFINITION(HetPicTCPDriverDD,1.0.0, chaos::driver::hetpic::HetPicTCPDriverDD)
REGISTER_CU_DRIVER_PLUGIN_CLASS_INIT_ATTRIBUTE(chaos::driver::hetpic::HetPicTCPDriverDD, http_address/dnsname:port)
CLOSE_CU_DRIVER_PLUGIN_CLASS_DEFINITION
OPEN_REGISTER_PLUGIN
REGISTER_PLUGIN(chaos::driver::hetpic::HetPicTCPDriverDD)
CLOSE_REGISTER_PLUGIN
chaos::driver::hetpic::HetPicTCPDriverDD::HetPicTCPDriverDD() {
	devicedriver = NULL;
}
chaos::driver::hetpic::HetPicTCPDriverDD::~HetPicTCPDriverDD() {
}
#ifdef CHAOS
void chaos::driver::hetpic::HetPicTCPDriverDD::driverInit(const chaos::common::data::CDataWrapper& json) {
	DRLAPP<< "Initializing HetPicTCPDriverDD HL Driver with CDataWrapper "<<std::endl;
	if (devicedriver) {
		throw chaos::CException(1,"Already Initialized ","HetPicTCPDriverDD::driverInit");
	}
	devicedriver= new ::common::hetpic::models::HetPicTCPDriver(json);
	if (devicedriver==NULL)
	{
		throw chaos::CException(1,"Cannot allocate resources for HetPicTCPDriver","HetPicTCPDriverDD::driverInit");
	}
}
#endif
void chaos::driver::hetpic::HetPicTCPDriverDD::driverInit(const char* initParameter) {
	DRLAPP<< "Initializing HetPicTCPDriverDD HL Driver with string "<< initParameter <<std::endl;
	if (devicedriver) {
		throw chaos::CException(1,"Already Initialized ","HetPicTCPDriverDD::driverInit");
	}
	devicedriver= new ::common::hetpic::models::HetPicTCPDriver(initParameter);
	if (devicedriver==NULL)
	{
		throw chaos::CException(1,"Cannot allocate resources for HetPicTCPDriver","HetPicTCPDriverDD::driverInit");
	}
}
