/*
AbstractDafnePresenterCommand.cpp
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
#include "AbstractDafnePresenterCommand.h"
#include <boost/format.hpp>
#define CMDCUINFO_ INFO_LOG(AbstractDafnePresenterCommand)
#define CMDCUDBG_ DBG_LOG(AbstractDafnePresenterCommand)
#define CMDCUERR_ ERR_LOG(AbstractDafnePresenterCommand)
using namespace driver::dafnepresenter;
namespace chaos_batch = chaos::common::batch_command;
using namespace chaos::cu::control_manager;
AbstractDafnePresenterCommand::AbstractDafnePresenterCommand() {
	dafnepresenter_drv = NULL;
}
AbstractDafnePresenterCommand::~AbstractDafnePresenterCommand() {
	if(dafnepresenter_drv)
		delete(dafnepresenter_drv);
	dafnepresenter_drv = NULL;
	if (CCALT)
		delete(this->CCALT);
	this->CCALT=NULL;
	if (VUGImporter)
		delete(this->VUGImporter);
	this->VUGImporter=NULL;
	if (this->DAFNE_ELAB)
		delete(this->DAFNE_ELAB);
		this->DAFNE_ELAB=NULL;
	if (this->RFImporter)
		delete(this->RFImporter);
		this->RFImporter=NULL;
}
void AbstractDafnePresenterCommand::setHandler(c_data::CDataWrapper *data) {
	CMDCUDBG_ << "loading pointer for output channel"; 
	//get pointer to the output dataset variable
	//o_status_id = getAttributeCache()->getRWPtr<int32_t>(DOMAIN_OUTPUT, "status_id");
	//o_alarms = getAttributeCache()->getRWPtr<uint64_t>(DOMAIN_OUTPUT, "alarms"); 
	const char*vugNamePointer= getAttributeCache()->getROPtr<char>(DOMAIN_CUSTOM,"CUvugImportName");
	const char*lumiNamePointer = getAttributeCache()->getROPtr<char>(DOMAIN_CUSTOM,"CULuminometerCCALT");
	this->CCALT= new  ::driver::misc::ChaosController(lumiNamePointer);
	this->DAFNE_ELAB= new   ::driver::misc::ChaosController("DAFNE/ELAB/DAFNE_STATE");
	this->VUGImporter= new  ::driver::misc::ChaosController(vugNamePointer);
	this->RFImporter= new   ::driver::misc::ChaosController("DAFNE/IMPORT/DAFNE_RFE");
	//setting default timeout (usec) 
	const int32_t *userTimeout=getAttributeCache()->getROPtr<int32_t>(DOMAIN_INPUT,"driver_timeout");
	if (*userTimeout > 0)
		setFeatures(chaos_batch::features::FeaturesFlagTypes::FF_SET_COMMAND_TIMEOUT,(uint32_t) (*userTimeout)*1000);
	else
		setFeatures(chaos_batch::features::FeaturesFlagTypes::FF_SET_COMMAND_TIMEOUT,(uint32_t) 10000000);
	chaos::cu::driver_manager::driver::DriverAccessor *dafnepresenter_accessor = driverAccessorsErogator->getAccessoInstanceByIndex(0);
	
	if(dafnepresenter_accessor != NULL) {
		if(dafnepresenter_drv == NULL) {
			dafnepresenter_drv = new chaos::driver::dafnepresenter::ChaosDafnePresenterInterface(dafnepresenter_accessor);
		}
	}
}
// return the implemented handler
uint8_t AbstractDafnePresenterCommand::implementedHandler() {
	return  chaos_batch::HandlerType::HT_Set | chaos_batch::HandlerType::HT_Correlation;
}
void AbstractDafnePresenterCommand::ccHandler() {

}
void AbstractDafnePresenterCommand::setWorkState(bool working_flag) {
	setBusyFlag(working_flag);
}

