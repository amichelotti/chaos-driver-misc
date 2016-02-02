/*
 *	RTDataSync.cpp
 *	!CHAOS
 *	Created automatically
 *
 *    	Copyright 2012 INFN, National Institute of Nuclear Physics
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

#include "RTDataSync.h"
//#include "RTDataSyncDriver.h"
#include <boost/algorithm/string.hpp>
#include <chaos/ui_toolkit/LowLevelApi/LLRpcApi.h>
#include <chaos/cu_toolkit/command_manager/CommandManager.h>
#include <common/debug/core/debug.h>
#include <chaos/common/utility/TimingUtil.h>
using namespace chaos;
using namespace chaos::common::data::cache;
using namespace chaos::common::utility;
using namespace chaos::cu::driver_manager::driver;
using namespace driver::misc;
PUBLISHABLE_CONTROL_UNIT_IMPLEMENTATION(RTDataSync)

#define RTDataSyncLAPP_		LAPP_ << "[RTDataSync] "
#define RTDataSyncLDBG_		LDBG_ << "[RTDataSync] " << __PRETTY_FUNCTION__ << " "
#define RTDataSyncLERR_		LERR_ << "[RTDataSync] " << __PRETTY_FUNCTION__ << "("<<__LINE__<<") "



/*
 Construct
 */
RTDataSync::RTDataSync(const string& _control_unit_id,
                        const string& _control_unit_param,
                        const ControlUnitDriverList& _control_unit_drivers):
RTAbstractControlUnit(_control_unit_id,
                        _control_unit_param,
                        _control_unit_drivers) {
    int cnt=0;
    std::vector<std::string>::iterator i;
   rem_variables=0;
     boost::split(cu_names,_control_unit_param,boost::is_any_of(" \n"));
     rem_variables=cu_names.size();
     DPRINT("Variables :%d",rem_variables);
     if(rem_variables>0){
         rem_data = new driver::misc::ChaosDatasetAttribute*[rem_variables];
     } else {
         return;
     }

     data_group=new ChaosDatasetAttributeSinchronizer();
    if(data_group==NULL){
        throw chaos::CException(-100,"## cannot create group access",__PRETTY_FUNCTION__);

    }
     i=cu_names.begin();

     for(cnt=0;cnt<rem_variables;cnt++){
         DPRINT("[%d] Adding Remote Variable \"%s\"",cnt,i->c_str());
         rem_data[cnt] = new ChaosDatasetAttribute(*i);
         data_group->add(rem_data[cnt] );
         i++;
     }
     
   
    data_group->setInterval(3000000);
    data_group->setTimeout (6000000);
  
    
}
void RTDataSync::unitDefineActionAndDataset() throw(chaos::CException) {
    //insert your definition code here
     std::vector<ChaosDatasetAttribute*> rattrs=data_group->getAttributes();
    for (std::vector<ChaosDatasetAttribute*>::iterator i=rattrs.begin();i!=rattrs.end();i++){
        std::string name=(*i)->getName();
        DPRINT("dynamic adding attribute: %s size :%d dir: %d type:%d",name.c_str(),(*i)->getSize(),(*i)->getDir(),(*i)->getType());
        if((*i)->getType()!=chaos::DataType::TYPE_BYTEARRAY){
            addAttributeToDataSet(name,(*i)->getDesc(),(*i)->getType(),(*i)->getDir());
        } else if((*i)->getBinaryType()!=chaos::DataType::SUB_TYPE_NONE){
            addBinaryAttributeAsSubtypeToDataSet(name,(*i)->getDesc(),(*i)->getBinaryType(),(*i)->getSize(),(*i)->getDir());
        }
    }
   
}

/*
 Destructor
 */
RTDataSync::~RTDataSync() {

}
