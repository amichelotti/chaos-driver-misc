/*
CmdHPCDefault.cpp
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
#include "CmdHPCDefault.h"

#include <cmath>
#include  <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <sstream>
#define SCLAPP_ INFO_LOG(CmdHPCDefault) << "[" << getDeviceID() << "] "
#define SCLDBG_ DBG_LOG(CmdHPCDefault) << "[" << getDeviceID() << "] "
#define SCLERR_ ERR_LOG(CmdHPCDefault) << "[" << getDeviceID() << "] "
namespace own = driver::hetpic;
namespace c_data =  chaos::common::data;
namespace chaos_batch = chaos::common::batch_command;
using namespace chaos::cu::control_manager;
BATCH_COMMAND_OPEN_DESCRIPTION(driver::hetpic::,CmdHPCDefault,
	"Default command executed when no other commands in queue",
	"3955c779-1267-4964-8235-ee433a9c9b81")
BATCH_COMMAND_CLOSE_DESCRIPTION()


// return the implemented handler
uint8_t own::CmdHPCDefault::implementedHandler(){
	return      AbstractHETPicCommand::implementedHandler()|chaos_batch::HandlerType::HT_Acquisition;
}
// empty set handler
void own::CmdHPCDefault::setHandler(c_data::CDataWrapper *data) {
	AbstractHETPicCommand::setHandler(data);
	clearFeatures(chaos_batch::features::FeaturesFlagTypes::FF_SET_COMMAND_TIMEOUT);
	setBusyFlag(false);
	SCLAPP_ << "Set Handler Default "; 
	BC_NORMAL_RUNNING_PROPERTY
}
// empty acquire handler
void own::CmdHPCDefault::acquireHandler() {
	SCLDBG_ << "o_status_id: " << *o_status_id;
	SCLDBG_ << "o_alarms: " << *o_alarms;
	getAttributeCache()->setOutputDomainAsChanged();
}
// empty correlation handler
void own::CmdHPCDefault::ccHandler() {
}
// empty timeout handler
bool own::CmdHPCDefault::timeoutHandler() {
	SCLDBG_ << "Timeout Handler Default "; 
	return false;
}
