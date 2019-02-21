/*
ChaosHETPicDD.cpp
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
#include "ChaosHETPicDD.h"
#include <string>
#include <boost/regex.hpp>
#include <chaos/cu_toolkit/driver_manager/driver/AbstractDriverPlugin.h>
// including interface
#include "driver/misc/models/HETPic/core/ChaosHETPicInterface.h"
#define ACLAPP	LAPP_ << "[ChaosHETPicDD] "
#define ACDBG	LDBG_ << "[ChaosHETPicDD] "
#define ACERR	LERR_ << "[ChaosHETPicDD] "
using namespace chaos::driver::hetpic;
//default constructor definition
DEFAULT_CU_DRIVER_PLUGIN_CONSTRUCTOR_WITH_NS(chaos::driver::hetpic, ChaosHETPicDD) {
	devicedriver = NULL;
}
ChaosHETPicDD::~ChaosHETPicDD() {
}
void ChaosHETPicDD::driverDeinit() {
	 if(devicedriver) {
		delete devicedriver;
	}
	devicedriver = NULL;
}
cu_driver::MsgManagmentResultType::MsgManagmentResult ChaosHETPicDD::execOpcode(cu_driver::DrvMsgPtr cmd){
	 cu_driver::MsgManagmentResultType::MsgManagmentResult result = cu_driver::MsgManagmentResultType::MMR_EXECUTED;
	hetpic_iparams_t *in = (hetpic_iparams_t *)cmd->inputData;
	hetpic_oparams_t *out = (hetpic_oparams_t *)cmd->resultData;
	switch(cmd->opcode){
		case OP_SETHIGHTHRESHOLD: {
			out->result=devicedriver->SetHighThreshold(in->int32_t1,in->int32_t2);
			ACDBG << "Sent to driver command SetHighThreshold result is " << out->result;
			} break;
		case OP_SETLOWTHRESHOLD: {
			out->result=devicedriver->SetLowThreshold(in->int32_t1,in->int32_t2);
			ACDBG << "Sent to driver command SetLowThreshold result is " << out->result;
			} break;
		case OP_SETPULSE: {
			out->result=devicedriver->setPulse(in->int32_t1);
			ACDBG << "Sent to driver command setPulse result is " << out->result;
			} break;
		case OP_GETLOWTHRESHOLDS: {
			out->result=devicedriver->getLowThresholds(out->int32_tE1);
			ACDBG << "Sent to driver command getLowThresholds result is " << out->result;
			} break;
		case OP_GETHIGHTHRESHOLDS: {
			out->result=devicedriver->getHighThresholds(out->int32_tE1);
			ACDBG << "Sent to driver command getHighThresholds result is " << out->result;
			} break;
		case OP_GETTEMPERATURES: {
			out->result=devicedriver->getTemperatures(out->doubleE1,out->doubleE2);
			ACDBG << "Sent to driver command getTemperatures result is " << out->result;
			} break;
	}
	return result;
}
