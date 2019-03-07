/*
AbstractHETControllerCommand.cpp
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
#include "AbstractHETControllerCommand.h"
#include <boost/format.hpp>
#define CMDCUINFO_ INFO_LOG(AbstractHETControllerCommand)
#define CMDCUDBG_ DBG_LOG(AbstractHETControllerCommand)
#define CMDCUERR_ ERR_LOG(AbstractHETControllerCommand)
using namespace driver::hetcontroller;
namespace chaos_batch = chaos::common::batch_command;
using namespace chaos::cu::control_manager;
AbstractHETControllerCommand::AbstractHETControllerCommand() {
	hetcontroller_drv = NULL;
}
AbstractHETControllerCommand::~AbstractHETControllerCommand() {
	if(hetcontroller_drv)
		delete(hetcontroller_drv);
	hetcontroller_drv = NULL;
}
void AbstractHETControllerCommand::setHandler(c_data::CDataWrapper *data) {
	CMDCUDBG_ << "loading pointer for output channel"; 
	//get pointer to the output dataset variable
	o_status_id = getAttributeCache()->getRWPtr<int32_t>(DOMAIN_OUTPUT, "status_id");
	o_alarms = getAttributeCache()->getRWPtr<uint64_t>(DOMAIN_OUTPUT, "alarms"); 
	//setting default timeout (usec) 
	const int32_t *userTimeout=getAttributeCache()->getROPtr<int32_t>(DOMAIN_INPUT,"driver_timeout");
	if (*userTimeout > 0)
		setFeatures(chaos_batch::features::FeaturesFlagTypes::FF_SET_COMMAND_TIMEOUT,(uint32_t) (*userTimeout)*1000);
	else
		setFeatures(chaos_batch::features::FeaturesFlagTypes::FF_SET_COMMAND_TIMEOUT,(uint32_t) 10000000);
	chaos::cu::driver_manager::driver::DriverAccessor *hetcontroller_accessor = driverAccessorsErogator->getAccessoInstanceByIndex(0);
	if(hetcontroller_accessor != NULL) {
		if(hetcontroller_drv == NULL) {
			hetcontroller_drv = new chaos::driver::hetcontroller::ChaosHETControllerInterface(hetcontroller_accessor);
		}
	}
}
// return the implemented handler
uint8_t AbstractHETControllerCommand::implementedHandler() {
	return  chaos_batch::HandlerType::HT_Set | chaos_batch::HandlerType::HT_Correlation;
}
void AbstractHETControllerCommand::ccHandler() {

}
void AbstractHETControllerCommand::setWorkState(bool working_flag) {
	setBusyFlag(working_flag);
}

