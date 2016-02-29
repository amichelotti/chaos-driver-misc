/*
 *	RTVme.h
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
#ifndef _RTVme_h
#define _RTVme_h

#include <chaos/cu_toolkit/control_manager/RTAbstractControlUnit.h>
#include <common/vme/core/vmewrap.h>

    namespace driver {

        namespace misc {

	  class RTVme : public chaos::cu::control_manager::RTAbstractControlUnit {
	PUBLISHABLE_CONTROL_UNIT_INTERFACE(RTVme)
	  public:
    /*!
     Construct a new CU with full constructor
     */
	    RTVme(const std::string& _control_unit_id, const std::string& _control_unit_param, const ControlUnitDriverList& _control_unit_drivers);
	    /*!
     Destructor a new CU
     */
    ~RTVme();

protected:
    uint64_t vme_base_address;
    
    uint32_t vme_base_size,vme_addressing,vme_data_access;
    uint32_t vme_master;
    uint32_t vme_options;
    vmewrap_vme_handle_t vme;
    std::vector<uint64_t> vme_offs;
public:
    void unitDefineActionAndDataset() throw(chaos::CException);
    void unitInit() throw(chaos::CException);

    void unitStart() throw(chaos::CException);
    void unitStop() throw(chaos::CException);
    void unitDeinit() throw(chaos::CException);
    void unitRun() throw(chaos::CException);

};
            }}
#endif
