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
#include "CmdGIBDefault.h"
using namespace chaos;
using namespace chaos::common::data;
using namespace chaos::common::batch_command;
using namespace chaos::cu::control_manager::slow_command;
using namespace chaos::cu::driver_manager::driver;
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
//end handlers
void ::driver::gibcontrol::SCGibControlControlUnit::unitDefineActionAndDataset() throw(chaos::CException) {
	installCommand(BATCH_COMMAND_GET_DESCRIPTION(CmdGIBsetPulse));
	installCommand(BATCH_COMMAND_GET_DESCRIPTION(CmdGIBsetChannelVoltage));
	installCommand(BATCH_COMMAND_GET_DESCRIPTION(CmdGIBDefault),true);
	addAttributeToDataSet("gastoneLV",
							"Low Voltage status",
							DataType::TYPE_INT32,
							DataType::Output);
	addAttributeToDataSet("pulsing",
							"pulse status",
							DataType::TYPE_BOOLEAN,
							DataType::Output);
	addAttributeToDataSet("pulse_width",
							"pulse width (ns)",
							DataType::TYPE_INT32,
							DataType::Output);
	addAttributeToDataSet("pulse_amplitude",
							"pulse amplitude (V)",
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
}
void ::driver::gibcontrol::SCGibControlControlUnit::unitDefineCustomAttribute() {
}
// Abstract method for the initialization of the control unit
void ::driver::gibcontrol::SCGibControlControlUnit::unitInit() throw(CException) {
	chaos::cu::driver_manager::driver::DriverAccessor *gibcontrol_accessor = getAccessoInstanceByIndex(0);
	
	/*
	if (gibcontrol_accessor == NULL ) {
		throw chaos::CException(-1, "Cannot retrieve the requested driver", __FUNCTION__);
	}
	gibcontrol_drv = new chaos::driver::gibcontrol::ChaosGibControlInterface(gibcontrol_accessor);
	if (gibcontrol_drv == NULL) {
		throw chaos::CException(-2, "Cannot allocate driver resources", __FUNCTION__);
	}*/
	SCCUAPP << "ALEDEBUG INIT DONE";
}
// Abstract method for the start of the control unit
void ::driver::gibcontrol::SCGibControlControlUnit::unitStart() throw(CException) {
	SCCUAPP << "ALEDEBUG START DONE";
}
// Abstract method for the stop of the control unit
void ::driver::gibcontrol::SCGibControlControlUnit::unitStop() throw(CException) {
	SCCUAPP << "ALEDEBUG STOP DONE";
}
// Abstract method for deinit the control unit
void ::driver::gibcontrol::SCGibControlControlUnit::unitDeinit() throw(CException) {
	SCCUAPP << "deinitializing ";
	gibcontrol_drv->deinit();
}
	//! restore the control unit to snapshot
#define RESTORE_LAPP SCCUAPP << "[RESTORE-" <<getCUID() << "] "
#define RESTORE_LERR SCCUERR << "[RESTORE-" <<getCUID() << "] "
bool ::driver::gibcontrol::SCGibControlControlUnit::unitRestoreToSnapshot(chaos::cu::control_manager::AbstractSharedDomainCache *const snapshot_cache) throw(chaos::CException) {
	return false;
}
bool ::driver::gibcontrol::SCGibControlControlUnit::waitOnCommandID(uint64_t cmd_id) {
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
