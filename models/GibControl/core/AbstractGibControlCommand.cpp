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
	o_status_id = getAttributeCache()->getRWPtr<int32_t>(DOMAIN_OUTPUT, "status_id"); 
	o_status = getAttributeCache()->getRWPtr<char>(DOMAIN_OUTPUT, "status");
	o_alarms = getAttributeCache()->getRWPtr<uint64_t>(DOMAIN_OUTPUT, "alarms"); 
	//get pointer to the output dataset variable
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
void AbstractGibControlCommand::getState(int& current_state, std::string& current_state_str) {
	CHAOS_ASSERT(gibcontrol_drv)
	int err = 0;
	std::string state_str;
	int32_t i_driver_timeout = getAttributeCache()->getValue<int32_t>(DOMAIN_INPUT, "driver_timeout");
	/*if((err=gibcontrol_drv->getState(current_state, state_str)) != 0) {
		setWorkState(false);
		CMDCUERR_ << boost::str( boost::format("Error getting the GibControl state = %1% ") % err); 
	}*/
}

void AbstractGibControlCommand::setWorkState(bool working_flag) {
	int64_t *o_dev_state = getAttributeCache()->getRWPtr<int64_t>(DOMAIN_OUTPUT, "dev_state");
	*o_dev_state = working_flag;
}

