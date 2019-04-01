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
AbstractHETControllerCommand::AbstractHETControllerCommand() :LV_Positron(NULL),LV_Electron(NULL),HVPS(NULL){
	hetcontroller_drv = NULL;
	//
}
AbstractHETControllerCommand::~AbstractHETControllerCommand() {
	if(hetcontroller_drv)
		delete(hetcontroller_drv);
	if (LV_Positron)
		delete(LV_Positron);
	if (LV_Electron)
		delete(LV_Electron);
	if (HVPS)
		delete(HVPS);
	
	LV_Positron=LV_Electron=HVPS=NULL;
	hetcontroller_drv = NULL;
}
void AbstractHETControllerCommand::setHandler(c_data::CDataWrapper *data) {
	CMDCUDBG_ << "loading pointer for output channel"; 
	//get pointer to the output dataset variable
	o_status_id = getAttributeCache()->getRWPtr<int32_t>(DOMAIN_OUTPUT, "HV_MainUnit_Status");
	o_alarms = getAttributeCache()->getRWPtr<uint64_t>(DOMAIN_OUTPUT, "alarms"); 
	bool configOk=false;
	LVPName = getAttributeCache()->getROPtr<char>(DOMAIN_INPUT, "LVPositron_ControlUnit_CompleteName");
	if ((LVPName == NULL) || (strlen(LVPName)==0) )
	{
		CMDCUERR_ << "ALEDEBUG LVPName not set" ;
	}
	else
	{
		LV_Positron= new ::driver::misc::ChaosController(LVPName);
		LVEName = getAttributeCache()->getROPtr<char>(DOMAIN_INPUT, "LVElectron_ControlUnit_CompleteName");
		if ((LVEName == NULL) || (strlen(LVEName)==0) )
		{
			CMDCUERR_ << "ALEDEBUG LVEName not set" ;
		}
		else
		{
			LV_Electron= new ::driver::misc::ChaosController(LVEName);
			MPSName = getAttributeCache()->getROPtr<char>(DOMAIN_INPUT, "HVMPS_ControlUnit_CompleteName");
			if ((MPSName == NULL) || (strlen(MPSName)==0) )
			{
				CMDCUERR_ << "ALEDEBUG MPSName not set" ;
			}
			else
			{
				HVPS= new ::driver::misc::ChaosController(MPSName);
				configOk=true;

			}
		}
		
	}
	if (!configOk)
	{
		throw chaos::CException(-1, "Failed to config the control of CU", __FUNCTION__);
	}







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
std::pair<std::vector<int32_t>,std::vector<std::string>> AbstractHETControllerCommand::checkHealthState() {
	std::pair<std::vector<int32_t>,std::vector<std::string>> ReturnObject;
	std::vector<int32_t> differenceTimeVector;
	std::vector<std::string> CUStateVector;
	differenceTimeVector.resize(3);
	CUStateVector.resize(3);
	chaos::common::data::CDWShrdPtr  LVP_HDS, LVE_HDS, HVMPS_HDS;
	LVP_HDS=LV_Positron->getLiveChannel(LVPName,4);
	LVE_HDS=LV_Electron->getLiveChannel(LVEName,4);
	HVMPS_HDS=HVPS->getLiveChannel(MPSName,4);
	
	u_int64_t atsTime;
	time_t now;
	u_int64_t differenceTime;
	std::string custate;
	now=time(NULL);
	
	atsTime=LVP_HDS->getUInt64Value("dpck_ats");
	custate=LVP_HDS->getStringValue("nh_status");
	differenceTime=now-(u_int64_t)(atsTime/1000);
	differenceTimeVector[0]=differenceTime;
	CUStateVector[0]=custate;

	atsTime=LVE_HDS->getUInt64Value("dpck_ats");
	custate=LVE_HDS->getStringValue("nh_status");
	differenceTime=now-(u_int64_t)(atsTime/1000);
	differenceTimeVector[1]=differenceTime;
	CUStateVector[1]=custate;

	atsTime=HVMPS_HDS->getUInt64Value("dpck_ats");
	custate=HVMPS_HDS->getStringValue("nh_status");
	differenceTime=now-(u_int64_t)(atsTime/1000);
	differenceTimeVector[2]=differenceTime;
	CUStateVector[2]=custate;


	ReturnObject.first=differenceTimeVector;
	ReturnObject.second=CUStateVector;
	return ReturnObject;
}

