/*
ChaosHETControllerDD.cpp
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
#include "ChaosHETControllerDD.h"
#include <string>
#include <regex>
#include <chaos/cu_toolkit/driver_manager/driver/AbstractDriverPlugin.h>
// including interface
#include "driver/misc/models/HETController/core/ChaosHETControllerInterface.h"
#define ACLAPP	LAPP_ << "[ChaosHETControllerDD] "
#define ACDBG	LDBG_ << "[ChaosHETControllerDD] "
#define ACERR	LERR_ << "[ChaosHETControllerDD] "
using namespace chaos::driver::hetcontroller;
//default constructor definition
DEFAULT_CU_DRIVER_PLUGIN_CONSTRUCTOR_WITH_NS(chaos::driver::hetcontroller, ChaosHETControllerDD) {
	devicedriver = NULL;
}
ChaosHETControllerDD::~ChaosHETControllerDD() {
}
void ChaosHETControllerDD::driverDeinit() {
	 if(devicedriver) {
		delete devicedriver;
	}
	devicedriver = NULL;
}
cu_driver::MsgManagmentResultType::MsgManagmentResult ChaosHETControllerDD::execOpcode(cu_driver::DrvMsgPtr cmd){
	 cu_driver::MsgManagmentResultType::MsgManagmentResult result = cu_driver::MsgManagmentResultType::MMR_EXECUTED;
	hetcontroller_iparams_t *in = (hetcontroller_iparams_t *)cmd->inputData;
	hetcontroller_oparams_t *out = (hetcontroller_oparams_t *)cmd->resultData;
	switch(cmd->opcode){
		case OP_SWITCHHVPOWER: {
			out->result=devicedriver->SwitchHVPower(in->int32_t1);
			ACDBG << "Sent to driver command SwitchHVPower result is " << out->result;
			} break;
		case OP_SETHVONCHANNEL: {
			out->result=devicedriver->SetHVOnChannel(in->int32_t1,in->int32_t2,in->int32_t3);
			ACDBG << "Sent to driver command SetHVOnChannel result is " << out->result;
			} break;
		case OP_SETHIGHTHRESHOLD: {
			out->result=devicedriver->SetHighThreshold(in->int32_t1,in->int32_t2,in->int32_t3);
			ACDBG << "Sent to driver command SetHighThreshold result is " << out->result;
			} break;
		case OP_SETLOWTHRESHOLD: {
			out->result=devicedriver->SetLowThreshold(in->int32_t1,in->int32_t2,in->int32_t3);
			ACDBG << "Sent to driver command SetLowThreshold result is " << out->result;
			} break;
		case OP_SWITCHPULSE: {
			out->result=devicedriver->SwitchPulse(in->int32_t1,in->int32_t2);
			ACDBG << "Sent to driver command SwitchPulse result is " << out->result;
			} break;
	}
	return result;
}
