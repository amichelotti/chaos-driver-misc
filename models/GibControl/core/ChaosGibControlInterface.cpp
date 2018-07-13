/*
ChaosGibControlInterface.cpp
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
#include "ChaosGibControlInterface.h"
using namespace chaos::driver::gibcontrol;
#define PREPARE_OP_RET_INT_TIMEOUT(op,tim) \
gibcontrol_oparams_t ret;\
gibcontrol_iparams_t idata;\
message.opcode = op; \
message.inputData=(void*)&idata;\
idata.timeout=tim;\
message.inputDataLength=sizeof(gibcontrol_iparams_t);\
message.resultDataLength=sizeof(gibcontrol_oparams_t);\
message.resultData = (void*)&ret;\

#define WRITE_OP_TIM(op,timeout) \
PREPARE_OP_RET_INT_TIMEOUT(op,timeout); \
accessor->send(&message);\
return ret.result;

#define WRITE_OP_INT32_T_INT32_T_INT32_T_INT32_T_TIM(op,VAR_int32_t1,VAR_int32_t2,VAR_int32_t3,VAR_int32_t4,timeout)\
PREPARE_OP_RET_INT_TIMEOUT(op,timeout); \
accessor->send(&message);\
idata.int32_t1=VAR_int32_t1;\
idata.int32_t2=VAR_int32_t2;\
idata.int32_t3=VAR_int32_t3;\
idata.int32_t4=VAR_int32_t4;\
return ret.result;

#define WRITE_OP_INT32_T_DOUBLE_TIM(op,VAR_int32_t1,VAR_double1,timeout)\
PREPARE_OP_RET_INT_TIMEOUT(op,timeout); \
accessor->send(&message);\
idata.int32_t1=VAR_int32_t1;\
idata.double1=VAR_double1;\
return ret.result;

int ChaosGibControlInterface::setPulse(int32_t channel,int32_t amplitude,int32_t width,int32_t state) {
	WRITE_OP_INT32_T_INT32_T_INT32_T_INT32_T_TIM(OP_SETPULSE,channel,amplitude,width,state,0);
} 
int ChaosGibControlInterface::setChannelVoltage(int32_t channel,double Voltage) {
	WRITE_OP_INT32_T_DOUBLE_TIM(OP_SETCHANNELVOLTAGE,channel,Voltage,0);
} 
int ChaosGibControlInterface::setThrVoltage(int32_t channel,double thrVoltage) {
	WRITE_OP_INT32_T_DOUBLE_TIM(OP_SETTHRVOLTAGE,channel,thrVoltage,0);
} 