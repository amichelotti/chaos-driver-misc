/*
ChaosHETPicInterface.h
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
#ifndef __ChaosHETPicInterface__
#define __ChaosHETPicInterface__
#include <iostream>
#include <chaos/cu_toolkit/driver_manager/driver/DriverTypes.h>
#include <chaos/cu_toolkit/driver_manager/driver/DriverAccessor.h>
#include <common/debug/core/debug.h>
#include <stdint.h>
#include <common/misc/HETPic/core/AbstractHETPic.h>
namespace chaos_driver=::chaos::cu::driver_manager::driver;
namespace chaos {
	namespace driver {
		#define MAX_STR_SIZE 256
		namespace hetpic {
			typedef enum {
				OP_SETHIGHTHRESHOLD,
				OP_SETLOWTHRESHOLD,
				OP_SETPULSE,
				OP_GETNUMBEROFCHANNEL,
				OP_GETSTATUS,
				OP_GETHIGHTHRESHOLDS,
				OP_GETLOWTHRESHOLDS
			} ChaosHETPicOpcode;
			typedef struct {
				uint32_t timeout;
				int32_t int32_t1;
				int32_t int32_t2;
			} hetpic_iparams_t;
			typedef struct {
				int64_t int64_t1;
				int32_t result;
				int32_t  int32_tE1;
				std::vector<int32_t>  vector_int32_t_E1;
			} hetpic_oparams_t;
			//wrapper interface
			class ChaosHETPicInterface:public ::common::hetpic::AbstractHETPic {
				protected:
				chaos_driver::DrvMsg message;
				public: 
				ChaosHETPicInterface(chaos_driver::DriverAccessor*_accessor):accessor(_accessor){};
				chaos_driver::DriverAccessor* accessor;
				int SetHighThreshold(int32_t channel,int32_t millivolts);
				int SetLowThreshold(int32_t channel,int32_t millivolts);
				int setPulse(int32_t value);
				int getNumberOfChannel(int32_t& chanNum);
				int getStatus(int32_t& status);
				int getHighThresholds(std::vector<int32_t>& highThresholds);
				int getLowThresholds(std::vector<int32_t>& lowThresholds);
			};
		}
	}//driver
}//chaos
namespace chaos_hetpic_dd = chaos::driver::hetpic;
#endif
