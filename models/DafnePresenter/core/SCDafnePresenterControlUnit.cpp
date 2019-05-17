/*
SCDafnePresenterControlUnit.cpp
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
#include "SCDafnePresenterControlUnit.h"
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <common/debug/core/debug.h>
#include <chaos/cu_toolkit/windowsCompliant.h>
#include "CmdDafDefault.h"
#include <json/json.h>
using namespace chaos;
using namespace chaos::common::data;
using namespace chaos::common::batch_command;
using namespace chaos::cu::control_manager::slow_command;
using namespace chaos::cu::driver_manager::driver;
using namespace chaos::cu::control_manager;
#define SCCUAPP INFO_LOG(SCDafnePresenterControlUnit)
#define SCCUDBG DBG_LOG(SCDafnePresenterControlUnit)
#define SCCUERR ERR_LOG(SCDafnePresenterControlUnit)


PUBLISHABLE_CONTROL_UNIT_IMPLEMENTATION(::driver::dafnepresenter::SCDafnePresenterControlUnit)

/*Construct a new CU with an identifier*/
::driver::dafnepresenter::SCDafnePresenterControlUnit::SCDafnePresenterControlUnit(const string &_control_unit_id,
			const string &_control_unit_param,const ControlUnitDriverList &_control_unit_drivers)
:  chaos::cu::control_manager::SCAbstractControlUnit(_control_unit_id,
			 _control_unit_param, _control_unit_drivers) {
	dafnepresenter_drv = NULL;
	SCCUAPP << "ALEDEBUG Constructing CU with load parameter " << _control_unit_param ;
	Json::Value json_parameter;
  	Json::Reader json_reader;
	this->loadedNewDafnePath="";
	this->loadedFastPath="";
	  if (!json_reader.parse(_control_unit_param, json_parameter))
	{
		SCCUERR << "Bad Json parameter " << json_parameter <<" INPUT " << _control_unit_param;
		
	
	}
	else
	{
		try
		{
			std::string dafnefilePath= json_parameter["dafneFilePath"].asString();
			SCCUAPP << "ALEDEBUG path for reading dafne data: " << dafnefilePath ;
			this->loadedNewDafnePath=dafnefilePath;
			std::string outFile= json_parameter["outFile"].asString();
			this->loadedOutFile=outFile;
			std::string fastfilePath= json_parameter["fastFilePath"].asString();
			this->loadedFastPath=fastfilePath;
		}
		catch(...)
		{
			throw chaos::CException(-1, "Bad CU Configuration. Cannot retrieve dafne files path", __FUNCTION__);
		}

	}
}
/*Base Destructor*/
::driver::dafnepresenter::SCDafnePresenterControlUnit::~SCDafnePresenterControlUnit() {
	if (dafnepresenter_drv) {
		delete (dafnepresenter_drv);
	}
}
//handlers
//end handlers
void ::driver::dafnepresenter::SCDafnePresenterControlUnit::unitDefineActionAndDataset()  {
	installCommand(BATCH_COMMAND_GET_DESCRIPTION(CmdDafDefault),true);
	/* //Uncomment when you want to connect the driver
	chaos::cu::driver_manager::driver::DriverAccessor *dafnepresenter_accessor = getAccessoInstanceByIndex(0);
	if (dafnepresenter_accessor == NULL ) {
		throw chaos::CException(-1, "Cannot retrieve the requested driver", __FUNCTION__);
	}
	dafnepresenter_drv = new chaos::driver::dafnepresenter::ChaosDafnePresenterInterface(dafnepresenter_accessor);
	if (dafnepresenter_drv == NULL) {
		throw chaos::CException(-2, "Cannot allocate driver resources", __FUNCTION__);
	}
	*/ //Uncomment when you want to connect the driver
	
	addAttributeToDataSet("timestamp",
							"timestamp of data",
							DataType::TYPE_INT64,
							DataType::Output);
	addAttributeToDataSet("dafne_status",
							"Dafne Status",
							DataType::TYPE_INT32,
							DataType::Output);
	addAttributeToDataSet("i_ele",
							"Corrente Elettroni (mA)",
							DataType::TYPE_DOUBLE,
							DataType::Output);
	addAttributeToDataSet("i_pos",
							"Corrente Positroni (mA)",
							DataType::TYPE_DOUBLE,
							DataType::Output);
	addAttributeToDataSet("nbunch_ele",
							"numero di bunch elettroni",
							DataType::TYPE_INT32,
							DataType::Output);
	addAttributeToDataSet("nbunch_pos",
							"numero di bunch elettroni",
							DataType::TYPE_INT32,
							DataType::Output);
	addAttributeToDataSet("fill_pattern_ele",
							"fill pattern elettroni",
							DataType::TYPE_INT32,
							DataType::Output);
	addAttributeToDataSet("fill_pattern_pos",
							"fill pattern positroni",
							DataType::TYPE_INT32,
							DataType::Output);
	addAttributeToDataSet("lifetime_ele",
							"lifetime elettroni",
							DataType::TYPE_INT32,
							DataType::Output);
	addAttributeToDataSet("lifetime_pos",
							"lifetime positroni",
							DataType::TYPE_INT32,
							DataType::Output);
	addAttributeToDataSet("sx_ele",
							"Sigma X Elettroni (um)",
							DataType::TYPE_DOUBLE,
							DataType::Output);
	addAttributeToDataSet("sy_ele",
							"Sigma Y Elettroni (um)",
							DataType::TYPE_DOUBLE,
							DataType::Output);
	addAttributeToDataSet("th_ele",
							"angolo di tilt elettroni (rad)",
							DataType::TYPE_DOUBLE,
							DataType::Output);
	addAttributeToDataSet("sx_pos",
							"Sigma X positroni (um)",
							DataType::TYPE_DOUBLE,
							DataType::Output);
	addAttributeToDataSet("sy_pos",
							"Sigma Y positroni (um)",
							DataType::TYPE_DOUBLE,
							DataType::Output);
	addAttributeToDataSet("th_pos",
							"Angolo di tilt positroni (rad)",
							DataType::TYPE_DOUBLE,
							DataType::Output);
	addAttributeToDataSet("rf",
							"DAFNE radiofrequency (Hz)",
							DataType::TYPE_DOUBLE,
							DataType::Output);
	addAttributeToDataSet("VUGEL101",
							"Vacuometro elettroni Long1 num1 (torr)",
							DataType::TYPE_DOUBLE,
							DataType::Output);
	addAttributeToDataSet("VUGEL102",
							"Vacuometro elettroni Long1 num2 (torr)",
							DataType::TYPE_DOUBLE,
							DataType::Output);
	addAttributeToDataSet("VUGEL103",
							"Vacuometro elettroni Long1 num3 (torr)",
							DataType::TYPE_DOUBLE,
							DataType::Output);
	addAttributeToDataSet("VUGEL201",
							"Vacuometro elettroni Long2 num1 (torr)",
							DataType::TYPE_DOUBLE,
							DataType::Output);
	addAttributeToDataSet("VUGEL202",
							"Vacuometro elettroni Long2 num2 (torr)",
							DataType::TYPE_DOUBLE,
							DataType::Output);
	addAttributeToDataSet("VUGEL203",
							"Vacuometro elettroni Long2 num3 (torr)",
							DataType::TYPE_DOUBLE,
							DataType::Output);
	addAttributeToDataSet("VUGES101",
							"Vacuometro elettroni Short1 num1 (torr)",
							DataType::TYPE_DOUBLE,
							DataType::Output);
	addAttributeToDataSet("VUGES102",
							"Vacuometro elettroni Short1 num2 (torr)",
							DataType::TYPE_DOUBLE,
							DataType::Output);
	addAttributeToDataSet("VUGES103",
							"Vacuometro elettroni Short1 num3 (torr)",
							DataType::TYPE_DOUBLE,
							DataType::Output);
	addAttributeToDataSet("VUGES201",
							"Vacuometro elettroni Short2 num1 (torr)",
							DataType::TYPE_DOUBLE,
							DataType::Output);
	addAttributeToDataSet("VUGES202",
							"Vacuometro elettroni Short2 num2 (torr)",
							DataType::TYPE_DOUBLE,
							DataType::Output);
	addAttributeToDataSet("VUGES203",
							"Vacuometro elettroni Short2 num3 (torr)",
							DataType::TYPE_DOUBLE,
							DataType::Output);
	addAttributeToDataSet("VUGPL101",
							"Vacuometro positroni Long1 num1 (torr)",
							DataType::TYPE_DOUBLE,
							DataType::Output);
	addAttributeToDataSet("VUGPL102",
							"Vacuometro positroni Long1 num2 (torr)",
							DataType::TYPE_DOUBLE,
							DataType::Output);
	addAttributeToDataSet("VUGPL103",
							"Vacuometro positroni Long1 num3 (torr)",
							DataType::TYPE_DOUBLE,
							DataType::Output);
	addAttributeToDataSet("VUGPL201",
							"Vacuometro positroni Long2 num1 (torr)",
							DataType::TYPE_DOUBLE,
							DataType::Output);
	addAttributeToDataSet("VUGPL202",
							"Vacuometro positroni Long2 num2 (torr)",
							DataType::TYPE_DOUBLE,
							DataType::Output);
	addAttributeToDataSet("VUGPL203",
							"Vacuometro positroni Long2 num3 (torr)",
							DataType::TYPE_DOUBLE,
							DataType::Output);
	addAttributeToDataSet("VUGPS101",
							"Vacuometro positroni Short1 num1 (torr)",
							DataType::TYPE_DOUBLE,
							DataType::Output);
	addAttributeToDataSet("VUGPS102",
							"Vacuometro positroni Short1 num2 (torr)",
							DataType::TYPE_DOUBLE,
							DataType::Output);
	addAttributeToDataSet("VUGPS103",
							"Vacuometro positroni Short1 num3 (torr)",
							DataType::TYPE_DOUBLE,
							DataType::Output);
	addAttributeToDataSet("VUGPS201",
							"Vacuometro positroni Short2 num1 (torr)",
							DataType::TYPE_DOUBLE,
							DataType::Output);
	addAttributeToDataSet("VUGPS202",
							"Vacuometro positroni Short2 num2 (torr)",
							DataType::TYPE_DOUBLE,
							DataType::Output);
	addAttributeToDataSet("VUGPS203",
							"Vacuometro positroni Short2 num3 (torr)",
							DataType::TYPE_DOUBLE,
							DataType::Output);
	addAttributeToDataSet("ty_ele",
							"Temperatura camera Y elettroni (Celsius)",
							DataType::TYPE_DOUBLE,
							DataType::Output);
	addAttributeToDataSet("ty_pos",
							"Temperatura camera Y positroni (Celsius)",
							DataType::TYPE_DOUBLE,
							DataType::Output);
	addAttributeToDataSet("R2_CCAL",
							"Rate totale di coincidenze misurata dal CCALT (Hz)",
							DataType::TYPE_DOUBLE,
							DataType::Output);
	addAttributeToDataSet("R2_BKG",
							"Rate totale di coincidenze di fondo  misurata dal CCALT (Hz)",
							DataType::TYPE_DOUBLE,
							DataType::Output);
	addAttributeToDataSet("Dead_TC",
							"Frazione di tempo morto del DAQ CCALT",
							DataType::TYPE_DOUBLE,
							DataType::Output);
	addAttributeToDataSet("R1C_ele",
							"Rate di conteggi sul lato illuminato dagli elettroni (Hz)",
							DataType::TYPE_DOUBLE,
							DataType::Output);
	addAttributeToDataSet("R1C_pos",
							"Rate di conteggi sul lato illuminato dai positroni (Hz)",
							DataType::TYPE_DOUBLE,
							DataType::Output);
	addAttributeToDataSet("lum_CCAL",
							"Luminosità (10^30 cm-2 s-1)",
							DataType::TYPE_DOUBLE,
							DataType::Output);
	
	addAttributeToDataSet("status_id",
							"default status attribute",
							DataType::TYPE_INT32,
							DataType::Output);
	addAttributeToDataSet("alarms",
							"default alarms attribute",
							DataType::TYPE_INT64,
							DataType::Output);
	
	
	
	
	addAttributeToDataSet("driver_timeout",
							"custom user timeout in milliseconds for commands",
							DataType::TYPE_INT32,
							DataType::Input);


	addAttributeToDataSet("printFile",
							"0: no print; 1: print as JsonSimple; 2 print as JsonComplete; 3 print as Raw TXT",
							DataType::TYPE_INT32,
							DataType::Input);
	addStateVariable(StateVariableTypeAlarmCU,"driver_command_error",
		"default driver communication error");
	addStateVariable(StateVariableTypeAlarmCU,"dafne_file_not_found",
		"raised when file with dafne data cannot be found");
	addStateVariable(StateVariableTypeAlarmCU,"dafne_file_incorrect",
		"raised when file with dafne data has format not recognized");
	addStateVariable(StateVariableTypeAlarmCU,"CCALT_data_not_retrieved",
		"raised when fails to retrieve CCALT data");
		
	addStateVariable(StateVariableTypeAlarmCU,"failed_to_write_output_file",
		"raised when fails to write output file");
	
	addStateVariable(StateVariableTypeAlarmDEV,"dafne_file_not_updated",
		"raised when timestamp of dafne file is not updated (Storer problem)");
}
void ::driver::dafnepresenter::SCDafnePresenterControlUnit::unitDefineCustomAttribute() {
	char newdafnepath[256];
	strcpy(newdafnepath,this->loadedNewDafnePath.c_str());
	char outfile[256];
	strcpy(outfile, this->loadedOutFile.c_str());
	char fastfile[256];
	strcpy(fastfile,this->loadedFastPath.c_str());
	getAttributeCache()->addCustomAttribute("newdafnepath", sizeof(char)*256, chaos::DataType::TYPE_STRING);
    getAttributeCache()->setCustomAttributeValue("newdafnepath", newdafnepath, sizeof(char)*256);
	getAttributeCache()->addCustomAttribute("fastfilepath", sizeof(char)*256, chaos::DataType::TYPE_STRING);
    getAttributeCache()->setCustomAttributeValue("fastfilepath", fastfile, sizeof(char)*256);
	getAttributeCache()->addCustomAttribute("outFileName", sizeof(char)*256, chaos::DataType::TYPE_STRING);
    getAttributeCache()->setCustomAttributeValue("outFileName", outfile, sizeof(char)*256);
}
// Abstract method for the initialization of the control unit
void ::driver::dafnepresenter::SCDafnePresenterControlUnit::unitInit() {
}
// Abstract method for the start of the control unit
void ::driver::dafnepresenter::SCDafnePresenterControlUnit::unitStart() {
}
// Abstract method for the stop of the control unit
void ::driver::dafnepresenter::SCDafnePresenterControlUnit::unitStop()  {
}
// Abstract method for deinit the control unit
void ::driver::dafnepresenter::SCDafnePresenterControlUnit::unitDeinit() {
	SCCUAPP << "deinitializing ";
}
	//! restore the control unit to snapshot
#define RESTORE_LAPP SCCUAPP << "[RESTORE-" <<getCUID() << "] "
#define RESTORE_LERR SCCUERR << "[RESTORE-" <<getCUID() << "] "
bool ::driver::dafnepresenter::SCDafnePresenterControlUnit::unitRestoreToSnapshot(chaos::cu::control_manager::AbstractSharedDomainCache *const snapshot_cache)  {
	return false;
}
bool ::driver::dafnepresenter::SCDafnePresenterControlUnit::waitOnCommandID(uint64_t cmd_id) {
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
