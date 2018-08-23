/*
CmdGIBPowerOn.cpp
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
#include "CmdGIBPowerOn.h"

#include <cmath>
#include  <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <sstream>
#define SCLAPP_ INFO_LOG(CmdGIBPowerOn) << "[" << getDeviceID() << "] "
#define SCLDBG_ DBG_LOG(CmdGIBPowerOn) << "[" << getDeviceID() << "] "
#define SCLERR_ ERR_LOG(CmdGIBPowerOn) << "[" << getDeviceID() << "] "
namespace own = driver::gibcontrol;
namespace c_data =  chaos::common::data;
namespace chaos_batch = chaos::common::batch_command;
using namespace chaos::cu::control_manager;
BATCH_COMMAND_OPEN_DESCRIPTION_ALIAS(driver::gibcontrol::,CmdGIBPowerOn,CMD_GIB_POWERON_ALIAS,
			"Switch on off the GIB main power",
			"0395634c-43a4-45ea-8468-a7c9d6b3f902")
BATCH_COMMAND_ADD_INT32_PARAM(CMD_GIB_POWERON_ON_STATE,"0= OFF , 1 = ON",chaos::common::batch_command::BatchCommandAndParameterDescriptionkey::BC_PARAMETER_FLAG_MANDATORY)
BATCH_COMMAND_CLOSE_DESCRIPTION()


// return the implemented handler
uint8_t own::CmdGIBPowerOn::implementedHandler(){
	return      AbstractGibControlCommand::implementedHandler()|chaos_batch::HandlerType::HT_Acquisition;
}
// empty set handler
void own::CmdGIBPowerOn::setHandler(c_data::CDataWrapper *data) {
	AbstractGibControlCommand::setHandler(data);
	SCLAPP_ << "Set Handler PowerOn "; 
	BC_NORMAL_RUNNING_PROPERTY
}
// empty acquire handler
void own::CmdGIBPowerOn::acquireHandler() {
	SCLAPP_ << "Acquire Handler PowerOn "; 
}
// empty correlation handler
void own::CmdGIBPowerOn::ccHandler() {
	BC_END_RUNNING_PROPERTY;
}
// empty timeout handler
bool own::CmdGIBPowerOn::timeoutHandler() {
	return false;
}
