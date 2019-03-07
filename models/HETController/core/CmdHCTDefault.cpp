/*
CmdHCTDefault.cpp
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
#include "CmdHCTDefault.h"

#include <cmath>
#include  <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <sstream>
#define SCLAPP_ INFO_LOG(CmdHCTDefault) << "[" << getDeviceID() << "] "
#define SCLDBG_ DBG_LOG(CmdHCTDefault) << "[" << getDeviceID() << "] "
#define SCLERR_ ERR_LOG(CmdHCTDefault) << "[" << getDeviceID() << "] "
namespace own = driver::hetcontroller;
namespace c_data =  chaos::common::data;
namespace chaos_batch = chaos::common::batch_command;
using namespace chaos::cu::control_manager;
BATCH_COMMAND_OPEN_DESCRIPTION(driver::hetcontroller::,CmdHCTDefault,
	"Default command executed when no other commands in queue",
	"43150730-3521-48d3-aadb-547bf630226c")
BATCH_COMMAND_CLOSE_DESCRIPTION()


// return the implemented handler
uint8_t own::CmdHCTDefault::implementedHandler(){
	return      AbstractHETControllerCommand::implementedHandler()|chaos_batch::HandlerType::HT_Acquisition;
}
// empty set handler
void own::CmdHCTDefault::setHandler(c_data::CDataWrapper *data) {
	AbstractHETControllerCommand::setHandler(data);
	clearFeatures(chaos_batch::features::FeaturesFlagTypes::FF_SET_COMMAND_TIMEOUT);
	setBusyFlag(false);
	SCLAPP_ << "Set Handler Default "; 
	BC_NORMAL_RUNNING_PROPERTY
}
// empty acquire handler
void own::CmdHCTDefault::acquireHandler() {
	SCLDBG_ << "o_status_id: " << *o_status_id;
	SCLDBG_ << "o_alarms: " << *o_alarms;
	getAttributeCache()->setOutputDomainAsChanged();
}
// empty correlation handler
void own::CmdHCTDefault::ccHandler() {
}
// empty timeout handler
bool own::CmdHCTDefault::timeoutHandler() {
	SCLDBG_ << "Timeout Handler Default "; 
	return false;
}
