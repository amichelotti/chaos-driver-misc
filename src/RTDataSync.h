/*
 *	RTDataSync.h
 *	!CHAOS
 *	Created by Andrea Michelotti
 *      Collects and align a given series of Libera BPMs
 *    	Copyright 2015 INFN, National Institute of Nuclear Physics
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
#ifndef _RTDataSync_h
#define _RTDataSync_h

#include <chaos/cu_toolkit/control_manager/RTAbstractControlUnit.h>
#include <driver/misc/ChaosControllerGroup.h>
#include <driver/misc/ChaosDatasetAttribute.h>
#include <driver/misc/ChaosDatasetAttributeSinchronizer.h>
#include <driver/daq/models/Libera/ChaosControllerLibera.h>

   
    namespace driver {
        
        namespace misc {
            
class RTDataSync : public chaos::cu::control_manager::RTAbstractControlUnit {
	PUBLISHABLE_CONTROL_UNIT_INTERFACE(RTDataSync)
public:
    /*!
     Construct a new CU with full constructor
     */
    RTDataSync(const std::string& _control_unit_id, const std::string& _control_unit_param, const ControlUnitDriverList& _control_unit_drivers);
    /*!
     Destructor a new CU
     */
    ~RTDataSync();

protected:
        driver::misc::ChaosDatasetAttribute** rem_data;
        int rem_variables;
        driver::misc::ChaosDatasetAttributeSinchronizer* data_group;
        std::vector<std::string> cu_names;
public:
    void unitDefineActionAndDataset() throw(chaos::CException);
    void unitInit() throw(chaos::CException){}
		
    void unitStart() throw(chaos::CException){}
    void unitStop() throw(chaos::CException){}
			void unitDeinit() throw(chaos::CException){}
			void unitRun() throw(chaos::CException){}

};
            }}
#endif
