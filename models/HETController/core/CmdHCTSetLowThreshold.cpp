/*
CmdHCTSetLowThreshold.cpp
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
#include "CmdHCTSetLowThreshold.h"

#include <cmath>
#include  <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <sstream>
#define SCLAPP_ INFO_LOG(CmdHCTSetLowThreshold) << "[" << getDeviceID() << "] "
#define SCLDBG_ DBG_LOG(CmdHCTSetLowThreshold) << "[" << getDeviceID() << "] "
#define SCLERR_ ERR_LOG(CmdHCTSetLowThreshold) << "[" << getDeviceID() << "] "
namespace own = driver::hetcontroller;
namespace c_data =  chaos::common::data;
namespace chaos_batch = chaos::common::batch_command;
using namespace chaos::cu::control_manager;
BATCH_COMMAND_OPEN_DESCRIPTION_ALIAS(driver::hetcontroller::,CmdHCTSetLowThreshold,CMD_HCT_SETLOWTHRESHOLD_ALIAS,
	"set the threshold low for a channel",
	"774450a0-02bb-4b95-bf88-8fcdc8397751")
BATCH_COMMAND_ADD_INT32_PARAM(CMD_HCT_SETLOWTHRESHOLD_SIDE,"0=positrons, 1=electrons",chaos::common::batch_command::BatchCommandAndParameterDescriptionkey::BC_PARAMETER_FLAG_MANDATORY)
BATCH_COMMAND_ADD_INT32_PARAM(CMD_HCT_SETLOWTHRESHOLD_CHANNEL,"-1 for all",chaos::common::batch_command::BatchCommandAndParameterDescriptionkey::BC_PARAMETER_FLAG_MANDATORY)
BATCH_COMMAND_ADD_INT32_PARAM(CMD_HCT_SETLOWTHRESHOLD_THRMILLIVOLT,"the threshold value to be set in millivolt",chaos::common::batch_command::BatchCommandAndParameterDescriptionkey::BC_PARAMETER_FLAG_MANDATORY)
BATCH_COMMAND_CLOSE_DESCRIPTION()


// return the implemented handler
uint8_t own::CmdHCTSetLowThreshold::implementedHandler(){
	return      AbstractHETControllerCommand::implementedHandler()|chaos_batch::HandlerType::HT_Acquisition;
}
// empty set handler
void own::CmdHCTSetLowThreshold::setHandler(c_data::CDataWrapper *data) {
	AbstractHETControllerCommand::setHandler(data);
	SCLAPP_ << "Set Handler SetLowThreshold "; 
	setStateVariableSeverity(StateVariableTypeAlarmCU,"driver_command_error",chaos::common::alarm::MultiSeverityAlarmLevelClear);
	if(!data || !data->hasKey(CMD_HCT_SETLOWTHRESHOLD_SIDE))
	{
		SCLERR_ << "side not present in CDataWrapper";
		metadataLogging(chaos::common::metadata_logging::StandardLoggingChannel::LogLevelError,"side not present in CDataWrapper" );
		setWorkState(false);
		BC_FAULT_RUNNING_PROPERTY
		return;
	}
	int32_t tmp_side=data->getInt32Value(CMD_HCT_SETLOWTHRESHOLD_SIDE);

	if(!data || !data->hasKey(CMD_HCT_SETLOWTHRESHOLD_CHANNEL))
	{
		SCLERR_ << "channel not present in CDataWrapper";
		metadataLogging(chaos::common::metadata_logging::StandardLoggingChannel::LogLevelError,"channel not present in CDataWrapper" );
		setWorkState(false);
		BC_FAULT_RUNNING_PROPERTY
		return;
	}
	int32_t tmp_channel=data->getInt32Value(CMD_HCT_SETLOWTHRESHOLD_CHANNEL);

	if(!data || !data->hasKey(CMD_HCT_SETLOWTHRESHOLD_THRMILLIVOLT))
	{
		SCLERR_ << "thrMillivolt not present in CDataWrapper";
		metadataLogging(chaos::common::metadata_logging::StandardLoggingChannel::LogLevelError,"thrMillivolt not present in CDataWrapper" );
		setWorkState(false);
		BC_FAULT_RUNNING_PROPERTY
		return;
	}
	int32_t tmp_thrMillivolt=data->getInt32Value(CMD_HCT_SETLOWTHRESHOLD_THRMILLIVOLT);

	int err=0;
	if ((err=hetcontroller_drv->SetLowThreshold(tmp_side,tmp_channel,tmp_thrMillivolt)) != 0)
	{
		metadataLogging(chaos::common::metadata_logging::StandardLoggingChannel::LogLevelError," command SetLowThreshold not acknowledged");
		setStateVariableSeverity(StateVariableTypeAlarmCU,"driver_command_error",chaos::common::alarm::MultiSeverityAlarmLevelHigh);
	}
	setWorkState(true);
	BC_NORMAL_RUNNING_PROPERTY
}
// empty acquire handler
void own::CmdHCTSetLowThreshold::acquireHandler() {
	SCLDBG_ << "Acquire Handler SetLowThreshold "; 
}
// empty correlation handler
void own::CmdHCTSetLowThreshold::ccHandler() {
	BC_END_RUNNING_PROPERTY;
}
// empty timeout handler
bool own::CmdHCTSetLowThreshold::timeoutHandler() {
	SCLDBG_ << "Timeout Handler SetLowThreshold "; 
	return false;
}
