/*
CmdGIBsetThrVoltage.cpp
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
#include "CmdGIBsetThrVoltage.h"

#include <cmath>
#include  <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <sstream>
#define SCLAPP_ INFO_LOG(CmdGIBsetThrVoltage) << "[" << getDeviceID() << "] "
#define SCLDBG_ DBG_LOG(CmdGIBsetThrVoltage) << "[" << getDeviceID() << "] "
#define SCLERR_ ERR_LOG(CmdGIBsetThrVoltage) << "[" << getDeviceID() << "] "
namespace own = driver::gibcontrol;
namespace c_data =  chaos::common::data;
namespace chaos_batch = chaos::common::batch_command;
BATCH_COMMAND_OPEN_DESCRIPTION_ALIAS(driver::gibcontrol::,CmdGIBsetThrVoltage,CMD_GIB_SETTHRVOLTAGE_ALIAS,
			"the threshold voltage ti be set (mv)",
			"56496013-17d6-4af7-bc57-aca38329f2cd")
BATCH_COMMAND_ADD_INT32_PARAM(CMD_GIB_SETTHRVOLTAGE_CHANNEL,"the channel to be set",chaos::common::batch_command::BatchCommandAndParameterDescriptionkey::BC_PARAMETER_FLAG_MANDATORY)
BATCH_COMMAND_ADD_DOUBLE_PARAM(CMD_GIB_SETTHRVOLTAGE_THRVOLTAGE,"the threshold voltage ti be set (mv)",chaos::common::batch_command::BatchCommandAndParameterDescriptionkey::BC_PARAMETER_FLAG_MANDATORY)
BATCH_COMMAND_CLOSE_DESCRIPTION()


// return the implemented handler
uint8_t own::CmdGIBsetThrVoltage::implementedHandler(){
	return      AbstractGibControlCommand::implementedHandler()|chaos_batch::HandlerType::HT_Acquisition;
}
// empty set handler
void own::CmdGIBsetThrVoltage::setHandler(c_data::CDataWrapper *data) {
	AbstractGibControlCommand::setHandler(data);
}
// empty acquire handler
void own::CmdGIBsetThrVoltage::acquireHandler() {
}
// empty correlation handler
void own::CmdGIBsetThrVoltage::ccHandler() {
	BC_END_RUNNING_PROPERTY;
}
// empty timeout handler
bool own::CmdGIBsetThrVoltage::timeoutHandler() {
	return false;
}
