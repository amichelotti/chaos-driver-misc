/*
CmdHCTDefault.cpp
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
#include "CmdHCTDefault.h"

#include <cmath>
#include  <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <sstream>
#define SCLAPP_ INFO_LOG(CmdHCTDefault) << "[" << getDeviceID() << "] "
#define SCLDBG_ DBG_LOG(CmdHCTDefault) << "[" << getDeviceID() << "] "
#define SCLERR_ ERR_LOG(CmdHCTDefault) << "[" << getDeviceID() << "] "
namespace own = driver::hetcontroller;
namespace c_data =  chaos::common::data;
namespace chaos_batch = chaos::common::batch_command;
using namespace chaos::cu::control_manager;
BATCH_COMMAND_OPEN_DESCRIPTION(driver::hetcontroller::,CmdHCTDefault,
	"Default command executed when no other commands in queue",
	"43150730-3521-48d3-aadb-547bf630226c")
BATCH_COMMAND_CLOSE_DESCRIPTION()


// return the implemented handler
uint8_t own::CmdHCTDefault::implementedHandler(){
	return      AbstractHETControllerCommand::implementedHandler()|chaos_batch::HandlerType::HT_Acquisition;
}
// empty set handler
void own::CmdHCTDefault::setHandler(c_data::CDataWrapper *data) {
	AbstractHETControllerCommand::setHandler(data);
	this->loggedNotStartedMsg=false;
	this->loggedDeadCUMsg=false;
	o_Positron_LTHR=getAttributeCache()->getRWPtr<int32_t>(DOMAIN_OUTPUT, "PositronsLowTHR");
	o_Positron_HTHR=getAttributeCache()->getRWPtr<int32_t>(DOMAIN_OUTPUT, "PositronsHighTHR");
	o_Electron_LTHR=getAttributeCache()->getRWPtr<int32_t>(DOMAIN_OUTPUT, "ElectronsLowTHR");
	o_Electron_HTHR=getAttributeCache()->getRWPtr<int32_t>(DOMAIN_OUTPUT, "ElectronsHighTHR");
	mainUnitStatusDescription=getAttributeCache()->getRWPtr<char>(DOMAIN_OUTPUT,"HV_Main_Status_Description");
	clearFeatures(chaos_batch::features::FeaturesFlagTypes::FF_SET_COMMAND_TIMEOUT);
	setBusyFlag(false);
	SCLAPP_ << "Set Handler Default "; 
	BC_NORMAL_RUNNING_PROPERTY
}
// empty acquire handler
void own::CmdHCTDefault::acquireHandler() {
	try 
	{
		std::pair<std::vector<int32_t>,std::vector<std::string>> retCuState=this->checkHealthState();
		bool raisedDeadAlarm=false;
		bool raiseNotStartAlarm=false;
		for (int i=0; i < retCuState.first.size();++i)
		{
			std::string cuIDName="";
			switch (i)
			{
				case 0 : cuIDName="LV_Positron";break;
				case 1 : cuIDName="LV_Electron";break;
				case 2 : cuIDName="HV_MPS";break;
				default: cuIDName="SomeError ";break;
			}
			if (retCuState.first[i] > DEFAULT_ELAPSED_TIME_HEALTH)
			{
				if (this->loggedDeadCUMsg==false)
				{
					metadataLogging(chaos::common::metadata_logging::StandardLoggingChannel::LogLevelError,"Control Unit "+ cuIDName +" Dead");
					this->loggedDeadCUMsg=true;
				}
				setStateVariableSeverity(StateVariableTypeAlarmDEV, "HET_CU_DEAD",chaos::common::alarm::MultiSeverityAlarmLevelHigh);
				raisedDeadAlarm=true;
			}
			if (retCuState.second[i] != "Start")
			{
				if (this->loggedNotStartedMsg==false)
				{
					metadataLogging(chaos::common::metadata_logging::StandardLoggingChannel::LogLevelError,"Control Unit "+cuIDName+" not started");
					this->loggedNotStartedMsg=true;
				}
				setStateVariableSeverity(StateVariableTypeAlarmDEV,"HET_CU_NOT_STARTED",chaos::common::alarm::MultiSeverityAlarmLevelHigh);
				raiseNotStartAlarm=true;
			}
		}
		if (raisedDeadAlarm==false) 
		{
			setStateVariableSeverity(StateVariableTypeAlarmDEV,"HET_CU_DEAD",chaos::common::alarm::MultiSeverityAlarmLevelClear);
			this->loggedDeadCUMsg=false;
		}
		if (raiseNotStartAlarm==false) 
		{
			setStateVariableSeverity(StateVariableTypeAlarmDEV,"HET_CU_NOT_STARTED",chaos::common::alarm::MultiSeverityAlarmLevelClear);
			this->loggedNotStartedMsg=false;
		}

		LVPDataset=LV_Positron->getLiveChannel(LVPName,0);
		LVEDataset=LV_Electron->getLiveChannel(LVEName,0);
		HVDataset=HVPS->getLiveChannel(MPSName,0);
		bool failed=false; //at the end, if failed is still false, lower the  data_retrieving error alarm
		if (LVPDataset == NULL)
		{
			SCLERR_ << "LV Positron Dataset null";
			metadataLogging(chaos::common::metadata_logging::StandardLoggingChannel::LogLevelError," cannot retrieve dataset for LV_Elecron");
			setStateVariableSeverity(StateVariableTypeAlarmCU,"data_retrieving_error",chaos::common::alarm::MultiSeverityAlarmLevelHigh);
			failed=true;
		}
		else
		{
			this->UpdateVoltagesFromDataset(LVPDataset, o_Positron_LTHR,o_Positron_HTHR);
		}
		if (LVEDataset == NULL)
		{
			SCLERR_ << "LV Electron Dataset null";
			metadataLogging(chaos::common::metadata_logging::StandardLoggingChannel::LogLevelError," cannot retrieve dataset for LV_Elecron");
			setStateVariableSeverity(StateVariableTypeAlarmCU,"data_retrieving_error",chaos::common::alarm::MultiSeverityAlarmLevelHigh);
			failed=true;
		}
		else
		{
			this->UpdateVoltagesFromDataset(LVEDataset, o_Electron_LTHR,o_Electron_HTHR);
		}
		if (HVDataset == NULL)
		{
			SCLERR_ << "HVDataset Dataset null";
			metadataLogging(chaos::common::metadata_logging::StandardLoggingChannel::LogLevelError," cannot retrieve dataset for HV Power Supply");
			setStateVariableSeverity(StateVariableTypeAlarmCU,"data_retrieving_error",chaos::common::alarm::MultiSeverityAlarmLevelHigh);
			failed=true;
		}
		else
		{
			UpdateMPSFromDataset(HVDataset);
		}
		




		
	}
	catch (int  e)
	{
		SCLDBG_ << "ALEDEBUG EXCEPTION CATCHED";
	}
	getAttributeCache()->setOutputDomainAsChanged();

}
// empty correlation handler
void own::CmdHCTDefault::ccHandler() {
}
// empty timeout handler
bool own::CmdHCTDefault::timeoutHandler() {
	SCLDBG_ << "Timeout Handler Default "; 
	return false;
}
bool own::CmdHCTDefault::UpdateMPSFromDataset(chaos::common::data::CDWShrdPtr fetched)
{
	*o_status_id=fetched->getInt32Value("status_id");
	
	std::string tmpDesc=fetched->getStringValue("Main_Status_Description");
	strncpy(mainUnitStatusDescription,tmpDesc.c_str(),tmpDesc.length());
	char* bindata;
	uint32_t size;

	bindata=(char*)fetched->getBinaryValue("ChannelStatus",size);
	for (int  i=0;((i < size/(sizeof(int32_t))) && (i<32)); i++)
	{
		//LTHR_dest[i]=(int32_t)bindata[i*(sizeof(int32_t))];
	}

}


bool own::CmdHCTDefault::UpdateVoltagesFromDataset(chaos::common::data::CDWShrdPtr fetched, int32_t* LTHR_dest,int32_t* HTHR_dest)
{
	//SCLDBG_ << "ALEDEBUG UPDATING VOLTAGES" << fetched->getCompliantJSONString();
	//int numOfChannels= fetched->getInt32Value("NumberOfChannels");
	//SCLDBG_ << "read number of channels " << numOfChannels;
	
	char* bindata;
	uint32_t size;
	bindata=(char*)fetched->getBinaryValue("ChannelLowThresholds",size);
	for (int  i=0;((i < size/(sizeof(int32_t))) && (i<32)); i++)
	{
		LTHR_dest[i]=(int32_t)bindata[i*(sizeof(int32_t))];
	}
	bindata=(char*)fetched->getBinaryValue("ChannelHighThresholds",size);
	for (int  i=0;((i < size/(sizeof(int32_t))) && (i<32)); i++)
	{
		HTHR_dest[i]=(int32_t)bindata[i*(sizeof(int32_t))];
	}
	return true;
}
