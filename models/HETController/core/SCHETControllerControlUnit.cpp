/*
SCHETControllerControlUnit.cpp
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
#include "SCHETControllerControlUnit.h"
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <chaos/cu_toolkit/windowsCompliant.h>
#include <common/debug/core/debug.h>
#include "CmdHCTDefault.h"
#include "CmdHCTSwitchHVPower.h"
#include "CmdHCTSetHVOnChannel.h"
#include "CmdHCTSetHighThreshold.h"
#include "CmdHCTSetLowThreshold.h"
#include "CmdHCTSwitchPulse.h"
using namespace chaos;
using namespace chaos::common::data;
using namespace chaos::common::batch_command;
using namespace chaos::cu::control_manager::slow_command;
using namespace chaos::cu::driver_manager::driver;
using namespace chaos::cu::control_manager;
#define SCCUAPP INFO_LOG(SCHETControllerControlUnit)
#define SCCUDBG DBG_LOG(SCHETControllerControlUnit)
#define SCCUERR ERR_LOG(SCHETControllerControlUnit)


PUBLISHABLE_CONTROL_UNIT_IMPLEMENTATION(::driver::hetcontroller::SCHETControllerControlUnit)

/*Construct a new CU with an identifier*/
::driver::hetcontroller::SCHETControllerControlUnit::SCHETControllerControlUnit(const string &_control_unit_id,
			const string &_control_unit_param,const ControlUnitDriverList &_control_unit_drivers)
:  chaos::cu::control_manager::SCAbstractControlUnit(_control_unit_id,
			 _control_unit_param, _control_unit_drivers) {
	hetcontroller_drv = NULL;
}
/*Base Destructor*/
::driver::hetcontroller::SCHETControllerControlUnit::~SCHETControllerControlUnit() {
	if (hetcontroller_drv) {
		delete (hetcontroller_drv);
	}
}
//handlers
//end handlers
void ::driver::hetcontroller::SCHETControllerControlUnit::unitDefineActionAndDataset()  {
	installCommand(BATCH_COMMAND_GET_DESCRIPTION(CmdHCTDefault),true);
	installCommand(BATCH_COMMAND_GET_DESCRIPTION(CmdHCTSwitchHVPower));
	installCommand(BATCH_COMMAND_GET_DESCRIPTION(CmdHCTSetHVOnChannel));
	installCommand(BATCH_COMMAND_GET_DESCRIPTION(CmdHCTSetHighThreshold));
	installCommand(BATCH_COMMAND_GET_DESCRIPTION(CmdHCTSetLowThreshold));
	installCommand(BATCH_COMMAND_GET_DESCRIPTION(CmdHCTSwitchPulse));
	chaos::cu::driver_manager::driver::DriverAccessor *hetcontroller_accessor = getAccessoInstanceByIndex(0);
	if (hetcontroller_accessor == NULL ) {
		throw chaos::CException(-1, "Cannot retrieve the requested driver", __FUNCTION__);
	}
	hetcontroller_drv = new chaos::driver::hetcontroller::ChaosHETControllerInterface(hetcontroller_accessor);
	if (hetcontroller_drv == NULL) {
		throw chaos::CException(-2, "Cannot allocate driver resources", __FUNCTION__);
	}
	addAttributeToDataSet("status_id",
							"default status attribute",
							DataType::TYPE_INT32,
							DataType::Output);
	addAttributeToDataSet("alarms",
							"default alarms attribute",
							DataType::TYPE_INT64,
							DataType::Output);
	addBinaryAttributeAsSubtypeToDataSet("ElectronsLowTHR",
							"the low thresholds of the electron side of the HET",
							chaos::DataType::SUB_TYPE_INT32,
							32*sizeof(int32_t),
							chaos::DataType::Output);
	addBinaryAttributeAsSubtypeToDataSet("ElectronsHighTHR",
							"the high thresholds of the electron side of the HET",
							chaos::DataType::SUB_TYPE_INT32,
							32*sizeof(int32_t),
							chaos::DataType::Output);
	addBinaryAttributeAsSubtypeToDataSet("PositronsHighTHR",
							"the high thresholds of the positron side of the HET",
							chaos::DataType::SUB_TYPE_INT32,
							32*sizeof(int32_t),
							chaos::DataType::Output);
	addBinaryAttributeAsSubtypeToDataSet("PositronsLowTHR",
							"the low thresholds of the positron side of the HET",
							chaos::DataType::SUB_TYPE_INT32,
							32*sizeof(int32_t),
							chaos::DataType::Output);
	addAttributeToDataSet("driver_timeout",
							"custom user timeout in milliseconds for commands",
							DataType::TYPE_INT32,
							DataType::Input);
	addStateVariable(StateVariableTypeAlarmCU,"driver_command_error",
		"default driver communication error");
}
void ::driver::hetcontroller::SCHETControllerControlUnit::unitDefineCustomAttribute() {
}
// Abstract method for the initialization of the control unit
void ::driver::hetcontroller::SCHETControllerControlUnit::unitInit() {
}
// Abstract method for the start of the control unit
void ::driver::hetcontroller::SCHETControllerControlUnit::unitStart() {
}
// Abstract method for the stop of the control unit
void ::driver::hetcontroller::SCHETControllerControlUnit::unitStop()  {
}
// Abstract method for deinit the control unit
void ::driver::hetcontroller::SCHETControllerControlUnit::unitDeinit() {
	SCCUAPP << "deinitializing ";
}
	//! restore the control unit to snapshot
#define RESTORE_LAPP SCCUAPP << "[RESTORE-" <<getCUID() << "] "
#define RESTORE_LERR SCCUERR << "[RESTORE-" <<getCUID() << "] "
bool ::driver::hetcontroller::SCHETControllerControlUnit::unitRestoreToSnapshot(chaos::cu::control_manager::AbstractSharedDomainCache *const snapshot_cache)  {
	return false;
}
bool ::driver::hetcontroller::SCHETControllerControlUnit::waitOnCommandID(uint64_t cmd_id) {
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
