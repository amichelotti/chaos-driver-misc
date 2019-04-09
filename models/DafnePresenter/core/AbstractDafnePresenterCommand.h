/*
AbstractDafnePresenterCommand.h
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
#ifndef __AbstractDafnePresenterCommand__
#define __AbstractDafnePresenterCommand__
#include "DafnePresenterConstants.h"
#include <driver/misc/models/DafnePresenter/core/ChaosDafnePresenterInterface.h>
#include <chaos/cu_toolkit/control_manager/slow_command/SlowCommand.h>

namespace c_data = chaos::common::data;
namespace ccc_slow_command = chaos::cu::control_manager::slow_command;
namespace driver {
	namespace dafnepresenter{
		class AbstractDafnePresenterCommand: public ccc_slow_command::SlowCommand {
		public:
			AbstractDafnePresenterCommand();
			~AbstractDafnePresenterCommand();
		protected: 
			//common members
			int32_t	*o_status_id;
			uint64_t	*o_alarms;
			//reference of the chaos abstraction of driver
			chaos::driver::dafnepresenter::ChaosDafnePresenterInterface *dafnepresenter_drv;
			//implemented handler
			uint8_t implementedHandler();
			void ccHandler();
			void setHandler(c_data::CDataWrapper *data);
			void setWorkState(bool working);
		};// AbstractDafnePresenterCommand
	}// dafnepresenter
}
#endif