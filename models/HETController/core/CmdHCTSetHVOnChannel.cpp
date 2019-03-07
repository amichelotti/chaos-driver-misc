/*
CmdHCTSetHVOnChannel.cpp
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
#include "CmdHCTSetHVOnChannel.h"

#include <cmath>
#include  <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <sstream>
#define SCLAPP_ INFO_LOG(CmdHCTSetHVOnChannel) << "[" << getDeviceID() << "] "
#define SCLDBG_ DBG_LOG(CmdHCTSetHVOnChannel) << "[" << getDeviceID() << "] "
#define SCLERR_ ERR_LOG(CmdHCTSetHVOnChannel) << "[" << getDeviceID() << "] "
namespace own = driver::hetcontroller;
namespace c_data =  chaos::common::data;
namespace chaos_batch = chaos::common::batch_command;
using namespace chaos::cu::control_manager;
BATCH_COMMAND_OPEN_DESCRIPTION_ALIAS(driver::hetcontroller::,CmdHCTSetHVOnChannel,CMD_HCT_SETHVONCHANNEL_ALIAS,
	"Set the supply voltage for a channel ",
	"fe7d7e87-a419-4593-99d3-6a84cb875ef0")
BATCH_COMMAND_ADD_INT32_PARAM(CMD_HCT_SETHVONCHANNEL_SLOT,"the slot value",chaos::common::batch_command::BatchCommandAndParameterDescriptionkey::BC_PARAMETER_FLAG_MANDATORY)
BATCH_COMMAND_ADD_INT32_PARAM(CMD_HCT_SETHVONCHANNEL_CHANNEL,"the channel to set",chaos::common::batch_command::BatchCommandAndParameterDescriptionkey::BC_PARAMETER_FLAG_MANDATORY)
BATCH_COMMAND_ADD_INT32_PARAM(CMD_HCT_SETHVONCHANNEL_VALUE,"the voltage value to be set",chaos::common::batch_command::BatchCommandAndParameterDescriptionkey::BC_PARAMETER_FLAG_MANDATORY)
BATCH_COMMAND_CLOSE_DESCRIPTION()


// return the implemented handler
uint8_t own::CmdHCTSetHVOnChannel::implementedHandler(){
	return      AbstractHETControllerCommand::implementedHandler()|chaos_batch::HandlerType::HT_Acquisition;
}
// empty set handler
void own::CmdHCTSetHVOnChannel::setHandler(c_data::CDataWrapper *data) {
	AbstractHETControllerCommand::setHandler(data);
	SCLAPP_ << "Set Handler SetHVOnChannel "; 
	setStateVariableSeverity(StateVariableTypeAlarmCU,"driver_command_error",chaos::common::alarm::MultiSeverityAlarmLevelClear);
	if(!data || !data->hasKey(CMD_HCT_SETHVONCHANNEL_SLOT))
	{
		SCLERR_ << "slot not present in CDataWrapper";
		metadataLogging(chaos::common::metadata_logging::StandardLoggingChannel::LogLevelError,"slot not present in CDataWrapper" );
		setWorkState(false);
		BC_FAULT_RUNNING_PROPERTY
		return;
	}
	int32_t tmp_slot=data->getInt32Value(CMD_HCT_SETHVONCHANNEL_SLOT);

	if(!data || !data->hasKey(CMD_HCT_SETHVONCHANNEL_CHANNEL))
	{
		SCLERR_ << "channel not present in CDataWrapper";
		metadataLogging(chaos::common::metadata_logging::StandardLoggingChannel::LogLevelError,"channel not present in CDataWrapper" );
		setWorkState(false);
		BC_FAULT_RUNNING_PROPERTY
		return;
	}
	int32_t tmp_channel=data->getInt32Value(CMD_HCT_SETHVONCHANNEL_CHANNEL);

	if(!data || !data->hasKey(CMD_HCT_SETHVONCHANNEL_VALUE))
	{
		SCLERR_ << "value not present in CDataWrapper";
		metadataLogging(chaos::common::metadata_logging::StandardLoggingChannel::LogLevelError,"value not present in CDataWrapper" );
		setWorkState(false);
		BC_FAULT_RUNNING_PROPERTY
		return;
	}
	int32_t tmp_value=data->getInt32Value(CMD_HCT_SETHVONCHANNEL_VALUE);

	int err=0;
	if ((err=hetcontroller_drv->SetHVOnChannel(tmp_slot,tmp_channel,tmp_value)) != 0)
	{
		metadataLogging(chaos::common::metadata_logging::StandardLoggingChannel::LogLevelError," command SetHVOnChannel not acknowledged");
		setStateVariableSeverity(StateVariableTypeAlarmCU,"driver_command_error",chaos::common::alarm::MultiSeverityAlarmLevelHigh);
	}
	setWorkState(true);
	BC_NORMAL_RUNNING_PROPERTY
}
// empty acquire handler
void own::CmdHCTSetHVOnChannel::acquireHandler() {
	SCLDBG_ << "Acquire Handler SetHVOnChannel "; 
}
// empty correlation handler
void own::CmdHCTSetHVOnChannel::ccHandler() {
	BC_END_RUNNING_PROPERTY;
}
// empty timeout handler
bool own::CmdHCTSetHVOnChannel::timeoutHandler() {
	SCLDBG_ << "Timeout Handler SetHVOnChannel "; 
	return false;
}
