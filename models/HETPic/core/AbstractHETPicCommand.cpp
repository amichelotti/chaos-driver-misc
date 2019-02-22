/*
AbstractHETPicCommand.cpp
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
#include "AbstractHETPicCommand.h"
#include <boost/format.hpp>
#define CMDCUINFO_ INFO_LOG(AbstractHETPicCommand)
#define CMDCUDBG_ DBG_LOG(AbstractHETPicCommand)
#define CMDCUERR_ ERR_LOG(AbstractHETPicCommand)
using namespace driver::hetpic;
namespace chaos_batch = chaos::common::batch_command;
using namespace chaos::cu::control_manager;
AbstractHETPicCommand::AbstractHETPicCommand() {
	hetpic_drv = NULL;
}
AbstractHETPicCommand::~AbstractHETPicCommand() {
	if(hetpic_drv)
		delete(hetpic_drv);
	hetpic_drv = NULL;
}
void AbstractHETPicCommand::setHandler(c_data::CDataWrapper *data) {
	CMDCUDBG_ << "loading pointer for output channel"; 
	//get pointer to the output dataset variable
	o_status_id = getAttributeCache()->getRWPtr<int32_t>(DOMAIN_OUTPUT, "status_id");
	o_alarms = getAttributeCache()->getRWPtr<uint64_t>(DOMAIN_OUTPUT, "alarms"); 
	numOfChanPt =getAttributeCache()->getRWPtr<int32_t>(DOMAIN_OUTPUT, "NumberOfChannels");;
	//setting default timeout (usec) 
	setFeatures(chaos_batch::features::FeaturesFlagTypes::FF_SET_COMMAND_TIMEOUT,(uint32_t) 10000000);
	chaos::cu::driver_manager::driver::DriverAccessor *hetpic_accessor = driverAccessorsErogator->getAccessoInstanceByIndex(0);
	if(hetpic_accessor != NULL) {
		if(hetpic_drv == NULL) {
			hetpic_drv = new chaos::driver::hetpic::ChaosHETPicInterface(hetpic_accessor);
		}
	}
}
// return the implemented handler
uint8_t AbstractHETPicCommand::implementedHandler() {
	return  chaos_batch::HandlerType::HT_Set | chaos_batch::HandlerType::HT_Correlation;
}
void AbstractHETPicCommand::ccHandler() {

}
void AbstractHETPicCommand::setWorkState(bool working_flag) {
	setBusyFlag(working_flag);
}

