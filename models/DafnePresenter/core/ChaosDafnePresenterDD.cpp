/*
ChaosDafnePresenterDD.cpp
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
#include "ChaosDafnePresenterDD.h"
#include <string>
#include <regex>
#include <chaos/cu_toolkit/driver_manager/driver/AbstractDriverPlugin.h>
// including interface
#include "driver/misc/models/DafnePresenter/core/ChaosDafnePresenterInterface.h"
#define ACLAPP	LAPP_ << "[ChaosDafnePresenterDD] "
#define ACDBG	LDBG_ << "[ChaosDafnePresenterDD] "
#define ACERR	LERR_ << "[ChaosDafnePresenterDD] "
using namespace chaos::driver::dafnepresenter;
//default constructor definition
DEFAULT_CU_DRIVER_PLUGIN_CONSTRUCTOR_WITH_NS(chaos::driver::dafnepresenter, ChaosDafnePresenterDD) {
	devicedriver = NULL;
}
ChaosDafnePresenterDD::~ChaosDafnePresenterDD() {
}
void ChaosDafnePresenterDD::driverDeinit() {
	 if(devicedriver) {
		delete devicedriver;
	}
	devicedriver = NULL;
}
cu_driver::MsgManagmentResultType::MsgManagmentResult ChaosDafnePresenterDD::execOpcode(cu_driver::DrvMsgPtr cmd){
	 cu_driver::MsgManagmentResultType::MsgManagmentResult result = cu_driver::MsgManagmentResultType::MMR_EXECUTED;
	dafnepresenter_iparams_t *in = (dafnepresenter_iparams_t *)cmd->inputData;
	dafnepresenter_oparams_t *out = (dafnepresenter_oparams_t *)cmd->resultData;
	switch(cmd->opcode){
	}
	return result;
}
