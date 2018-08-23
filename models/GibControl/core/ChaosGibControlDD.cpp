/*
ChaosGibControlDD.cpp
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
#include "ChaosGibControlDD.h"
#include <string>
#include <boost/regex.hpp>
#include <chaos/cu_toolkit/driver_manager/driver/AbstractDriverPlugin.h>
// including interface
#include "driver/misc/models/GibControl/core/ChaosGibControlInterface.h"
#define ACLAPP	LAPP_ << "[GibControlDD] "
#define ACDBG		LDBG_ << "[GibControlDD] "
#define ACERR		LERR_ << "[GibControlDD] "
using namespace chaos::driver::gibcontrol;
//default constructor definition
DEFAULT_CU_DRIVER_PLUGIN_CONSTRUCTOR_WITH_NS(chaos::driver::gibcontrol, ChaosGibControlDD) {
	devicedriver = NULL;
}
ChaosGibControlDD::~ChaosGibControlDD() {
}
void ChaosGibControlDD::driverDeinit() throw(chaos::CException) {
	 if(devicedriver) {
		delete devicedriver;
	}
	devicedriver = NULL;
}
cu_driver::MsgManagmentResultType::MsgManagmentResult ChaosGibControlDD::execOpcode(cu_driver::DrvMsgPtr cmd){
	 cu_driver::MsgManagmentResultType::MsgManagmentResult result = cu_driver::MsgManagmentResultType::MMR_EXECUTED;
	gibcontrol_iparams_t *in = (gibcontrol_iparams_t *)cmd->inputData;
	gibcontrol_oparams_t *out = (gibcontrol_oparams_t *)cmd->resultData;
	 switch(cmd->opcode){
		case OP_INIT:
			ACDBG<< "Initializing";
			out->result = devicedriver->init(NULL);
			break;
		case OP_GET_FEATURE:
			{uint64_t feat=devicedriver->getFeatures();
			out->int64_t1=feat;
			ACDBG<<"Got Features:"<<feat;
			}break;
		case OP_SETPULSE: {
			out->result=devicedriver->setPulse(in->int32_t1,in->int32_t2,in->int32_t3,in->int32_t4);
			ACDBG << "Sent to driver command setPulse result is" << out->result;
			} break;
		case OP_SETCHANNELVOLTAGE: {
			out->result=devicedriver->setChannelVoltage(in->int32_t1,in->double1);
			ACDBG << "Sent to driver command setChannelVoltage result is" << out->result;
			} break;
		case OP_POWERON: {
			out->result=devicedriver->PowerOn(in->int32_t1);
			ACDBG << "Sent to driver command PowerOn result is" << out->result;
			} break;
		case OP_GETSTATE: {
			out->result=devicedriver->getState(out->int32_tP1,out->stringE1);
			ACDBG << "Sent to driver command getState result is" << out->result;
			} break;
	}
	return result;
}
