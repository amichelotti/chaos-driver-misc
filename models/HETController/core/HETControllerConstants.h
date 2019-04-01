/*
HETControllerConstants.h
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
#ifndef HETController_HETControllerConstants_h
#define HETController_HETControllerConstants_h
namespace driver {
	namespace hetcontroller {
		#define TROW_ERROR(n,m,d) throw chaos::CException(n, m, d);
		#define LOG_TAIL(n) "[" << #n << "] - " << getDeviceID() << " - [" << getUID() << "] - " 
		const char* const CMD_HCT_DEFAULT_ALIAS = "Default";
		const char* const CMD_HCT_SWITCHHVPOWER_ALIAS = "SwitchHVPower";
		const char* const CMD_HCT_SWITCHHVPOWER_ON_STATE = "on_state";
		const char* const CMD_HCT_SETHVONCHANNEL_ALIAS = "SetHVOnChannel";
		const char* const CMD_HCT_SETHVONCHANNEL_SLOT = "slot";
		const char* const CMD_HCT_SETHVONCHANNEL_CHANNEL = "channel";
		const char* const CMD_HCT_SETHVONCHANNEL_VALUE = "value";
		const char* const CMD_HCT_SETHIGHTHRESHOLD_ALIAS = "SetHighThreshold";
		const char* const CMD_HCT_SETHIGHTHRESHOLD_SIDE = "side";
		const char* const CMD_HCT_SETHIGHTHRESHOLD_CHANNEL = "channel";
		const char* const CMD_HCT_SETHIGHTHRESHOLD_THRMILLIVOLT = "thrMillivolt";
		const char* const CMD_HCT_SETLOWTHRESHOLD_ALIAS = "SetLowThreshold";
		const char* const CMD_HCT_SETLOWTHRESHOLD_SIDE = "side";
		const char* const CMD_HCT_SETLOWTHRESHOLD_CHANNEL = "channel";
		const char* const CMD_HCT_SETLOWTHRESHOLD_THRMILLIVOLT = "thrMillivolt";
		const char* const CMD_HCT_SWITCHPULSE_ALIAS = "SwitchPulse";
		const char* const CMD_HCT_SWITCHPULSE_SIDE = "side";
		const char* const CMD_HCT_SWITCHPULSE_VALUE = "value";
		#define DEFAULT_COMMAND_TIMEOUT_MS   10000
	}
}
#endif
