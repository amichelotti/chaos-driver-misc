/*
SCGibControlControlUnit.cpp
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
#include "SCGibControlControlUnit.h"
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <common/debug/core/debug.h>
#include "CmdGIBsetPulse.h"
#include "CmdGIBsetChannelVoltage.h"
#include "CmdGIBPowerOn.h"
#include "CmdGIBDefault.h"
using namespace chaos;
using namespace chaos::common::data;
using namespace chaos::common::batch_command;
using namespace chaos::cu::control_manager::slow_command;
using namespace chaos::cu::driver_manager::driver;
using namespace chaos::cu::control_manager;
#define SCCUAPP INFO_LOG(SCGibControlControlUnit)
#define SCCUDBG DBG_LOG(SCGibControlControlUnit)
#define SCCUERR ERR_LOG(SCGibControlControlUnit)


PUBLISHABLE_CONTROL_UNIT_IMPLEMENTATION(::driver::gibcontrol::SCGibControlControlUnit)

/*Construct a new CU with an identifier*/
::driver::gibcontrol::SCGibControlControlUnit::SCGibControlControlUnit(const string &_control_unit_id,
			const string &_control_unit_param,const ControlUnitDriverList &_control_unit_drivers)
:  chaos::cu::control_manager::SCAbstractControlUnit(_control_unit_id,
			 _control_unit_param, _control_unit_drivers) {
	gibcontrol_drv = NULL;
}
/*Base Destructor*/
::driver::gibcontrol::SCGibControlControlUnit::~SCGibControlControlUnit() {
	if (gibcontrol_drv) {
		delete (gibcontrol_drv);
	}
	
}
//handlers
bool ::driver::gibcontrol::SCGibControlControlUnit::myFunc(const std::string &name, double value, uint32_t size)
{
	std::string chanName=name.substr(2) ;
	int chan= atoi(chanName.c_str());
        SCCUAPP << "myFunc:"<< " VALUE: "<<value << "channel: " << chan;
        int  ret;
	uint64_t cmd_id;
        std::auto_ptr<CDataWrapper> cmd_pack(new CDataWrapper());
	cmd_pack->addInt32Value(CMD_GIB_SETCHANNELVOLTAGE_CHANNEL,chan);
	cmd_pack->addDoubleValue(CMD_GIB_SETCHANNELVOLTAGE_VOLTAGE,value);
	submitBatchCommand(CMD_GIB_SETCHANNELVOLTAGE_ALIAS,
        cmd_pack.release(),
        cmd_id,
        0,
        50,
        SubmissionRuleType::SUBMIT_NORMAL);
        return (ret==chaos::ErrorCode::EC_NO_ERROR);

        return true;
}
//end handlers
void ::driver::gibcontrol::SCGibControlControlUnit::unitDefineActionAndDataset()  {
	installCommand(BATCH_COMMAND_GET_DESCRIPTION(CmdGIBsetPulse));
	installCommand(BATCH_COMMAND_GET_DESCRIPTION(CmdGIBsetChannelVoltage));
	installCommand(BATCH_COMMAND_GET_DESCRIPTION(CmdGIBPowerOn));
	installCommand(BATCH_COMMAND_GET_DESCRIPTION(CmdGIBDefault),true);
	

	chaos::cu::driver_manager::driver::DriverAccessor *gibcontrol_accessor = getAccessoInstanceByIndex(0);
	if (gibcontrol_accessor == NULL ) {
		throw chaos::CException(-1, "Cannot retrieve the requested driver", __FUNCTION__);
	}
	gibcontrol_drv = new chaos::driver::gibcontrol::ChaosGibControlInterface(gibcontrol_accessor);
	if (gibcontrol_drv == NULL) {
		throw chaos::CException(-2, "Cannot allocate driver resources", __FUNCTION__);
	}
	this->numofchannels=0;
	int32_t err;
	err=gibcontrol_drv->getNumOfChannels(&numofchannels);
	if (err != 0)
	{
		throw chaos::CException(-2, "Driver doesn't answered to getNumOfChannels", __FUNCTION__);
	}
	SCCUAPP << "Obtained number of channels " << numofchannels;

	addAttributeToDataSet("numberOfChannels",
							"number of channel of current GIB",
							DataType::TYPE_INT32,
							DataType::Output);

	addAttributeToDataSet("status",
							"status",
							DataType::TYPE_STRING,
							DataType::Output, 256);
	addAttributeToDataSet("status_id",
							"status_id",
							DataType::TYPE_INT32,
							DataType::Output);
	addAttributeToDataSet("alarms",
							"alarms",
							DataType::TYPE_INT64,
							DataType::Output);
	addAttributeToDataSet("driver_timeout",
							"Driver timeout in milliseconds",
							DataType::TYPE_INT32,
							DataType::Input);

	addAttributeToDataSet("max_channel_voltage",
							"The max value of voltage to be set on each channel",
							DataType::TYPE_DOUBLE,
							DataType::Input);

	addAttributeToDataSet("min_channel_voltage",
							"The min value of voltage to be set on each channel",
							DataType::TYPE_DOUBLE,
							DataType::Input);

	addAttributeToDataSet("voltage_channel_resolution",
							"The resolution on setting voltage channel and the tolerated drift from the setpoint",
							DataType::TYPE_DOUBLE,
							DataType::Input);


	addAttributeToDataSet("HVMain",
							"HV Main Voltage",
							DataType::TYPE_DOUBLE,
							DataType::Output);
	addAttributeToDataSet("Supply5V",
							"Readout +5V Supply",
							DataType::TYPE_DOUBLE,
							DataType::Output);
	addAttributeToDataSet("SupplyN5V",
							"Readout -5V Supply",
							DataType::TYPE_DOUBLE,
							DataType::Output);
	addBinaryAttributeAsSubtypeToDataSet("PulsingAmplitudes",
							"The Amplitudes of Pulsing channels",
							chaos::DataType::SUB_TYPE_INT32,
							numofchannels*sizeof(int32_t),
							chaos::DataType::Output);
	addBinaryAttributeAsSubtypeToDataSet("PulsingWidth",
							"The Width of Pulsing channels",
							chaos::DataType::SUB_TYPE_INT32,
							numofchannels*sizeof(int32_t),
							chaos::DataType::Output);
		
	for (int i=0; i < numofchannels; ++i)
	{
	   char nums[8];
       sprintf(nums,"%d",i);
	   std::string chanName=(std::string)"CH"+ nums;
	   addAttributeToDataSet(chanName,
							"voltage channel",
							DataType::TYPE_DOUBLE,
							DataType::Bidirectional);

	   addHandlerOnInputAttributeName< ::driver::gibcontrol::SCGibControlControlUnit,double>(this,
		&::driver::gibcontrol::SCGibControlControlUnit::myFunc,chanName) ;
	}
	addStateVariable(StateVariableTypeAlarmCU,"driver_command_error",
		"notified when driver answers not zero");
	addStateVariable(StateVariableTypeAlarmCU,"gib_unreachable",
		"notified when gib is not reachable");	
	addStateVariable(StateVariableTypeAlarmCU,"bad_command_parameter",
		"notified when a command is issued with wrong parameters");
	addStateVariable(StateVariableTypeAlarmCU,"setPoint_not_reached",
		"notified when a channel voltage setPoint is not reached after a set Voltage command");

}
void ::driver::gibcontrol::SCGibControlControlUnit::unitDefineCustomAttribute() {
}
// Abstract method for the initialization of the control unit
void ::driver::gibcontrol::SCGibControlControlUnit::unitInit()  {
	int32_t* chanNum;
	chanNum = getAttributeCache()->getRWPtr<int32_t>(DOMAIN_OUTPUT, "numberOfChannels"); 
	*chanNum=numofchannels;
	
}
// Abstract method for the start of the control unit
void ::driver::gibcontrol::SCGibControlControlUnit::unitStart()  {
}
// Abstract method for the stop of the control unit
void ::driver::gibcontrol::SCGibControlControlUnit::unitStop()  {
}
// Abstract method for deinit the control unit
void ::driver::gibcontrol::SCGibControlControlUnit::unitDeinit() {
	SCCUAPP << "deinitializing ";
	gibcontrol_drv->deinit();
}
	//! restore the control unit to snapshot
#define RESTORE_LAPP SCCUAPP << "[RESTORE-" <<getCUID() << "] "
#define RESTORE_LERR SCCUERR << "[RESTORE-" <<getCUID() << "] "
#define RESTORE_LDBG SCCUDBG << "[RESTORE-" << getCUID() << "] "
bool ::driver::gibcontrol::SCGibControlControlUnit::unitRestoreToSnapshot(chaos::cu::control_manager::AbstractSharedDomainCache *const snapshot_cache)  {
	uint64_t start_restore_time= chaos::common::utility::TimingUtil::getTimeStamp();
	try
	{
		if (snapshot_cache == NULL)
		{
			RESTORE_LERR << "cache nulla";
			return false;
		}
		//check if in the restore cache we have all information we need
		if (snapshot_cache->getSharedDomain(DOMAIN_INPUT).hasAttribute("voltage_channel_resolution"))
		{
			double restore_channel_resolution = *snapshot_cache->getAttributeValue(DOMAIN_INPUT, "voltage_channel_resolution")->getValuePtr<double>();
			double* chanRes = getAttributeCache()->getRWPtr<double>(DOMAIN_INPUT, "voltage_channel_resolution");
			if (chanRes != NULL)
			{
				RESTORE_LDBG << "Restoring channel resolution";
			    *chanRes=restore_channel_resolution;
				getAttributeCache()->setInputDomainAsChanged();
			}
			{
				RESTORE_LDBG << "NOT Restoring channel resolution because of null";
			}
		}
		RESTORE_LDBG << "Restore Check if  cache for status_id";
		if (!snapshot_cache->getSharedDomain(DOMAIN_OUTPUT).hasAttribute("status_id"))
		{
			RESTORE_LERR << " missing status_id to restore";
			return false;
		}
		int32_t restore_power_sp = *snapshot_cache->getAttributeValue(DOMAIN_OUTPUT, "status_id")->getValuePtr<int32_t>();
		RESTORE_LDBG << "Restore in cache status is " << restore_power_sp;
		restore_power_sp=(int32_t)((restore_power_sp & ::common::gibcontrol::GIBCONTROL_SUPPLIED) != 0);
    	RESTORE_LDBG << "Restore Trying to set power at " << restore_power_sp;
		if (!setPowerOn(restore_power_sp))
		{
			metadataLogging(chaos::common::metadata_logging::StandardLoggingChannel::LogLevelError, CHAOS_FORMAT("Error applying power on during restore \"%1%\"  to power %2% ", % getDeviceID()  % restore_power_sp));
			return false;
		}
    	sleep(1);
		int nums;
		gibcontrol_drv->getNumOfChannels(&nums);
		if (restore_power_sp==1)
		{
			for (int i=0;i < nums; ++i)
			{
				char chVal[8];
       			sprintf(chVal,"%d",i);
				std::string chanToRestore=(std::string)"CH"+ chVal ;
				if (!snapshot_cache->getSharedDomain(DOMAIN_OUTPUT).hasAttribute(chanToRestore))
				{
					RESTORE_LERR << " missing "<<chanToRestore <<" to restore";
					return false;
				}
				else
				{
					double restore_channel_value= *snapshot_cache->getAttributeValue(DOMAIN_OUTPUT, chanToRestore)->getValuePtr<double>();
					int ok=myFunc(chanToRestore,restore_channel_value,5);
					if (!ok)
					{
						RESTORE_LERR << " Failed to restore channel "<<chanToRestore << "restore Abort";
						return false;
					}
				}
				//sleep(1);//

			}
		}
		return true;
	}
	 catch (CException &ex)
	{
		uint64_t restore_duration_in_ms = chaos::common::utility::TimingUtil::getTimeStamp() - start_restore_time;
		RESTORE_LERR << "[metric] Restore has fault in " << restore_duration_in_ms << " milliseconds";
		throw ex;
	}



	return false;
}
bool ::driver::gibcontrol::SCGibControlControlUnit::waitOnCommandID(uint64_t cmd_id) 
{
 ChaosUniquePtr<chaos::common::batch_command::CommandState> cmd_state;
do 
{ 
	cmd_state = getStateForCommandID(cmd_id);
	if (!cmd_state.get()) break;
	switch (cmd_state->last_event) 
	{
		case BatchCommandEventType::EVT_QUEUED:
		SCCUAPP << cmd_id << " -> QUEUED";
		break;
		case BatchCommandEventType::EVT_RUNNING:
		SCCUAPP << cmd_id << " -> RUNNING"; 
		break;
		case BatchCommandEventType::EVT_WAITING:
		SCCUAPP << cmd_id << " -> WAITING";
		break;
		case BatchCommandEventType::EVT_PAUSED:
		SCCUAPP << cmd_id << " -> PAUSED";
		break;
		case BatchCommandEventType::EVT_KILLED:
		SCCUAPP << cmd_id << " -> KILLED";
		break;
		case BatchCommandEventType::EVT_COMPLETED:
		SCCUAPP << cmd_id << " -> COMPLETED";
		break;
		case BatchCommandEventType::EVT_FAULT:
			SCCUAPP << cmd_id << " -> FAULT";
		break;
	}
	usleep(500000);
} 
while (cmd_state->last_event != BatchCommandEventType::EVT_COMPLETED &&  cmd_state->last_event != BatchCommandEventType::EVT_FAULT &&     cmd_state->last_event != BatchCommandEventType::EVT_KILLED);
return (cmd_state.get() && cmd_state->last_event == BatchCommandEventType::EVT_COMPLETED);
}
bool ::driver::gibcontrol::SCGibControlControlUnit::setPowerOn(int32_t value, bool sync)
{
  uint64_t cmd_id;
  bool result = true;

  SCCUAPP << "LAUNCHING BATCH COMMAND cmdGIBPowerOn " << value;
  ChaosUniquePtr<CDataWrapper> cmd_pack(new CDataWrapper());
  cmd_pack->addInt32Value(CMD_GIB_POWERON_ON_STATE, value);
  //send command
  submitBatchCommand(CMD_GIB_POWERON_ALIAS,
                     cmd_pack.release(),
                     cmd_id,
                     0,
                     50,
                     SubmissionRuleType::SUBMIT_AND_STACK);
  if (sync)
  {
    result = waitOnCommandID(cmd_id);
  }
  return result;
}