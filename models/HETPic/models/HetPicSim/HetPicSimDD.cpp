/*
HetPicSimDD.cpp
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
#include "HetPicSimDD.h"
#include "driver/misc/models/HETPic/core/ChaosHETPicInterface.h"
#include <common/misc/HETPic/models/HetPicSim/HetPicSim.h>
#include <common/misc/driver/ConfigDriverMacro.h>
OPEN_CU_DRIVER_PLUGIN_CLASS_DEFINITION(HetPicSimDD,1.0.0, chaos::driver::hetpic::HetPicSimDD)
REGISTER_CU_DRIVER_PLUGIN_CLASS_INIT_ATTRIBUTE(chaos::driver::hetpic::HetPicSimDD, http_address/dnsname:port)
CLOSE_CU_DRIVER_PLUGIN_CLASS_DEFINITION
OPEN_REGISTER_PLUGIN
REGISTER_PLUGIN(chaos::driver::hetpic::HetPicSimDD)
CLOSE_REGISTER_PLUGIN
chaos::driver::hetpic::HetPicSimDD::HetPicSimDD() {
	devicedriver = NULL;
}
chaos::driver::hetpic::HetPicSimDD::~HetPicSimDD() {
}
#ifdef CHAOS
void chaos::driver::hetpic::HetPicSimDD::driverInit(const chaos::common::data::CDataWrapper& json) {
	DRLAPP<< "Initializing HetPicSimDD HL Driver with CDataWrapper "<<std::endl;
	if (devicedriver) {
		throw chaos::CException(1,"Already Initialized ","HetPicSimDD::driverInit");
	}
	devicedriver= new ::common::hetpic::models::HetPicSim(json);
	if (devicedriver==NULL)
	{
		throw chaos::CException(1,"Cannot allocate resources for HetPicSim","HetPicSimDD::driverInit");
	}
}
#endif
void chaos::driver::hetpic::HetPicSimDD::driverInit(const char* initParameter) {
	DRLAPP<< "Initializing HetPicSimDD HL Driver with string "<< initParameter <<std::endl;
	if (devicedriver) {
		throw chaos::CException(1,"Already Initialized ","HetPicSimDD::driverInit");
	}
	devicedriver= new ::common::hetpic::models::HetPicSim(initParameter);
	if (devicedriver==NULL)
	{
		throw chaos::CException(1,"Cannot allocate resources for HetPicSim","HetPicSimDD::driverInit");
	}
}
