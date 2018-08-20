/*
GibControlConstants.h
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
#ifndef GibControl_GibControlConstants_h
#define GibControl_GibControlConstants_h
namespace driver {
	namespace gibcontrol {
		#define TROW_ERROR(n,m,d) throw chaos::CException(n, m, d);
		#define LOG_TAIL(n) "[" << #n << "] - " << getDeviceID() << " - [" << getUID() << "] - " 
		const char* const CMD_GIB_SETPULSE_ALIAS = "setPulse";
		const char* const CMD_GIB_SETPULSE_CHANNEL = "channel";
		const char* const CMD_GIB_SETPULSE_AMPLITUDE = "amplitude";
		const char* const CMD_GIB_SETPULSE_WIDTH = "width";
		const char* const CMD_GIB_SETPULSE_STATE = "state";
		const char* const CMD_GIB_SETCHANNELVOLTAGE_ALIAS = "setChannelVoltage";
		const char* const CMD_GIB_SETCHANNELVOLTAGE_CHANNEL = "channel";
		const char* const CMD_GIB_SETCHANNELVOLTAGE_VOLTAGE = "Voltage";
		const char* const CMD_GIB_DEFAULT_ALIAS = "Default";
		#define DEFAULT_COMMAND_TIMEOUT_MS   10000
	}
}
#endif
