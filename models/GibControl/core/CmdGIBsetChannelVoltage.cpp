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
BATCH_COMMAND_ADD_INT32_PARAM(CMD_GIB_SETCHANNELVOLTAGE_CHANNEL,"the channel to set (-1 ALL)",chaos::common::batch_command::BatchCommandAndParameterDescriptionkey::BC_PARAMETER_FLAG_MANDATORY)
BATCH_COMMAND_ADD_DOUBLE_PARAM(CMD_GIB_SETCHANNELVOLTAGE_VOLTAGE,"the voltage setPoint",chaos::common::batch_command::BatchCommandAndParameterDescriptionkey::BC_PARAMETER_FLAG_MANDATORY)
BATCH_COMMAND_CLOSE_DESCRIPTION()


// return the implemented handler
uint8_t own::CmdGIBsetChannelVoltage::implementedHandler(){
	return      AbstractGibControlCommand::implementedHandler()|chaos_batch::HandlerType::HT_Acquisition;
}
// empty set handler
void own::CmdGIBsetChannelVoltage::setHandler(c_data::CDataWrapper *data) {
	AbstractGibControlCommand::setHandler(data);
	
	inputVoltageResolution= getAttributeCache()->getROPtr<double>(DOMAIN_INPUT,"voltage_channel_resolution");
	SCLAPP_ << "Set Handler setChannelVoltage "; 
	this->clearCUAlarms();
	
	//setStateVariableSeverity(StateVariableTypeAlarmCU,"driver_command_error",chaos::common::alarm::MultiSeverityAlarmLevelClear);
	//setStateVariableSeverity(StateVariableTypeAlarmCU,"bad_command_parameter",chaos::common::alarm::MultiSeverityAlarmLevelClear);
	if(!data || !data->hasKey(CMD_GIB_SETCHANNELVOLTAGE_CHANNEL)) 
	{
		SCLERR_ << "Channel parameter not present";
		metadataLogging(chaos::common::metadata_logging::StandardLoggingChannel::LogLevelError,"Channel parameter  not present" );
		setStateVariableSeverity(StateVariableTypeAlarmCU,"bad_command_parameter",chaos::common::alarm::MultiSeverityAlarmLevelHigh);
		BC_FAULT_RUNNING_PROPERTY;
		return;
	}
	if(!data || !data->hasKey(CMD_GIB_SETCHANNELVOLTAGE_VOLTAGE)) 
	{
		SCLERR_ << "Voltage parameter value not present";
		metadataLogging(chaos::common::metadata_logging::StandardLoggingChannel::LogLevelError,"Voltage parameter  not present" );
		setStateVariableSeverity(StateVariableTypeAlarmCU,"bad_command_parameter",chaos::common::alarm::MultiSeverityAlarmLevelHigh);
		BC_FAULT_RUNNING_PROPERTY;
		return;
	}
	

	int32_t tmp_channel=data->getInt32Value(CMD_GIB_SETCHANNELVOLTAGE_CHANNEL);
	double tmp_Voltage=data->getDoubleValue(CMD_GIB_SETCHANNELVOLTAGE_VOLTAGE);
	this->chanNum=tmp_channel;
	this->setValue=tmp_Voltage;
	if (tmp_channel >= (*this->numOfchannels))
	{
		metadataLogging(chaos::common::metadata_logging::StandardLoggingChannel::LogLevelError,"Channel parameter out of bounds" );
		setStateVariableSeverity(StateVariableTypeAlarmCU,"bad_command_parameter",chaos::common::alarm::MultiSeverityAlarmLevelHigh);
		BC_FAULT_RUNNING_PROPERTY;
		return;
	}
	const double* maxVol = getAttributeCache()->getROPtr<double>(DOMAIN_INPUT, "max_channel_voltage"); 
	if ((maxVol==NULL) || (*maxVol==0) )
	{
		metadataLogging(chaos::common::metadata_logging::StandardLoggingChannel::LogLevelWarning,"max channel voltage value not set in configuration. No Control of data from CU");
		setStateVariableSeverity(StateVariableTypeAlarmCU,"bad_command_parameter",chaos::common::alarm::MultiSeverityAlarmLevelWarning);
	}
	else
	{
		if (*maxVol < tmp_Voltage)
		{
			SCLERR_ << "Voltage parameter over the max (" << *maxVol << ")";
			metadataLogging(chaos::common::metadata_logging::StandardLoggingChannel::LogLevelError,"Voltage parameter too high" );
			setStateVariableSeverity(StateVariableTypeAlarmCU,"bad_command_parameter",chaos::common::alarm::MultiSeverityAlarmLevelHigh);
			BC_FAULT_RUNNING_PROPERTY;
			return;
		}
	}
	const double* minVol = getAttributeCache()->getROPtr<double>(DOMAIN_INPUT, "min_channel_voltage"); 
	if ((minVol==NULL) || (*minVol==0) )
	{
		metadataLogging(chaos::common::metadata_logging::StandardLoggingChannel::LogLevelWarning,"min channel voltage value not set in configuration. No Control of data from CU");
		setStateVariableSeverity(StateVariableTypeAlarmCU,"bad_command_parameter",chaos::common::alarm::MultiSeverityAlarmLevelWarning);
	}
	else
	{
		if (*maxVol < tmp_Voltage)
		{
			SCLERR_ << "Voltage parameter below the min (" << *minVol << ")";
			metadataLogging(chaos::common::metadata_logging::StandardLoggingChannel::LogLevelError,"Voltage parameter too low" );
			setStateVariableSeverity(StateVariableTypeAlarmCU,"bad_command_parameter",chaos::common::alarm::MultiSeverityAlarmLevelHigh);
			BC_FAULT_RUNNING_PROPERTY;
			return;
		}

	}



	int err=0;
	if ( CHECKMASK(*o_status_id,::common::gibcontrol::GIBCONTROL_SUPPLIED) == false )
	{
		metadataLogging(chaos::common::metadata_logging::StandardLoggingChannel::LogLevelWarning,"cannot set HV channels when device is off");
		setStateVariableSeverity(StateVariableTypeAlarmCU,"driver_command_error",chaos::common::alarm::MultiSeverityAlarmLevelHigh);
		BC_FAULT_RUNNING_PROPERTY;
		return;
	}
	
	if (err=gibcontrol_drv->setChannelVoltage(tmp_channel,tmp_Voltage) != 0)
	{
		metadataLogging(chaos::common::metadata_logging::StandardLoggingChannel::LogLevelError," command setChannelVoltage not acknowledged");
		setStateVariableSeverity(StateVariableTypeAlarmCU,"driver_command_error",chaos::common::alarm::MultiSeverityAlarmLevelHigh);
		BC_FAULT_RUNNING_PROPERTY;
		return;
	}
	BC_NORMAL_RUNNING_PROPERTY
}
// empty acquire handler
void own::CmdGIBsetChannelVoltage::acquireHandler() {
	SCLDBG_ << "Acquire Handler setChannelVoltage "; 
}
// empty correlation handler
void own::CmdGIBsetChannelVoltage::ccHandler() {
	int err=0;
	std::vector<double> readChannels;
	if ((err=gibcontrol_drv->getVoltages(readChannels)) != 0)
	{
		metadataLogging(chaos::common::metadata_logging::StandardLoggingChannel::LogLevelError," cannot read voltages from GIB");
		setStateVariableSeverity(StateVariableTypeAlarmCU,"driver_command_error",chaos::common::alarm::MultiSeverityAlarmLevelHigh);
		BC_FAULT_RUNNING_PROPERTY;
		return;
	}
	
	double channelVoltageResolution=0;
	if ((inputVoltageResolution!= NULL) && ((*inputVoltageResolution) != 0) )
	    channelVoltageResolution=(*inputVoltageResolution);
	
	if (this->chanNum != -1)
	{
		//DPRINT( "checkTimeout now %f SET=%f, resolution=%f",readChannels[chanNum],this->setValue,channelVoltageResolution);
		if ( std::fabs(readChannels[chanNum] - this->setValue) <= channelVoltageResolution) 
		{
			char Num[8];
			sprintf(Num,"%d",chanNum);
			std::string chanName=(std::string)"CH"+Num;
			double* chan = getAttributeCache()->getRWPtr<double>(DOMAIN_INPUT, chanName);
			
			*chan=this->setValue;
			getAttributeCache()->setInputDomainAsChanged();
			//SCLDBG_ << "chan is" << chan << "value " <<*chan; 
			BC_END_RUNNING_PROPERTY;
		}
	}
	else
	{
		bool allOk=true;
		for (int i=0;i < (*this->numOfchannels);++i)
		{
			if ( std::abs(readChannels[i] - this->setValue) > channelVoltageResolution) 
			{
				char Num[8];
				sprintf(Num,"%d",i);
				std::string chanName=(std::string)"CH"+Num;
				double* chan = getAttributeCache()->getRWPtr<double>(DOMAIN_INPUT, chanName);
				*chan=this->setValue;
				getAttributeCache()->setInputDomainAsChanged();
				allOk=false;
			}
		}
		if (allOk==true)
		{
			BC_END_RUNNING_PROPERTY;
		}
	}

	
}
// empty timeout handler
bool own::CmdGIBsetChannelVoltage::timeoutHandler() {
	SCLDBG_ << "Timeout Handler setChannelVoltage ";
	
	metadataLogging(chaos::common::metadata_logging::StandardLoggingChannel::LogLevelError," setPoint not reached");
	setStateVariableSeverity(StateVariableTypeAlarmCU,"setPoint_not_reached",chaos::common::alarm::MultiSeverityAlarmLevelHigh);
	BC_FAULT_RUNNING_PROPERTY;
	return false;
}
