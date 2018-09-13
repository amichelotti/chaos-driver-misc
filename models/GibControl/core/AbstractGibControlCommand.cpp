/*
AbstractGibControlCommand.cpp
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
#include "AbstractGibControlCommand.h"
#include <boost/format.hpp>
#define CMDCUINFO_ INFO_LOG(AbstractGibControlCommand)
#define CMDCUDBG_ DBG_LOG(AbstractGibControlCommand)
#define CMDCUERR_ ERR_LOG(AbstractGibControlCommand)
using namespace driver::gibcontrol;
namespace chaos_batch = chaos::common::batch_command;
using namespace chaos::cu::control_manager;
AbstractGibControlCommand::AbstractGibControlCommand() {
	gibcontrol_drv = NULL;
}
AbstractGibControlCommand::~AbstractGibControlCommand() {
	if(gibcontrol_drv)
		delete(gibcontrol_drv);
	gibcontrol_drv = NULL;
}
void AbstractGibControlCommand::setHandler(c_data::CDataWrapper *data) {
	CMDCUDBG_ << "loading pointer for output channel";
	//get pointer to the output dataset variables 
	o_status_id = getAttributeCache()->getRWPtr<int32_t>(DOMAIN_OUTPUT, "status_id"); 
	o_status = getAttributeCache()->getRWPtr<char>(DOMAIN_OUTPUT, "status");
	o_alarms = getAttributeCache()->getRWPtr<uint64_t>(DOMAIN_OUTPUT, "alarms"); 
	numOfchannels = getAttributeCache()->getROPtr<int32_t>(DOMAIN_OUTPUT,"numberOfChannels");
	
	//POTREBBERO ESSERE MICROSECONDI
	
	const int32_t* reg_command_timeout= getAttributeCache()->getROPtr<int32_t>(DOMAIN_INPUT,"driver_timeout");
	int32_t commandTimeout;
	if ((reg_command_timeout!= NULL) && ((*reg_command_timeout) != 0) )
	    commandTimeout=(*reg_command_timeout)*1000;
	else
	    commandTimeout=10000000;
	setFeatures(chaos_batch::features::FeaturesFlagTypes::FF_SET_COMMAND_TIMEOUT,(uint32_t) commandTimeout);

	chaos::cu::driver_manager::driver::DriverAccessor *gibcontrol_accessor = driverAccessorsErogator->getAccessoInstanceByIndex(0);
	if(gibcontrol_accessor != NULL) {
		if(gibcontrol_drv == NULL) {
			gibcontrol_drv = new chaos::driver::gibcontrol::ChaosGibControlInterface(gibcontrol_accessor);
		}
	}
}
// return the implemented handler
uint8_t AbstractGibControlCommand::implementedHandler() {
	return  chaos_batch::HandlerType::HT_Set | chaos_batch::HandlerType::HT_Correlation;
}
void AbstractGibControlCommand::ccHandler() {

}
void AbstractGibControlCommand::setWorkState(bool working_flag) {
	setBusyFlag(working_flag);
}

void AbstractGibControlCommand::clearCUAlarms() {
	setStateVariableSeverity(StateVariableTypeAlarmCU,"driver_command_error",chaos::common::alarm::MultiSeverityAlarmLevelClear);
	setStateVariableSeverity(StateVariableTypeAlarmCU,"bad_command_parameter",chaos::common::alarm::MultiSeverityAlarmLevelClear);
	setStateVariableSeverity(StateVariableTypeAlarmCU,"setPoint_not_reached",chaos::common::alarm::MultiSeverityAlarmLevelClear);
}