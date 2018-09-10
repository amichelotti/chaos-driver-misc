/*
CmdGIBDefault.cpp
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
#include "CmdGIBDefault.h"

#include <cmath>
#include  <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <sstream>
#define SCLAPP_ INFO_LOG(CmdGIBDefault) << "[" << getDeviceID() << "] "
#define SCLDBG_ DBG_LOG(CmdGIBDefault) << "[" << getDeviceID() << "] "
#define SCLERR_ ERR_LOG(CmdGIBDefault) << "[" << getDeviceID() << "] "
namespace own = driver::gibcontrol;
namespace c_data =  chaos::common::data;
namespace chaos_batch = chaos::common::batch_command;
using namespace chaos::cu::control_manager;
BATCH_COMMAND_OPEN_DESCRIPTION(driver::gibcontrol::,CmdGIBDefault,
			"Default Command",
			"041d71fe-4f08-4e74-a364-94c873ca2791")
BATCH_COMMAND_CLOSE_DESCRIPTION()


// return the implemented handler
uint8_t own::CmdGIBDefault::implementedHandler(){
	return      AbstractGibControlCommand::implementedHandler()|chaos_batch::HandlerType::HT_Acquisition;
}
// empty set handler
void own::CmdGIBDefault::setHandler(c_data::CDataWrapper *data) {
	AbstractGibControlCommand::setHandler(data);
	setBusyFlag(false);
	SCLAPP_ << "Set Handler Default "; 

	BC_NORMAL_RUNNING_PROPERTY
}
// empty acquire handler
void own::CmdGIBDefault::acquireHandler() {
 	int state;
	int err= 0;
        std::string descr;
	double *channels=getAttributeCache()->getRWPtr<double>(DOMAIN_OUTPUT,"HVCHANNELS");
        if (err=gibcontrol_drv->getState(&state,descr) != 0)
	{
		 metadataLogging(chaos::common::metadata_logging::StandardLoggingChannel::LogLevelError," cannot retrieve status of GIB");
	}
       	*o_status_id=state;
       	o_status=strncpy(o_status,descr.c_str(),256);
	SCLDBG_ << "o_status: " << o_status;
	SCLDBG_ << "o_status_id: " << *o_status_id;
	SCLDBG_ << "o_alarms: " << *o_alarms;
	std::vector<double> Voltaggi;
	if (err=gibcontrol_drv->getVoltages(Voltaggi) != 0 )
	{
		 metadataLogging(chaos::common::metadata_logging::StandardLoggingChannel::LogLevelError," cannot retrieve channel voltages");
	}
	else
	{
		for (int i=0; i < Voltaggi.size(); i++)
		{
			char nums[8];
			sprintf(nums,"%d",i);
			std::string attrname=(std::string)"CH"+ nums;
			double *tmp=getAttributeCache()->getRWPtr<double>(DOMAIN_OUTPUT,attrname);
			*tmp=Voltaggi[i];
	   		channels[i]=Voltaggi[i];
		}
	}
	getAttributeCache()->setOutputDomainAsChanged();
}
// empty correlation handler
void own::CmdGIBDefault::ccHandler() {
}
// empty timeout handler
bool own::CmdGIBDefault::timeoutHandler() {
	return false;
}
