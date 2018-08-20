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
BATCH_COMMAND_OPEN_DESCRIPTION_ALIAS(driver::gibcontrol::,CmdGIBDefault,CMD_GIB_DEFAULT_ALIAS,
			"Default command",
			"d3bdc407-89f1-404d-bd67-8a07459f6193")
BATCH_COMMAND_CLOSE_DESCRIPTION()


// return the implemented handler
uint8_t own::CmdGIBDefault::implementedHandler(){
	return      AbstractGibControlCommand::implementedHandler()|chaos_batch::HandlerType::HT_Acquisition;
}
// empty set handler
void own::CmdGIBDefault::setHandler(c_data::CDataWrapper *data) {
	AbstractGibControlCommand::setHandler(data);
	SCLAPP_ <<"GIB Set Handler Default" ;
	BC_NORMAL_RUNNING_PROPERTY
}
// empty acquire handler
void own::CmdGIBDefault::acquireHandler() {
}
// empty correlation handler
void own::CmdGIBDefault::ccHandler() {
	SCLAPP_ <<"GIB ACQUIRE Handler Default" ;
        o_status_id=getAttributeCache()->getRWPtr<int32_t>(DOMAIN_OUTPUT,"status_id");
	*o_status_id=12;

	getAttributeCache()->setOutputDomainAsChanged();
}
// empty timeout handler
bool own::CmdGIBDefault::timeoutHandler() {
	return false;
}
