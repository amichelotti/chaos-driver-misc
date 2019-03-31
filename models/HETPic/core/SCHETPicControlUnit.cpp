/*
SCHETPicControlUnit.cpp
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
#include "SCHETPicControlUnit.h"
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <common/debug/core/debug.h>
#include "CmdHPCDefault.h"
#include "CmdHPCSetHighThreshold.h"
#include "CmdHPCSetLowThreshold.h"
#include "CmdHPCsetPulse.h"
using namespace chaos;
using namespace chaos::common::data;
using namespace chaos::common::batch_command;
using namespace chaos::cu::control_manager::slow_command;
using namespace chaos::cu::driver_manager::driver;
using namespace chaos::cu::control_manager;
#define SCCUAPP INFO_LOG(SCHETPicControlUnit)
#define SCCUDBG DBG_LOG(SCHETPicControlUnit)
#define SCCUERR ERR_LOG(SCHETPicControlUnit)


PUBLISHABLE_CONTROL_UNIT_IMPLEMENTATION(::driver::hetpic::SCHETPicControlUnit)

/*Construct a new CU with an identifier*/
::driver::hetpic::SCHETPicControlUnit::SCHETPicControlUnit(const string &_control_unit_id,
			const string &_control_unit_param,const ControlUnitDriverList &_control_unit_drivers)
:  chaos::cu::control_manager::SCAbstractControlUnit(_control_unit_id,
			 _control_unit_param, _control_unit_drivers) {
	hetpic_drv = NULL;
}
/*Base Destructor*/
::driver::hetpic::SCHETPicControlUnit::~SCHETPicControlUnit() {
	if (hetpic_drv) {
		delete (hetpic_drv);
	}
}
//handlers
//end handlers
void ::driver::hetpic::SCHETPicControlUnit::unitDefineActionAndDataset()  {
	installCommand(BATCH_COMMAND_GET_DESCRIPTION(CmdHPCDefault),true);
	installCommand(BATCH_COMMAND_GET_DESCRIPTION(CmdHPCSetHighThreshold));
	installCommand(BATCH_COMMAND_GET_DESCRIPTION(CmdHPCSetLowThreshold));
	installCommand(BATCH_COMMAND_GET_DESCRIPTION(CmdHPCsetPulse));
	chaos::cu::driver_manager::driver::DriverAccessor *hetpic_accessor = getAccessoInstanceByIndex(0);
	if (hetpic_accessor == NULL ) {
		throw chaos::CException(-1, "Cannot retrieve the requested driver", __FUNCTION__);
	}
	hetpic_drv = new chaos::driver::hetpic::ChaosHETPicInterface(hetpic_accessor);
	if (hetpic_drv == NULL) {
		throw chaos::CException(-2, "Cannot allocate driver resources", __FUNCTION__);
	}
	int32_t chans;
	SCCUAPP << "Reading number of channels" ;
	if (hetpic_drv->getNumberOfChannel(chans) != 0)
	{
		throw chaos::CException(-3, "Cannot receive number of channels info from driver", __FUNCTION__);
	}
	this->numberOfChannels=chans;





	addAttributeToDataSet("status_id",
							"default status attribute",
							DataType::TYPE_INT32,
							DataType::Output);
	addAttributeToDataSet("alarms",
							"default alarms attribute",
							DataType::TYPE_INT64,
							DataType::Output);
	addAttributeToDataSet("driver_timeout",
							"the custom driver timeout (in ms)",
							DataType::TYPE_INT32,
							DataType::Input);
	addAttributeToDataSet("NumberOfChannels",
							"returns the number of channels driven by this PIC",
							DataType::TYPE_INT32,
							DataType::Output);
	addBinaryAttributeAsSubtypeToDataSet("ChannelHighThresholds",
							"the high thresholds current value of each channel",
							chaos::DataType::SUB_TYPE_INT32,
							chans*sizeof(int32_t),
							chaos::DataType::Output);
	addBinaryAttributeAsSubtypeToDataSet("ChannelLowThresholds",
							"the high thresholds current value of each channel",
							chaos::DataType::SUB_TYPE_INT32,
							chans*sizeof(int32_t),
							chaos::DataType::Output);
	addStateVariable(StateVariableTypeAlarmCU,"driver_command_error",
		"default driver communication error");
}
void ::driver::hetpic::SCHETPicControlUnit::unitDefineCustomAttribute() {
}
// Abstract method for the initialization of the control unit
void ::driver::hetpic::SCHETPicControlUnit::unitInit() {
	int32_t * numChanPt=getAttributeCache()->getRWPtr<int32_t>(DOMAIN_OUTPUT,"NumberOfChannels");
	*numChanPt=this->numberOfChannels;
}
// Abstract method for the start of the control unit
void ::driver::hetpic::SCHETPicControlUnit::unitStart() {
}
// Abstract method for the stop of the control unit
void ::driver::hetpic::SCHETPicControlUnit::unitStop()  {
}
// Abstract method for deinit the control unit
void ::driver::hetpic::SCHETPicControlUnit::unitDeinit() {
	SCCUAPP << "deinitializing ";
}
	//! restore the control unit to snapshot
#define RESTORE_LAPP SCCUAPP << "[RESTORE-" <<getCUID() << "] "
#define RESTORE_LERR SCCUERR << "[RESTORE-" <<getCUID() << "] "
bool ::driver::hetpic::SCHETPicControlUnit::unitRestoreToSnapshot(chaos::cu::control_manager::AbstractSharedDomainCache *const snapshot_cache)  {
	return false;
}
bool ::driver::hetpic::SCHETPicControlUnit::waitOnCommandID(uint64_t cmd_id) {
	 ChaosUniquePtr<chaos::common::batch_command::CommandState> cmd_state;
do { 
cmd_state = getStateForCommandID(cmd_id);
if (!cmd_state.get()) break;
switch (cmd_state->last_event) {
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
} while (cmd_state->last_event != BatchCommandEventType::EVT_COMPLETED &&
        cmd_state->last_event != BatchCommandEventType::EVT_FAULT &&
    cmd_state->last_event != BatchCommandEventType::EVT_KILLED);
return (cmd_state.get() &&
cmd_state->last_event == BatchCommandEventType::EVT_COMPLETED);

}
