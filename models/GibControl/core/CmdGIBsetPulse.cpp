/*
CmdGIBsetPulse.cpp
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
#include "CmdGIBsetPulse.h"

#include <cmath>
#include  <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <sstream>
#define SCLAPP_ INFO_LOG(CmdGIBsetPulse) << "[" << getDeviceID() << "] "
#define SCLDBG_ DBG_LOG(CmdGIBsetPulse) << "[" << getDeviceID() << "] "
#define SCLERR_ ERR_LOG(CmdGIBsetPulse) << "[" << getDeviceID() << "] "
namespace own = driver::gibcontrol;
namespace c_data =  chaos::common::data;
namespace chaos_batch = chaos::common::batch_command;
using namespace chaos::cu::control_manager;
BATCH_COMMAND_OPEN_DESCRIPTION_ALIAS(driver::gibcontrol::,CmdGIBsetPulse,CMD_GIB_SETPULSE_ALIAS,
			"set Pulse on Channel",
			"522aafcf-4828-4d1c-8ed4-d3130ae7ee05")
BATCH_COMMAND_ADD_INT32_PARAM(CMD_GIB_SETPULSE_CHANNEL,"channel to pulse",chaos::common::batch_command::BatchCommandAndParameterDescriptionkey::BC_PARAMETER_FLAG_MANDATORY)
BATCH_COMMAND_ADD_INT32_PARAM(CMD_GIB_SETPULSE_AMPLITUDE,"amplitude of pulse (V)",chaos::common::batch_command::BatchCommandAndParameterDescriptionkey::BC_PARAMETER_FLAG_MANDATORY)
BATCH_COMMAND_ADD_INT32_PARAM(CMD_GIB_SETPULSE_WIDTH,"pulse width (ns)",chaos::common::batch_command::BatchCommandAndParameterDescriptionkey::BC_PARAMETER_FLAG_MANDATORY)
BATCH_COMMAND_ADD_INT32_PARAM(CMD_GIB_SETPULSE_STATE,"pulse state (0:off, 1:on)",chaos::common::batch_command::BatchCommandAndParameterDescriptionkey::BC_PARAMETER_FLAG_MANDATORY)
BATCH_COMMAND_CLOSE_DESCRIPTION()


// return the implemented handler
uint8_t own::CmdGIBsetPulse::implementedHandler(){
	return      AbstractGibControlCommand::implementedHandler()|chaos_batch::HandlerType::HT_Acquisition;
}
// empty set handler
void own::CmdGIBsetPulse::setHandler(c_data::CDataWrapper *data) {
	AbstractGibControlCommand::setHandler(data);
	SCLAPP_ << "Set Handler setPulse "; 
	BC_NORMAL_RUNNING_PROPERTY
}
// empty acquire handler
void own::CmdGIBsetPulse::acquireHandler() {
	SCLAPP_ << "Acquire Handler setPulse "; 
}
// empty correlation handler
void own::CmdGIBsetPulse::ccHandler() {
}
// empty timeout handler
bool own::CmdGIBsetPulse::timeoutHandler() {
	return false;
}
