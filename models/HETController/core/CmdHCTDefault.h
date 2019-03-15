/*
CmdHCTDefault.h
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
#ifndef __HETController__CmdHCTDefault_h__
#define __HETController__CmdHCTDefault_h__
#include "AbstractHETControllerCommand.h"
#include <bitset>
namespace c_data = chaos::common::data;
namespace ccc_slow_command = chaos::cu::control_manager::slow_command;
namespace driver {
	namespace hetcontroller {
		DEFINE_BATCH_COMMAND_CLASS(CmdHCTDefault,AbstractHETControllerCommand) {
			//implemented handler
			uint8_t implementedHandler();
			//initial set handler
			void setHandler(c_data::CDataWrapper *data);
			//custom acquire handler
			void acquireHandler();
			//correlation and commit handler
			void ccHandler();
			//manage the timeout 
			bool timeoutHandler();
			bool UpdateVoltagesFromDataset(chaos::common::data::CDWShrdPtr fetched, int32_t* LTHR_dest,int32_t* HTHR_dest);
			bool UpdateMPSFromDataset(chaos::common::data::CDWShrdPtr fetched);
			bool CheckAlarmsFromCUs(chaos::common::data::CDWShrdPtr fetchedAlarm,uint64_t* alarmBitMask,uint8_t element);
			void DecodeAlarmMaskAndRaiseAlarms();

			int32_t* o_Positron_HTHR;
			int32_t* o_Positron_LTHR;
			int32_t* o_Electron_HTHR;
			int32_t* o_Electron_LTHR;
			int64_t* hvChannelStatus;
			int64_t* hvChannelAlarms;
			double* hvChannelVoltages;
			char* mainUnitStatusDescription;
			bool loggedNotStartedMsg;
			bool loggedDeadCUMsg;
			bool loggedDataRetrieving;
			chaos::common::data::CDWShrdPtr  LVPDataset;
			chaos::common::data::CDWShrdPtr  LVEDataset;
			chaos::common::data::CDWShrdPtr  HVDataset;
		};
	}
}
#endif
