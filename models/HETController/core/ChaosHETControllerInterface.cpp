/*
ChaosHETControllerInterface.cpp
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
#include "ChaosHETControllerInterface.h"
using namespace chaos::driver::hetcontroller;
#define PREPARE_OP_RET_INT_TIMEOUT(op,tim) \
hetcontroller_oparams_t ret;\
hetcontroller_iparams_t idata;\
message.opcode = op; \
message.inputData=(void*)&idata;\
idata.timeout=tim;\
message.inputDataLength=sizeof(hetcontroller_iparams_t);\
message.resultDataLength=sizeof(hetcontroller_oparams_t);\
message.resultData = (void*)&ret;\

#define WRITE_OP_TIM(op,timeout) \
PREPARE_OP_RET_INT_TIMEOUT(op,timeout); \
accessor->send(&message);\
return ret.result;

#define WRITE_OP_INT32_T_TIM(op,VAR_int32_t1,timeout)\
PREPARE_OP_RET_INT_TIMEOUT(op,timeout); \
idata.int32_t1=VAR_int32_t1;\
accessor->send(&message);\
return ret.result;

#define WRITE_OP_INT32_T_INT32_T_INT32_T_TIM(op,VAR_int32_t1,VAR_int32_t2,VAR_int32_t3,timeout)\
PREPARE_OP_RET_INT_TIMEOUT(op,timeout); \
idata.int32_t1=VAR_int32_t1;\
idata.int32_t2=VAR_int32_t2;\
idata.int32_t3=VAR_int32_t3;\
accessor->send(&message);\
return ret.result;

#define WRITE_OP_INT32_T_INT32_T_TIM(op,VAR_int32_t1,VAR_int32_t2,timeout)\
PREPARE_OP_RET_INT_TIMEOUT(op,timeout); \
idata.int32_t1=VAR_int32_t1;\
idata.int32_t2=VAR_int32_t2;\
accessor->send(&message);\
return ret.result;

int ChaosHETControllerInterface::SwitchHVPower(int32_t on_state) {
	WRITE_OP_INT32_T_TIM(OP_SWITCHHVPOWER,on_state,0);
} 
int ChaosHETControllerInterface::SetHVOnChannel(int32_t slot,int32_t channel,int32_t value) {
	WRITE_OP_INT32_T_INT32_T_INT32_T_TIM(OP_SETHVONCHANNEL,slot,channel,value,0);
} 
int ChaosHETControllerInterface::SetHighThreshold(int32_t side,int32_t channel,int32_t thrMillivolt) {
	WRITE_OP_INT32_T_INT32_T_INT32_T_TIM(OP_SETHIGHTHRESHOLD,side,channel,thrMillivolt,0);
} 
int ChaosHETControllerInterface::SetLowThreshold(int32_t side,int32_t channel,int32_t thrMillivolt) {
	WRITE_OP_INT32_T_INT32_T_INT32_T_TIM(OP_SETLOWTHRESHOLD,side,channel,thrMillivolt,0);
} 
int ChaosHETControllerInterface::SwitchPulse(int32_t side,int32_t value) {
	WRITE_OP_INT32_T_INT32_T_TIM(OP_SWITCHPULSE,side,value,0);
} 
