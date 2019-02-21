/*
CmdHPCSetHighThreshold.cpp
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
#include "CmdHPCSetHighThreshold.h"

#include <cmath>
#include  <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <sstream>
#define SCLAPP_ INFO_LOG(CmdHPCSetHighThreshold) << "[" << getDeviceID() << "] "
#define SCLDBG_ DBG_LOG(CmdHPCSetHighThreshold) << "[" << getDeviceID() << "] "
#define SCLERR_ ERR_LOG(CmdHPCSetHighThreshold) << "[" << getDeviceID() << "] "
namespace own = driver::hetpic;
namespace c_data =  chaos::common::data;
namespace chaos_batch = chaos::common::batch_command;
using namespace chaos::cu::control_manager;
BATCH_COMMAND_OPEN_DESCRIPTION_ALIAS(driver::hetpic::,CmdHPCSetHighThreshold,CMD_HPC_SETHIGHTHRESHOLD_ALIAS,
	"set the high threshold for a specified (or all) channel",
	"00acc45b-ffce-45b5-a14b-ed5d5fe7c2f3")
BATCH_COMMAND_ADD_INT32_PARAM(CMD_HPC_SETHIGHTHRESHOLD_CHANNEL,"the channel to set (-1 for all)",chaos::common::batch_command::BatchCommandAndParameterDescriptionkey::BC_PARAMETER_FLAG_MANDATORY)
BATCH_COMMAND_ADD_INT32_PARAM(CMD_HPC_SETHIGHTHRESHOLD_MILLIVOLTS,"the millivolt value to be set on",chaos::common::batch_command::BatchCommandAndParameterDescriptionkey::BC_PARAMETER_FLAG_MANDATORY)
BATCH_COMMAND_CLOSE_DESCRIPTION()


// return the implemented handler
uint8_t own::CmdHPCSetHighThreshold::implementedHandler(){
	return      AbstractHETPicCommand::implementedHandler()|chaos_batch::HandlerType::HT_Acquisition;
}
// empty set handler
void own::CmdHPCSetHighThreshold::setHandler(c_data::CDataWrapper *data) {
	AbstractHETPicCommand::setHandler(data);
	SCLAPP_ << "Set Handler SetHighThreshold "; 
	setStateVariableSeverity(StateVariableTypeAlarmCU,"driver_command_error",chaos::common::alarm::MultiSeverityAlarmLevelClear);
	if(!data || !data->hasKey(CMD_HPC_SETHIGHTHRESHOLD_CHANNEL))
	{
		SCLERR_ << "channel not present in CDataWrapper";
		metadataLogging(chaos::common::metadata_logging::StandardLoggingChannel::LogLevelError,"channel not present in CDataWrapper" );
		setWorkState(false);
		BC_FAULT_RUNNING_PROPERTY
		return;
	}
	int32_t tmp_channel=data->getInt32Value(CMD_HPC_SETHIGHTHRESHOLD_CHANNEL);

	if(!data || !data->hasKey(CMD_HPC_SETHIGHTHRESHOLD_MILLIVOLTS))
	{
		SCLERR_ << "millivolts not present in CDataWrapper";
		metadataLogging(chaos::common::metadata_logging::StandardLoggingChannel::LogLevelError,"millivolts not present in CDataWrapper" );
		setWorkState(false);
		BC_FAULT_RUNNING_PROPERTY
		return;
	}
	int32_t tmp_millivolts=data->getInt32Value(CMD_HPC_SETHIGHTHRESHOLD_MILLIVOLTS);

	int err=0;
	if ((err=hetpic_drv->SetHighThreshold(tmp_channel,tmp_millivolts)) != 0)
	{
		metadataLogging(chaos::common::metadata_logging::StandardLoggingChannel::LogLevelError," command SetHighThreshold not acknowledged");
		setStateVariableSeverity(StateVariableTypeAlarmCU,"driver_command_error",chaos::common::alarm::MultiSeverityAlarmLevelHigh);
	}
	setWorkState(true);
	BC_NORMAL_RUNNING_PROPERTY
}
// empty acquire handler
void own::CmdHPCSetHighThreshold::acquireHandler() {
	SCLDBG_ << "Acquire Handler SetHighThreshold "; 
}
// empty correlation handler
void own::CmdHPCSetHighThreshold::ccHandler() {
	BC_END_RUNNING_PROPERTY;
}
// empty timeout handler
bool own::CmdHPCSetHighThreshold::timeoutHandler() {
	SCLDBG_ << "Timeout Handler SetHighThreshold "; 
	return false;
}
