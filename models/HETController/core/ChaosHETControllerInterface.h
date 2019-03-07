/*
ChaosHETControllerInterface.h
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
#ifndef __ChaosHETControllerInterface__
#define __ChaosHETControllerInterface__
#include <iostream>
#include <chaos/cu_toolkit/driver_manager/driver/DriverTypes.h>
#include <chaos/cu_toolkit/driver_manager/driver/DriverAccessor.h>
#include <common/debug/core/debug.h>
#include <stdint.h>
#include "AbstractHETController.h"
namespace chaos_driver=::chaos::cu::driver_manager::driver;
namespace chaos {
	namespace driver {
		#define MAX_STR_SIZE 256
		namespace hetcontroller {
			typedef enum {
				OP_SWITCHHVPOWER,
				OP_SETHVONCHANNEL,
				OP_SETHIGHTHRESHOLD,
				OP_SETLOWTHRESHOLD,
				OP_SWITCHPULSE
			} ChaosHETControllerOpcode;
			typedef struct {
				uint32_t timeout;
				int32_t int32_t1;
				int32_t int32_t2;
				int32_t int32_t3;
			} hetcontroller_iparams_t;
			typedef struct {
				int64_t int64_t1;
				int32_t result;
			} hetcontroller_oparams_t;
			//wrapper interface
			class ChaosHETControllerInterface:public ::common::hetcontroller::AbstractHETController {
				protected:
				chaos_driver::DrvMsg message;
				public: 
				ChaosHETControllerInterface(chaos_driver::DriverAccessor*_accessor):accessor(_accessor){};
				chaos_driver::DriverAccessor* accessor;
				int SwitchHVPower(int32_t on_state);
				int SetHVOnChannel(int32_t slot,int32_t channel,int32_t value);
				int SetHighThreshold(int32_t side,int32_t channel,int32_t thrMillivolt);
				int SetLowThreshold(int32_t side,int32_t channel,int32_t thrMillivolt);
				int SwitchPulse(int32_t side,int32_t value);
			};
		}
	}//driver
}//chaos
namespace chaos_hetcontroller_dd = chaos::driver::hetcontroller;
#endif
