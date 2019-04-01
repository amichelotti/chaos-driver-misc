/*
CmdHPCSetLowThreshold.cpp
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
#include "CmdHPCSetLowThreshold.h"

#include <cmath>
#include  <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <sstream>
#define SCLAPP_ INFO_LOG(CmdHPCSetLowThreshold) << "[" << getDeviceID() << "] "
#define SCLDBG_ DBG_LOG(CmdHPCSetLowThreshold) << "[" << getDeviceID() << "] "
#define SCLERR_ ERR_LOG(CmdHPCSetLowThreshold) << "[" << getDeviceID() << "] "
namespace own = driver::hetpic;
namespace c_data =  chaos::common::data;
namespace chaos_batch = chaos::common::batch_command;
using namespace chaos::cu::control_manager;
BATCH_COMMAND_OPEN_DESCRIPTION_ALIAS(driver::hetpic::,CmdHPCSetLowThreshold,CMD_HPC_SETLOWTHRESHOLD_ALIAS,
	"set the low threshold for a specified (or all) channel",
	"70f9c1a1-abc0-455a-ab29-3cd5ae7bcf78")
BATCH_COMMAND_ADD_INT32_PARAM(CMD_HPC_SETLOWTHRESHOLD_CHANNEL,"the channel to set (-1 for all)",chaos::common::batch_command::BatchCommandAndParameterDescriptionkey::BC_PARAMETER_FLAG_MANDATORY)
BATCH_COMMAND_ADD_INT32_PARAM(CMD_HPC_SETLOWTHRESHOLD_MILLIVOLTS,"the millivolt value to be set on",chaos::common::batch_command::BatchCommandAndParameterDescriptionkey::BC_PARAMETER_FLAG_MANDATORY)
BATCH_COMMAND_CLOSE_DESCRIPTION()


// return the implemented handler
uint8_t own::CmdHPCSetLowThreshold::implementedHandler(){
	return      AbstractHETPicCommand::implementedHandler()|chaos_batch::HandlerType::HT_Acquisition;
}
// empty set handler
void own::CmdHPCSetLowThreshold::setHandler(c_data::CDataWrapper *data) {
	AbstractHETPicCommand::setHandler(data);
	SCLAPP_ << "Set Handler SetLowThreshold "; 
	setStateVariableSeverity(StateVariableTypeAlarmCU,"driver_command_error",chaos::common::alarm::MultiSeverityAlarmLevelClear);
	if(!data || !data->hasKey(CMD_HPC_SETLOWTHRESHOLD_CHANNEL))
	{
		SCLERR_ << "channel not present in CDataWrapper";
		metadataLogging(chaos::common::metadata_logging::StandardLoggingChannel::LogLevelError,"channel not present in CDataWrapper" );
		setWorkState(false);
		BC_FAULT_RUNNING_PROPERTY
		return;
	}
	int32_t tmp_channel=data->getInt32Value(CMD_HPC_SETLOWTHRESHOLD_CHANNEL);

	if(!data || !data->hasKey(CMD_HPC_SETLOWTHRESHOLD_MILLIVOLTS))
	{
		SCLERR_ << "millivolts not present in CDataWrapper";
		metadataLogging(chaos::common::metadata_logging::StandardLoggingChannel::LogLevelError,"millivolts not present in CDataWrapper" );
		setWorkState(false);
		BC_FAULT_RUNNING_PROPERTY
		return;
	}
	int32_t tmp_millivolts=data->getInt32Value(CMD_HPC_SETLOWTHRESHOLD_MILLIVOLTS);

	int err=0;
	if ((err=hetpic_drv->SetLowThreshold(tmp_channel,tmp_millivolts)) != 0)
	{
		metadataLogging(chaos::common::metadata_logging::StandardLoggingChannel::LogLevelError," command SetLowThreshold not acknowledged");
		setStateVariableSeverity(StateVariableTypeAlarmCU,"driver_command_error",chaos::common::alarm::MultiSeverityAlarmLevelHigh);
	}
	setWorkState(true);
	BC_NORMAL_RUNNING_PROPERTY
}
// empty acquire handler
void own::CmdHPCSetLowThreshold::acquireHandler() {
	SCLDBG_ << "Acquire Handler SetLowThreshold "; 
}
// empty correlation handler
void own::CmdHPCSetLowThreshold::ccHandler() {
	BC_END_RUNNING_PROPERTY;
}
// empty timeout handler
bool own::CmdHPCSetLowThreshold::timeoutHandler() {
	SCLDBG_ << "Timeout Handler SetLowThreshold "; 
	return false;
}
