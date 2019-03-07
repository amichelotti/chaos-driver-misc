/*
CmdHCTSwitchHVPower.cpp
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
#include "CmdHCTSwitchHVPower.h"

#include <cmath>
#include  <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <sstream>
#define SCLAPP_ INFO_LOG(CmdHCTSwitchHVPower) << "[" << getDeviceID() << "] "
#define SCLDBG_ DBG_LOG(CmdHCTSwitchHVPower) << "[" << getDeviceID() << "] "
#define SCLERR_ ERR_LOG(CmdHCTSwitchHVPower) << "[" << getDeviceID() << "] "
namespace own = driver::hetcontroller;
namespace c_data =  chaos::common::data;
namespace chaos_batch = chaos::common::batch_command;
using namespace chaos::cu::control_manager;
BATCH_COMMAND_OPEN_DESCRIPTION_ALIAS(driver::hetcontroller::,CmdHCTSwitchHVPower,CMD_HCT_SWITCHHVPOWER_ALIAS,
	"Turn on and off all the HV channels",
	"cedd3e83-6fb5-4b51-a6e3-748534772445")
BATCH_COMMAND_ADD_INT32_PARAM(CMD_HCT_SWITCHHVPOWER_ON_STATE,"on=1, off=0",chaos::common::batch_command::BatchCommandAndParameterDescriptionkey::BC_PARAMETER_FLAG_MANDATORY)
BATCH_COMMAND_CLOSE_DESCRIPTION()


// return the implemented handler
uint8_t own::CmdHCTSwitchHVPower::implementedHandler(){
	return      AbstractHETControllerCommand::implementedHandler()|chaos_batch::HandlerType::HT_Acquisition;
}
// empty set handler
void own::CmdHCTSwitchHVPower::setHandler(c_data::CDataWrapper *data) {
	AbstractHETControllerCommand::setHandler(data);
	SCLAPP_ << "Set Handler SwitchHVPower "; 
	setStateVariableSeverity(StateVariableTypeAlarmCU,"driver_command_error",chaos::common::alarm::MultiSeverityAlarmLevelClear);
	if(!data || !data->hasKey(CMD_HCT_SWITCHHVPOWER_ON_STATE))
	{
		SCLERR_ << "on_state not present in CDataWrapper";
		metadataLogging(chaos::common::metadata_logging::StandardLoggingChannel::LogLevelError,"on_state not present in CDataWrapper" );
		setWorkState(false);
		BC_FAULT_RUNNING_PROPERTY
		return;
	}
	int32_t tmp_on_state=data->getInt32Value(CMD_HCT_SWITCHHVPOWER_ON_STATE);

	int err=0;
	if ((err=hetcontroller_drv->SwitchHVPower(tmp_on_state)) != 0)
	{
		metadataLogging(chaos::common::metadata_logging::StandardLoggingChannel::LogLevelError," command SwitchHVPower not acknowledged");
		setStateVariableSeverity(StateVariableTypeAlarmCU,"driver_command_error",chaos::common::alarm::MultiSeverityAlarmLevelHigh);
	}
	setWorkState(true);
	BC_NORMAL_RUNNING_PROPERTY
}
// empty acquire handler
void own::CmdHCTSwitchHVPower::acquireHandler() {
	SCLDBG_ << "Acquire Handler SwitchHVPower "; 
}
// empty correlation handler
void own::CmdHCTSwitchHVPower::ccHandler() {
	BC_END_RUNNING_PROPERTY;
}
// empty timeout handler
bool own::CmdHCTSwitchHVPower::timeoutHandler() {
	SCLDBG_ << "Timeout Handler SwitchHVPower "; 
	return false;
}
