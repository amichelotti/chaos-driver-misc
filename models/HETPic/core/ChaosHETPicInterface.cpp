/*
ChaosHETPicInterface.cpp
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
#include "ChaosHETPicInterface.h"
using namespace chaos::driver::hetpic;
#define PREPARE_OP_RET_INT_TIMEOUT(op,tim) \
hetpic_oparams_t ret;\
hetpic_iparams_t idata;\
message.opcode = op; \
message.inputData=(void*)&idata;\
idata.timeout=tim;\
message.inputDataLength=sizeof(hetpic_iparams_t);\
message.resultDataLength=sizeof(hetpic_oparams_t);\
message.resultData = (void*)&ret;\

#define WRITE_OP_TIM(op,timeout) \
PREPARE_OP_RET_INT_TIMEOUT(op,timeout); \
accessor->send(&message);\
return ret.result;

#define WRITE_OP_INT32_T_INT32_T_TIM(op,VAR_int32_t1,VAR_int32_t2,timeout)\
PREPARE_OP_RET_INT_TIMEOUT(op,timeout); \
idata.int32_t1=VAR_int32_t1;\
idata.int32_t2=VAR_int32_t2;\
accessor->send(&message);\
return ret.result;

#define WRITE_OP_INT32_T_TIM(op,VAR_int32_t1,timeout)\
PREPARE_OP_RET_INT_TIMEOUT(op,timeout); \
idata.int32_t1=VAR_int32_t1;\
accessor->send(&message);\
return ret.result;

#define READ_OP_INT32_T_TIM(op,VAR_int32_tE1,timeout)\
PREPARE_OP_RET_INT_TIMEOUT(op,timeout); \
accessor->send(&message);\
VAR_int32_tE1=ret.int32_tE1;\
return ret.result;

#define READ_OP_DOUBLE_DOUBLE_TIM(op,VAR_doubleE1,VAR_doubleE2,timeout)\
PREPARE_OP_RET_INT_TIMEOUT(op,timeout); \
accessor->send(&message);\
VAR_doubleE1=ret.doubleE1;\
VAR_doubleE2=ret.doubleE2;\
return ret.result;

int ChaosHETPicInterface::SetHighThreshold(int32_t channel,int32_t millivolts) {
	WRITE_OP_INT32_T_INT32_T_TIM(OP_SETHIGHTHRESHOLD,channel,millivolts,0);
} 
int ChaosHETPicInterface::SetLowThreshold(int32_t channel,int32_t millivolts) {
	WRITE_OP_INT32_T_INT32_T_TIM(OP_SETLOWTHRESHOLD,channel,millivolts,0);
} 
int ChaosHETPicInterface::setPulse(int32_t value) {
	WRITE_OP_INT32_T_TIM(OP_SETPULSE,value,0);
} 
int ChaosHETPicInterface::getLowThresholds(int32_t& lowthresholds) {
	READ_OP_INT32_T_TIM(OP_GETLOWTHRESHOLDS,lowthresholds,0);
} 
int ChaosHETPicInterface::getHighThresholds(int32_t& highthresholds) {
	READ_OP_INT32_T_TIM(OP_GETHIGHTHRESHOLDS,highthresholds,0);
} 
int ChaosHETPicInterface::getTemperatures(double& temperature_0,double& temperature_1) {
	READ_OP_DOUBLE_DOUBLE_TIM(OP_GETTEMPERATURES,temperature_0,temperature_1,0);
} 
