/*
CmdGIBsetChannelVoltage.cpp
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
#include "CmdGIBsetChannelVoltage.h"

#include <cmath>
#include  <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <sstream>
#define SCLAPP_ INFO_LOG(CmdGIBsetChannelVoltage) << "[" << getDeviceID() << "] "
#define SCLDBG_ DBG_LOG(CmdGIBsetChannelVoltage) << "[" << getDeviceID() << "] "
#define SCLERR_ ERR_LOG(CmdGIBsetChannelVoltage) << "[" << getDeviceID() << "] "
namespace own = driver::gibcontrol;
namespace c_data =  chaos::common::data;
namespace chaos_batch = chaos::common::batch_command;
using namespace chaos::cu::control_manager;
using namespace ::common::gibcontrol ;
BATCH_COMMAND_OPEN_DESCRIPTION_ALIAS(driver::gibcontrol::,CmdGIBsetChannelVoltage,CMD_GIB_SETCHANNELVOLTAGE_ALIAS,
			"set the voltage to a Channel",
			"28ab2b93-2c92-455c-b18f-ec91b05bf4ce")
BATCH_COMMAND_ADD_INT32_PARAM(CMD_GIB_SETCHANNELVOLTAGE_CHANNEL,"the channel to set",chaos::common::batch_command::BatchCommandAndParameterDescriptionkey::BC_PARAMETER_FLAG_MANDATORY)
BATCH_COMMAND_ADD_DOUBLE_PARAM(CMD_GIB_SETCHANNELVOLTAGE_VOLTAGE,"the voltage setPoint",chaos::common::batch_command::BatchCommandAndParameterDescriptionkey::BC_PARAMETER_FLAG_MANDATORY)
BATCH_COMMAND_CLOSE_DESCRIPTION()


// return the implemented handler
uint8_t own::CmdGIBsetChannelVoltage::implementedHandler(){
	return      AbstractGibControlCommand::implementedHandler()|chaos_batch::HandlerType::HT_Acquisition;
}
// empty set handler
void own::CmdGIBsetChannelVoltage::setHandler(c_data::CDataWrapper *data) {
	AbstractGibControlCommand::setHandler(data);
	SCLAPP_ << "Set Handler setChannelVoltage "; 
	setStateVariableSeverity(StateVariableTypeAlarmCU,"driver_command_error",chaos::common::alarm::MultiSeverityAlarmLevelClear);
	int32_t tmp_channel=data->getInt32Value(CMD_GIB_SETCHANNELVOLTAGE_CHANNEL);
	double tmp_Voltage=data->getDoubleValue(CMD_GIB_SETCHANNELVOLTAGE_VOLTAGE);
	this->chanNum=tmp_channel;
	this->setValue=tmp_Voltage;
	int err=0;
	if ( CHECKMASK(*o_status_id,::common::gibcontrol::GIBCONTROL_SUPPLIED) == false )
	{
		metadataLogging(chaos::common::metadata_logging::StandardLoggingChannel::LogLevelWarning,"cannot set HV channels when device is off");
		BC_END_RUNNING_PROPERTY
	}
	//if ()



	if (err=gibcontrol_drv->setChannelVoltage(tmp_channel,tmp_Voltage) != 0)
	{
		metadataLogging(chaos::common::metadata_logging::StandardLoggingChannel::LogLevelError," command setChannelVoltage not acknowledged");
		setStateVariableSeverity(StateVariableTypeAlarmCU,"driver_command_error",chaos::common::alarm::MultiSeverityAlarmLevelHigh);
	}
	setWorkState(true);
	BC_NORMAL_RUNNING_PROPERTY
}
// empty acquire handler
void own::CmdGIBsetChannelVoltage::acquireHandler() {
	SCLDBG_ << "Acquire Handler setChannelVoltage "; 
}
// empty correlation handler
void own::CmdGIBsetChannelVoltage::ccHandler() {
	BC_END_RUNNING_PROPERTY;
}
// empty timeout handler
bool own::CmdGIBsetChannelVoltage::timeoutHandler() {
	return false;
}
