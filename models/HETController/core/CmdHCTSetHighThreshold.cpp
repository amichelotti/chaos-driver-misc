/*
CmdHCTSetHighThreshold.cpp
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
#include "CmdHCTSetHighThreshold.h"

#include <cmath>
#include  <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <sstream>
#define SCLAPP_ INFO_LOG(CmdHCTSetHighThreshold) << "[" << getDeviceID() << "] "
#define SCLDBG_ DBG_LOG(CmdHCTSetHighThreshold) << "[" << getDeviceID() << "] "
#define SCLERR_ ERR_LOG(CmdHCTSetHighThreshold) << "[" << getDeviceID() << "] "
namespace own = driver::hetcontroller;
namespace c_data =  chaos::common::data;
namespace chaos_batch = chaos::common::batch_command;
using namespace chaos::cu::control_manager;
BATCH_COMMAND_OPEN_DESCRIPTION_ALIAS(driver::hetcontroller::,CmdHCTSetHighThreshold,CMD_HCT_SETHIGHTHRESHOLD_ALIAS,
	"set the threshold high for a channel",
	"036e2237-b895-4171-95c2-24745fe52c4f")
BATCH_COMMAND_ADD_INT32_PARAM(CMD_HCT_SETHIGHTHRESHOLD_SIDE,"0=positrons, 1=electrons",chaos::common::batch_command::BatchCommandAndParameterDescriptionkey::BC_PARAMETER_FLAG_MANDATORY)
BATCH_COMMAND_ADD_INT32_PARAM(CMD_HCT_SETHIGHTHRESHOLD_CHANNEL,"-1 for all",chaos::common::batch_command::BatchCommandAndParameterDescriptionkey::BC_PARAMETER_FLAG_MANDATORY)
BATCH_COMMAND_ADD_INT32_PARAM(CMD_HCT_SETHIGHTHRESHOLD_THRMILLIVOLT,"the threshold value to be set in millivolt",chaos::common::batch_command::BatchCommandAndParameterDescriptionkey::BC_PARAMETER_FLAG_MANDATORY)
BATCH_COMMAND_CLOSE_DESCRIPTION()


// return the implemented handler
uint8_t own::CmdHCTSetHighThreshold::implementedHandler(){
	return      AbstractHETControllerCommand::implementedHandler()|chaos_batch::HandlerType::HT_Acquisition;
}
// empty set handler
void own::CmdHCTSetHighThreshold::setHandler(c_data::CDataWrapper *data) {
	AbstractHETControllerCommand::setHandler(data);
	SCLAPP_ << "Set Handler SetHighThreshold "; 
	setStateVariableSeverity(StateVariableTypeAlarmCU,"driver_command_error",chaos::common::alarm::MultiSeverityAlarmLevelClear);
	if(!data || !data->hasKey(CMD_HCT_SETHIGHTHRESHOLD_SIDE))
	{
		SCLERR_ << "side not present in CDataWrapper";
		metadataLogging(chaos::common::metadata_logging::StandardLoggingChannel::LogLevelError,"side not present in CDataWrapper" );
		setWorkState(false);
		BC_FAULT_RUNNING_PROPERTY
		return;
	}
	int32_t tmp_side=data->getInt32Value(CMD_HCT_SETHIGHTHRESHOLD_SIDE);

	if(!data || !data->hasKey(CMD_HCT_SETHIGHTHRESHOLD_CHANNEL))
	{
		SCLERR_ << "channel not present in CDataWrapper";
		metadataLogging(chaos::common::metadata_logging::StandardLoggingChannel::LogLevelError,"channel not present in CDataWrapper" );
		setWorkState(false);
		BC_FAULT_RUNNING_PROPERTY
		return;
	}
	int32_t tmp_channel=data->getInt32Value(CMD_HCT_SETHIGHTHRESHOLD_CHANNEL);

	if(!data || !data->hasKey(CMD_HCT_SETHIGHTHRESHOLD_THRMILLIVOLT))
	{
		SCLERR_ << "thrMillivolt not present in CDataWrapper";
		metadataLogging(chaos::common::metadata_logging::StandardLoggingChannel::LogLevelError,"thrMillivolt not present in CDataWrapper" );
		setWorkState(false);
		BC_FAULT_RUNNING_PROPERTY
		return;
	}
	int32_t tmp_thrMillivolt=data->getInt32Value(CMD_HCT_SETHIGHTHRESHOLD_THRMILLIVOLT);

	int err=0;
	if ((err=hetcontroller_drv->SetHighThreshold(tmp_side,tmp_channel,tmp_thrMillivolt)) != 0)
	{
		metadataLogging(chaos::common::metadata_logging::StandardLoggingChannel::LogLevelError," command SetHighThreshold not acknowledged");
		setStateVariableSeverity(StateVariableTypeAlarmCU,"driver_command_error",chaos::common::alarm::MultiSeverityAlarmLevelHigh);
	}
	setWorkState(true);
	BC_NORMAL_RUNNING_PROPERTY
}
// empty acquire handler
void own::CmdHCTSetHighThreshold::acquireHandler() {
	SCLDBG_ << "Acquire Handler SetHighThreshold "; 
}
// empty correlation handler
void own::CmdHCTSetHighThreshold::ccHandler() {
	BC_END_RUNNING_PROPERTY;
}
// empty timeout handler
bool own::CmdHCTSetHighThreshold::timeoutHandler() {
	SCLDBG_ << "Timeout Handler SetHighThreshold "; 
	return false;
}
