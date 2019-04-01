/*
ChaosGibControlDD.h
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
#ifndef __driver_ChaosGibControlDD_h__
#define __driver_ChaosGibControlDD_h__
#include <chaos/cu_toolkit/driver_manager/driver/AbstractDriverPlugin.h>
#include <common/misc/GibControl/core/AbstractGibControl.h>
DEFINE_CU_DRIVER_DEFINITION_PROTOTYPE(ChaosGibControlDD)
namespace cu_driver = chaos::cu::driver_manager::driver;
namespace chaos {
	namespace driver {
		namespace gibcontrol {
			    /*         driver definition            */ 
			class ChaosGibControlDD: ADD_CU_DRIVER_PLUGIN_SUPERCLASS{
			protected: 
				::common::gibcontrol::AbstractGibControl* devicedriver;
			public: 
				ChaosGibControlDD();
				~ChaosGibControlDD();
				 //! Execute a command
				cu_driver::MsgManagmentResultType::MsgManagmentResult execOpcode(cu_driver::DrvMsgPtr cmd);
				void driverDeinit() ;
			}; //ChaosGibControlDD
		} //gibcontrol
	}
}
#endif
