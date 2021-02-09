/*
ChaosGibControlInterface.h
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
#ifndef __ChaosGibControlInterface__
#define __ChaosGibControlInterface__
//#include <chaos/cu_toolkit/driver_manager/driver/DriverTypes.h>
#include <chaos/cu_toolkit/driver_manager/driver/DriverAccessor.h>
#include <common/debug/core/debug.h>
#include <common/misc/GibControl/core/AbstractGibControl.h>

/*namespace chaos{
	namespace cu{
		namespace driver_manager{
			namespace driver{
				class DriverAccessor;
			}
		}
	}
}
*/
namespace chaos_driver=chaos::cu::driver_manager::driver;
namespace chaos {
	namespace driver {
#define MAX_STR_SIZE 256
		namespace gibcontrol {
			class ChaosGibControlDD;
			typedef enum {
				
				OP_DEINIT, // deinit low level driver
				OP_SETPULSE,
				OP_SETCHANNELVOLTAGE,
				OP_POWERON,
				OP_GETSTATE,
				OP_GETVOLTAGES,
				OP_GETNUMOFCHANNELS,
				OP_GETPULSINGSTATE,
				OP_GETSUPPLYVOLTAGES
				
			} ChaosGibControlOpcode;
			typedef struct {
				uint32_t timeout;
				int32_t int32_t1;
				int32_t int32_t2;
				int32_t int32_t3;
				int32_t int32_t4;
				double double1;
			} gibcontrol_iparams_t;
			typedef struct {
				int64_t int64_t1;
				int32_t result;
				int32_t  int32_tP1;
				std::string  stringE1;
				std::vector<double>  vector_double_E1;
				std::vector<int32_t>  vector_int32_t_E1;
				std::vector<int32_t>  vector_int32_t_E2;
				double  doubleP1;
				double  doubleP2;
				double  doubleP3;
			} gibcontrol_oparams_t;
			//wrapper interface
			class ChaosGibControlInterface:public ::common::gibcontrol::AbstractGibControl {
				protected:
				chaos_driver::DrvMsg message;
				ChaosGibControlDD*impl;
				public: 
				ChaosGibControlInterface(chaos_driver::DriverAccessor*_accessor):accessor(_accessor){impl=(ChaosGibControlDD*)_accessor->getImpl();};
				chaos_driver::DriverAccessor* accessor;
				int init(void*);
				int deinit();
				uint64_t getFeatures();
				int setPulse(int32_t channel,int32_t amplitude,int32_t width,int32_t state);
				int setChannelVoltage(int32_t channel,double Voltage);
				int PowerOn(int32_t on_state);
				int getState(int32_t* state,std::string& desc);
				int getVoltages(std::vector<double>& voltages);
				int getNumOfChannels(int32_t* numOfChannels);
				int getPulsingState(std::vector<int32_t>& amplitudes,std::vector<int32_t>& widthChannels);
				int getSupplyVoltages(double* HVSupply,double* P5V,double* N5V);
			};
		}
	}//driver
}//chaos
namespace chaos_gibcontrol_dd = chaos::driver::gibcontrol;
#endif
