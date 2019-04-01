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
	/*
	chaos::cu::driver_manager::driver::DriverAccessor *hetcontroller_accessor = getAccessoInstanceByIndex(0);
	if (hetcontroller_accessor == NULL ) {
		throw chaos::CException(-1, "Cannot retrieve the requested driver", __FUNCTION__);
	}
	hetcontroller_drv = new chaos::driver::hetcontroller::ChaosHETControllerInterface(hetcontroller_accessor);
	if (hetcontroller_drv == NULL) {
		throw chaos::CException(-2, "Cannot allocate driver resources", __FUNCTION__);
	}
	*/
	addAttributeToDataSet("HV_MainUnit_Status",
							"The HV Status of the Main Unit",
							DataType::TYPE_INT32,
							DataType::Output);
	addAttributeToDataSet("HV_Main_Status_Description",
							"a string with the description of the main unit status",
							DataType::TYPE_STRING,
							DataType::Output, 256);

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
	
	addAttributeToDataSet("HVChannels",
							"The number of the channels of the HV Power Supply",
							DataType::TYPE_INT32,
							DataType::Output);

	addBinaryAttributeAsSubtypeToDataSet("HVChannelVoltages",
							"the values of the voltage channels",
							chaos::DataType::SUB_TYPE_DOUBLE,
							256*sizeof(double),
							chaos::DataType::Output);

	addBinaryAttributeAsSubtypeToDataSet("HVChannelAlarms",
							"the alarms  from the HV channels",
							chaos::DataType::SUB_TYPE_INT64,
							256*sizeof(int64_t),
							chaos::DataType::Output);

	addBinaryAttributeAsSubtypeToDataSet("HVChannelStatus",
							"the status  of the HV channels",
							chaos::DataType::SUB_TYPE_INT64,
							256*sizeof(int64_t),
							chaos::DataType::Output);



/*INPUT*/
	addAttributeToDataSet("driver_timeout",
							"custom user timeout in milliseconds for commands",
							DataType::TYPE_INT32,
							DataType::Input);
	addAttributeToDataSet("LVPositron_ControlUnit_CompleteName",
							"the name of the CU that controls the Low Voltages thresholds for positron side",
							DataType::TYPE_STRING,
							DataType::Input,256);
	addAttributeToDataSet("LVElectron_ControlUnit_CompleteName",
							"the name of the CU that controls the Low Voltages thresholds for electron side",
							DataType::TYPE_STRING,
							DataType::Input,256);
	addAttributeToDataSet("HVMPS_ControlUnit_CompleteName",
							"the name of the CU that controls the High Voltages Power Supply",
							DataType::TYPE_STRING,
							DataType::Input,256);

	
	addStateVariable(StateVariableTypeAlarmCU,"driver_command_error",
		"default driver communication error");
	
	addStateVariable(StateVariableTypeAlarmCU,"data_retrieving_error",
		"raised on communication error while retrieving data from  CUs");
	
	addStateVariable(StateVariableTypeAlarmDEV,"HET_CU_DEAD",
		"Some of the controlled Control Unit is dead");
	
	addStateVariable(StateVariableTypeAlarmDEV,"HET_CU_NOT_STARTED",
		"Some of the controlled Control Unit is not in start phase");
	
	addStateVariable(StateVariableTypeAlarmDEV,"LV_electron_driver_error",
		"The control unit for LV side Electrons have driver communication error");
	addStateVariable(StateVariableTypeAlarmDEV,"LV_positron_driver_error",
		"The control unit for LV side Positron have driver communication error");
	
	addStateVariable(StateVariableTypeAlarmDEV,"HV_powersupply_driver_error",
		"The control unit for HV crate have driver communication error");

	addStateVariable(StateVariableTypeAlarmDEV,"HV_channel_setPoint_not_reached",
		"The control unit for HV crate have failed reaching the setpoint");

	addStateVariable(StateVariableTypeAlarmDEV,"HV_channel_drifted_from_setPoint",
		"Raised when a HV Channel drifted from his setpoint");


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
default:
break;
}
usleep(500000);
} while (cmd_state->last_event != BatchCommandEventType::EVT_COMPLETED &&
        cmd_state->last_event != BatchCommandEventType::EVT_FAULT &&
    cmd_state->last_event != BatchCommandEventType::EVT_KILLED);
return (cmd_state.get() &&
cmd_state->last_event == BatchCommandEventType::EVT_COMPLETED);

}
