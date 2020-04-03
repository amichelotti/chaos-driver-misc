/*
 *	CmdSync.cpp
 *	!CHAOS
 *	Created by Andrea Michelotti
 *
 *    	Copyright 2013 INFN, National Institute of Nuclear Physics
 *
 *    	Licensed under the Apache License, Version 2.0 (the "License");
 *    	you may not use this file except in compliance with the License.
 *    	You may obtain a copy of the License at
 *
 *    	http://www.apache.org/licenses/LICENSE-2.0
 *
 *    	Unless required by applicable law or agreed to in writing, software
 *    	distributed under the License is distributed on an "AS IS" BASIS,
 *    	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    	See the License for the specific language governing permissions and
 *    	limitations under the License.
 */

#include <string.h>
#include "CmdSync.h"



namespace chaos_batch = chaos::common::batch_command;
using namespace chaos::common::data;
using namespace chaos::common::batch_command;
using namespace chaos::cu::control_manager::slow_command;
using namespace ::driver::misc;
#define CTRLDBG_ DBG_LOG(CmdSync) 
#define CTRLERR_ ERR_LOG(CmdSync)

CmdSync::CmdSync() {
     
    driver=0;
   
}

CmdSync::~CmdSync() {
    if(driver)
        delete driver;
    driver = 0;
}
 
void  CmdSync::setHandler(c_data::CDataWrapper *data){
    chaos::cu::driver_manager::driver::DriverAccessor * accessor=driverAccessorsErogator->getAccessoInstanceByIndex(0);
    CTRLDBG_<<" ["<<getAlias()<<"]";
    if(accessor==NULL){
          throw chaos::CException(-1,"no driver available",__PRETTY_FUNCTION__);

    }
     driver=new remoteGroupAccessInterface(accessor);
     if((driver == NULL) || (driver->connect()!=0)){
         throw chaos::CException(-1,"cannot connect remote resources",__PRETTY_FUNCTION__);
     }
     if(data){
     CTRLDBG_<<" ["<<getAlias()<<"]" <<"CMD:"<<data->getJSONString();
     driver->broadcastCmd(getAlias(),data);
     }
}

uint8_t CmdSync::implementedHandler(){
       return chaos_batch::HandlerType::HT_Set  ;
}
