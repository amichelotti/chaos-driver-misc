/*
 *	SCDataSync
 *	!CHAOS
 *	Created by Andrea Michelotti 13/10/2015
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

#include "SCDataSync.h"
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include "remoteGroupAccessInterface.h"
#include "ChaosDatasetAttribute.h"
using namespace chaos;

using namespace chaos::common::data;
using namespace chaos::common::batch_command;

using namespace chaos::cu::control_manager::slow_command;
using namespace chaos::cu::driver_manager::driver;



#define SCCUAPP LAPP_ << "[SCDataSync - " << getCUID() << "] - "<<__FUNCTION__<<":"
#define SCCULDBG LDBG_ << "[SCDataSync - " << getCUID() << "] - "<<__FUNCTION__<<":"

PUBLISHABLE_CONTROL_UNIT_IMPLEMENTATION(::driver::misc::SCDataSync)

using namespace ::driver::misc;
/*
 Construct a new CU with an identifier
 */
SCDataSync::SCDataSync(const string& _control_unit_id,
														const string& _control_unit_param,
														const ControlUnitDriverList& _control_unit_drivers):
//call base constructor
chaos::cu::control_manager::SCAbstractControlUnit(_control_unit_id,
												  _control_unit_param,
												  _control_unit_drivers){
    
  driver =NULL;
}

/*
 Base destructor
 */
SCDataSync::~SCDataSync() {
  if(driver){
    delete driver;
  }
    
    
}


/*
 Return the default configuration
 */
void SCDataSync::unitDefineActionAndDataset()  {
   driver=new remoteGroupAccessInterface(getAccessoInstanceByIndex(0));
     if((driver == NULL) || (driver->connect()!=0)){
         throw chaos::CException(-1,"cannot connect with driver",__PRETTY_FUNCTION__);
     }
    std::vector<ChaosDatasetAttribute*> ret= driver->getRemoteVariables();
    for(std::vector<ChaosDatasetAttribute*>::iterator i=ret.begin();i!=ret.end();i++){
        SCCULDBG<<"adding \""<<(*i)->getName()<<"\" desc: \""<<(*i)->getDesc()<<"\" type:"<<(*i)->getType()<<" dir:"<<(*i)->getDir()<<" size:"<<(*i)->getSize();
        addAttributeToDataSet((*i)->getName(),"["+(*i)->getPath()+"]"+ (*i)->getDesc() ,(*i)->getType(),(*i)->getDir(),(*i)->getSize());
    }
    
	//install all command
//	installCommand<CmdBPMDefault>("default");
//	installCommand<CmdBPMAcquire>("acquire");
//	installCommand<CmdBPMEnv>("env");
//	installCommand<CmdBPMTime>("time");
	
	//set it has default
//	setDefaultCommand("default");
	
      
	
}

void SCDataSync::unitDefineCustomAttribute() {
	
}

// Abstract method for the initialization of the control unit
void SCDataSync::unitInit() {
    driver->init(1);
}

// Abstract method for the start of the control unit
void SCDataSync::unitStart() {
    driver->start(1);

}

// Abstract method for the stop of the control unit
void SCDataSync::unitStop() {
	 driver->stop(1);
}

// Abstract method for the deinit of the control unit
void SCDataSync::unitDeinit() {
    driver->deinit(1);
	
}
