/*
CmdGIBsetPulse.cpp
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
#include "CmdGIBsetPulse.h"

#include <cmath>
#include  <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <sstream>
#define SCLAPP_ INFO_LOG(CmdGIBsetPulse) << "[" << getDeviceID() << "] "
#define SCLDBG_ DBG_LOG(CmdGIBsetPulse) << "[" << getDeviceID() << "] "
#define SCLERR_ ERR_LOG(CmdGIBsetPulse) << "[" << getDeviceID() << "] "
namespace own = driver::gibcontrol;
namespace c_data =  chaos::common::data;
namespace chaos_batch = chaos::common::batch_command;
using namespace chaos::cu::control_manager;
BATCH_COMMAND_OPEN_DESCRIPTION_ALIAS(driver::gibcontrol::,CmdGIBsetPulse,CMD_GIB_SETPULSE_ALIAS,
			"set Pulse on Channel",
			"33feeaf0-b113-4a43-a0e5-82299e04a697")
BATCH_COMMAND_ADD_INT32_PARAM(CMD_GIB_SETPULSE_CHANNEL,"channel to pulse",chaos::common::batch_command::BatchCommandAndParameterDescriptionkey::BC_PARAMETER_FLAG_MANDATORY)
BATCH_COMMAND_ADD_INT32_PARAM(CMD_GIB_SETPULSE_AMPLITUDE,"amplitude of pulse (V)",chaos::common::batch_command::BatchCommandAndParameterDescriptionkey::BC_PARAMETER_FLAG_MANDATORY)
BATCH_COMMAND_ADD_INT32_PARAM(CMD_GIB_SETPULSE_WIDTH,"pulse width (ns)",chaos::common::batch_command::BatchCommandAndParameterDescriptionkey::BC_PARAMETER_FLAG_MANDATORY)
BATCH_COMMAND_ADD_INT32_PARAM(CMD_GIB_SETPULSE_STATE,"pulse state (0:off, 1:on)",chaos::common::batch_command::BatchCommandAndParameterDescriptionkey::BC_PARAMETER_FLAG_MANDATORY)
BATCH_COMMAND_CLOSE_DESCRIPTION()


// return the implemented handler
uint8_t own::CmdGIBsetPulse::implementedHandler(){
	return      AbstractGibControlCommand::implementedHandler()|chaos_batch::HandlerType::HT_Acquisition;
}
// empty set handler
void own::CmdGIBsetPulse::setHandler(c_data::CDataWrapper *data) {
	AbstractGibControlCommand::setHandler(data);
	SCLAPP_ << "Set Handler setPulse "; 
	setStateVariableSeverity(StateVariableTypeAlarmCU,"driver_command_error",chaos::common::alarm::MultiSeverityAlarmLevelClear);
	int32_t tmp_channel=data->getInt32Value(CMD_GIB_SETPULSE_CHANNEL);
	int32_t tmp_amplitude=data->getInt32Value(CMD_GIB_SETPULSE_AMPLITUDE);
	int32_t tmp_width=data->getInt32Value(CMD_GIB_SETPULSE_WIDTH);
	int32_t tmp_state=data->getInt32Value(CMD_GIB_SETPULSE_STATE);
	int err=0;
	if (err=gibcontrol_drv->setPulse(tmp_channel,tmp_amplitude,tmp_width,tmp_state) != 0)
	{
		metadataLogging(chaos::common::metadata_logging::StandardLoggingChannel::LogLevelError," command setPulse not acknowledged");
		setStateVariableSeverity(StateVariableTypeAlarmCU,"driver_command_error",chaos::common::alarm::MultiSeverityAlarmLevelHigh);
	}
	setWorkState(true);
	BC_NORMAL_RUNNING_PROPERTY
}
// empty acquire handler
void own::CmdGIBsetPulse::acquireHandler() {
	SCLDBG_ << "Acquire Handler setPulse "; 
}
// empty correlation handler
void own::CmdGIBsetPulse::ccHandler() {
	BC_END_RUNNING_PROPERTY;
}
// empty timeout handler
bool own::CmdGIBsetPulse::timeoutHandler() {
	return false;
}
