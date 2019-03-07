/*
AbstractHETController.h
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
#include <inttypes.h>
#include <string>
#ifndef __common_AbstractHETController_h__
#define __common_AbstractHETController_h__
namespace common {
	namespace hetcontroller {
		class AbstractHETController {
		  public:
			AbstractHETController() {};
			virtual ~AbstractHETController() {};
			virtual int SwitchHVPower(int32_t on_state)=0;
			virtual int SetHVOnChannel(int32_t slot,int32_t channel,int32_t value)=0;
			virtual int SetHighThreshold(int32_t side,int32_t channel,int32_t thrMillivolt)=0;
			virtual int SetLowThreshold(int32_t side,int32_t channel,int32_t thrMillivolt)=0;
			virtual int SwitchPulse(int32_t side,int32_t value)=0;
		};
	}
}//common
#endif
