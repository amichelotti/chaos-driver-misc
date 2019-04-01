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

#define READ_OP_VECTOR_INT32_T__TIM(op,VAR_vector_int32_t_E1,timeout)\
PREPARE_OP_RET_INT_TIMEOUT(op,timeout); \
accessor->send(&message);\
VAR_vector_int32_t_E1=ret.vector_int32_t_E1;\
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
int ChaosHETPicInterface::getNumberOfChannel(int32_t& chanNum) {
	READ_OP_INT32_T_TIM(OP_GETNUMBEROFCHANNEL,chanNum,0);
} 
int ChaosHETPicInterface::getStatus(int32_t& status) {
	READ_OP_INT32_T_TIM(OP_GETSTATUS,status,0);
} 
int ChaosHETPicInterface::getHighThresholds(std::vector<int32_t>& highThresholds) {
	READ_OP_VECTOR_INT32_T__TIM(OP_GETHIGHTHRESHOLDS,highThresholds,0);
} 
int ChaosHETPicInterface::getLowThresholds(std::vector<int32_t>& lowThresholds) {
	READ_OP_VECTOR_INT32_T__TIM(OP_GETLOWTHRESHOLDS,lowThresholds,0);
} 
