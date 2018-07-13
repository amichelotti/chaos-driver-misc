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
BATCH_COMMAND_OPEN_DESCRIPTION_ALIAS(driver::gibcontrol::,CmdGIBsetPulse,CMD_GIB_SETPULSE_ALIAS,
			"pulse state (0:off, 1:on)",
			"c3f9b57b-63cc-4b77-9203-49805c56b04c")
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
}
// empty acquire handler
void own::CmdGIBsetPulse::acquireHandler() {
}
// empty correlation handler
void own::CmdGIBsetPulse::ccHandler() {
	BC_END_RUNNING_PROPERTY;
}
// empty timeout handler
bool own::CmdGIBsetPulse::timeoutHandler() {
	return false;
}
