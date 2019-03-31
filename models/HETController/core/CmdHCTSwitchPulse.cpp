/*
CmdHCTSwitchPulse.cpp
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
#include "CmdHCTSwitchPulse.h"

#include <cmath>
#include  <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <sstream>
#define SCLAPP_ INFO_LOG(CmdHCTSwitchPulse) << "[" << getDeviceID() << "] "
#define SCLDBG_ DBG_LOG(CmdHCTSwitchPulse) << "[" << getDeviceID() << "] "
#define SCLERR_ ERR_LOG(CmdHCTSwitchPulse) << "[" << getDeviceID() << "] "
namespace own = driver::hetcontroller;
namespace c_data =  chaos::common::data;
namespace chaos_batch = chaos::common::batch_command;
using namespace chaos::cu::control_manager;
BATCH_COMMAND_OPEN_DESCRIPTION_ALIAS(driver::hetcontroller::,CmdHCTSwitchPulse,CMD_HCT_SWITCHPULSE_ALIAS,
	"set on and off pulsing for each side",
	"f7d8da4e-79f7-4f75-a3f7-2663413ca162")
BATCH_COMMAND_ADD_INT32_PARAM(CMD_HCT_SWITCHPULSE_SIDE,"0 positrons, 1 electrons",chaos::common::batch_command::BatchCommandAndParameterDescriptionkey::BC_PARAMETER_FLAG_MANDATORY)
BATCH_COMMAND_ADD_INT32_PARAM(CMD_HCT_SWITCHPULSE_VALUE,"0 for off",chaos::common::batch_command::BatchCommandAndParameterDescriptionkey::BC_PARAMETER_FLAG_MANDATORY)
BATCH_COMMAND_CLOSE_DESCRIPTION()


// return the implemented handler
uint8_t own::CmdHCTSwitchPulse::implementedHandler(){
	return      AbstractHETControllerCommand::implementedHandler()|chaos_batch::HandlerType::HT_Acquisition;
}
// empty set handler
void own::CmdHCTSwitchPulse::setHandler(c_data::CDataWrapper *data) {
	AbstractHETControllerCommand::setHandler(data);
	SCLAPP_ << "Set Handler SwitchPulse "; 
	setStateVariableSeverity(StateVariableTypeAlarmCU,"driver_command_error",chaos::common::alarm::MultiSeverityAlarmLevelClear);
	if(!data || !data->hasKey(CMD_HCT_SWITCHPULSE_SIDE))
	{
		SCLERR_ << "side not present in CDataWrapper";
		metadataLogging(chaos::common::metadata_logging::StandardLoggingChannel::LogLevelError,"side not present in CDataWrapper" );
		setWorkState(false);
		BC_FAULT_RUNNING_PROPERTY
		return;
	}
	int32_t tmp_side=data->getInt32Value(CMD_HCT_SWITCHPULSE_SIDE);

	if(!data || !data->hasKey(CMD_HCT_SWITCHPULSE_VALUE))
	{
		SCLERR_ << "value not present in CDataWrapper";
		metadataLogging(chaos::common::metadata_logging::StandardLoggingChannel::LogLevelError,"value not present in CDataWrapper" );
		setWorkState(false);
		BC_FAULT_RUNNING_PROPERTY
		return;
	}
	int32_t tmp_value=data->getInt32Value(CMD_HCT_SWITCHPULSE_VALUE);

	int err=0;
	if ((err=hetcontroller_drv->SwitchPulse(tmp_side,tmp_value)) != 0)
	{
		metadataLogging(chaos::common::metadata_logging::StandardLoggingChannel::LogLevelError," command SwitchPulse not acknowledged");
		setStateVariableSeverity(StateVariableTypeAlarmCU,"driver_command_error",chaos::common::alarm::MultiSeverityAlarmLevelHigh);
	}
	setWorkState(true);
	BC_NORMAL_RUNNING_PROPERTY
}
// empty acquire handler
void own::CmdHCTSwitchPulse::acquireHandler() {
	SCLDBG_ << "Acquire Handler SwitchPulse "; 
}
// empty correlation handler
void own::CmdHCTSwitchPulse::ccHandler() {
	BC_END_RUNNING_PROPERTY;
}
// empty timeout handler
bool own::CmdHCTSwitchPulse::timeoutHandler() {
	SCLDBG_ << "Timeout Handler SwitchPulse "; 
	return false;
}
