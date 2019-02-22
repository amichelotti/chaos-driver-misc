/*
CmdHPCsetPulse.cpp
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
#include "CmdHPCsetPulse.h"

#include <cmath>
#include  <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <sstream>
#define SCLAPP_ INFO_LOG(CmdHPCsetPulse) << "[" << getDeviceID() << "] "
#define SCLDBG_ DBG_LOG(CmdHPCsetPulse) << "[" << getDeviceID() << "] "
#define SCLERR_ ERR_LOG(CmdHPCsetPulse) << "[" << getDeviceID() << "] "
namespace own = driver::hetpic;
namespace c_data =  chaos::common::data;
namespace chaos_batch = chaos::common::batch_command;
using namespace chaos::cu::control_manager;
BATCH_COMMAND_OPEN_DESCRIPTION_ALIAS(driver::hetpic::,CmdHPCsetPulse,CMD_HPC_SETPULSE_ALIAS,
	"send the command to start pulsing at the specified value",
	"78fb177c-54ab-4b3b-8215-d563613d5314")
BATCH_COMMAND_ADD_INT32_PARAM(CMD_HPC_SETPULSE_VALUE,"0 for off",chaos::common::batch_command::BatchCommandAndParameterDescriptionkey::BC_PARAMETER_FLAG_MANDATORY)
BATCH_COMMAND_CLOSE_DESCRIPTION()


// return the implemented handler
uint8_t own::CmdHPCsetPulse::implementedHandler(){
	return      AbstractHETPicCommand::implementedHandler()|chaos_batch::HandlerType::HT_Acquisition;
}
// empty set handler
void own::CmdHPCsetPulse::setHandler(c_data::CDataWrapper *data) {
	AbstractHETPicCommand::setHandler(data);
	SCLAPP_ << "Set Handler setPulse "; 
	setStateVariableSeverity(StateVariableTypeAlarmCU,"driver_command_error",chaos::common::alarm::MultiSeverityAlarmLevelClear);
	if(!data || !data->hasKey(CMD_HPC_SETPULSE_VALUE))
	{
		SCLERR_ << "value not present in CDataWrapper";
		metadataLogging(chaos::common::metadata_logging::StandardLoggingChannel::LogLevelError,"value not present in CDataWrapper" );
		setWorkState(false);
		BC_FAULT_RUNNING_PROPERTY
		return;
	}
	int32_t tmp_value=data->getInt32Value(CMD_HPC_SETPULSE_VALUE);

	int err=0;
	if ((err=hetpic_drv->setPulse(tmp_value)) != 0)
	{
		metadataLogging(chaos::common::metadata_logging::StandardLoggingChannel::LogLevelError," command setPulse not acknowledged");
		setStateVariableSeverity(StateVariableTypeAlarmCU,"driver_command_error",chaos::common::alarm::MultiSeverityAlarmLevelHigh);
	}
	setWorkState(true);
	BC_NORMAL_RUNNING_PROPERTY
}
// empty acquire handler
void own::CmdHPCsetPulse::acquireHandler() {
	SCLDBG_ << "Acquire Handler setPulse "; 
}
// empty correlation handler
void own::CmdHPCsetPulse::ccHandler() {
	BC_END_RUNNING_PROPERTY;
}
// empty timeout handler
bool own::CmdHPCsetPulse::timeoutHandler() {
	SCLDBG_ << "Timeout Handler setPulse "; 
	return false;
}
