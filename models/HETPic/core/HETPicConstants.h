/*
HETPicConstants.h
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
#ifndef HETPic_HETPicConstants_h
#define HETPic_HETPicConstants_h
namespace driver {
	namespace hetpic {
		#define TROW_ERROR(n,m,d) throw chaos::CException(n, m, d);
		#define LOG_TAIL(n) "[" << #n << "] - " << getDeviceID() << " - [" << getUID() << "] - " 
		const char* const CMD_HPC_DEFAULT_ALIAS = "Default";
		const char* const CMD_HPC_SETHIGHTHRESHOLD_ALIAS = "SetHighThreshold";
		const char* const CMD_HPC_SETHIGHTHRESHOLD_CHANNEL = "channel";
		const char* const CMD_HPC_SETHIGHTHRESHOLD_MILLIVOLTS = "millivolts";
		const char* const CMD_HPC_SETLOWTHRESHOLD_ALIAS = "SetLowThreshold";
		const char* const CMD_HPC_SETLOWTHRESHOLD_CHANNEL = "channel";
		const char* const CMD_HPC_SETLOWTHRESHOLD_MILLIVOLTS = "millivolts";
		const char* const CMD_HPC_SETPULSE_ALIAS = "setPulse";
		const char* const CMD_HPC_SETPULSE_VALUE = "value";
		const char* const CMD_HPC_GETNUMBEROFCHANNEL_ALIAS = "getNumberOfChannel";
		const char* const CMD_HPC_GETNUMBEROFCHANNEL_CHANNUM = "chanNum";
		const char* const CMD_HPC_GETSTATUS_ALIAS = "getStatus";
		const char* const CMD_HPC_GETSTATUS_STATUS = "status";
		const char* const CMD_HPC_GETHIGHTHRESHOLDS_ALIAS = "getHighThresholds";
		const char* const CMD_HPC_GETHIGHTHRESHOLDS_HIGHTHRESHOLDS = "highThresholds";
		const char* const CMD_HPC_GETLOWTHRESHOLDS_ALIAS = "getLowThresholds";
		const char* const CMD_HPC_GETLOWTHRESHOLDS_LOWTHRESHOLDS = "lowThresholds";
		#define DEFAULT_COMMAND_TIMEOUT_MS   10000
	}
}
#endif
