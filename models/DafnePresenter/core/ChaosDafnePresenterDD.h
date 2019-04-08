/*
ChaosDafnePresenterDD.h
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
#ifndef __driver_ChaosDafnePresenterDD_h__
#define __driver_ChaosDafnePresenterDD_h__
#include <chaos/cu_toolkit/driver_manager/driver/AbstractDriverPlugin.h>
#include <common/misc/DafnePresenter/core/AbstractDafnePresenter.h>
DEFINE_CU_DRIVER_DEFINITION_PROTOTYPE(ChaosDafnePresenterDD)
namespace cu_driver = chaos::cu::driver_manager::driver;
namespace chaos {
	namespace driver {
		namespace dafnepresenter {
			    /*         driver definition            */ 
			class ChaosDafnePresenterDD: ADD_CU_DRIVER_PLUGIN_SUPERCLASS{
			protected: 
				::common::dafnepresenter::AbstractDafnePresenter* devicedriver;
			public: 
				ChaosDafnePresenterDD();
				~ChaosDafnePresenterDD();
				 //! Execute a command
				cu_driver::MsgManagmentResultType::MsgManagmentResult execOpcode(cu_driver::DrvMsgPtr cmd);
				void driverDeinit();
			}; //ChaosDafnePresenterDD
		} //dafnepresenter
	}
}
#endif
