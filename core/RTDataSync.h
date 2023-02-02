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
#include "ChaosDatasetAttribute.h"
#include "ChaosDatasetAttributeSinchronizer.h"


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
        ChaosDatasetAttribute** rem_data;
        int rem_variables;
        ChaosDatasetAttributeSinchronizer* data_group;
        std::vector<std::string> cu_names;
public:
    void unitDefineActionAndDataset() ;
    void unitInit() {}

    void unitStart() {}
    void unitStop() {}
	void unitDeinit() {}
	void unitRun() {}

};
            }}
#endif
