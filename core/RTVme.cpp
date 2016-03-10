/*
 *	RTVme.cpp
 *	!CHAOS
 *	Andrea Michelotti
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

#include "RTVme.h"
#include <stdlib.h>
#include <boost/algorithm/string.hpp>
#include <chaos/ui_toolkit/LowLevelApi/LLRpcApi.h>
#include <chaos/cu_toolkit/command_manager/CommandManager.h>
#include <common/debug/core/debug.h>

#include <chaos/common/utility/TimingUtil.h>
using namespace chaos;
using namespace chaos::common::data::cache;
using namespace chaos::common::utility;
using namespace chaos::cu::driver_manager::driver;
using namespace ::driver::misc;
PUBLISHABLE_CONTROL_UNIT_IMPLEMENTATION(RTVme)

#define RTVmeLAPP_		LAPP_ << "[RTDataSync] "
#define RTVmeLDBG_		LDBG_ << "[RTDataSync] " << __PRETTY_FUNCTION__ << " "
#define RTVmeLERR_		LERR_ << "[RTDataSync] " << __PRETTY_FUNCTION__ << "("<<__LINE__<<") "



/*
 Construct
 */
RTVme::RTVme(const string& _control_unit_id,
                        const string& _control_unit_param,
                        const ControlUnitDriverList& _control_unit_drivers):
RTAbstractControlUnit(_control_unit_id,
                        _control_unit_param,
                        _control_unit_drivers) {
    int cnt=0;
    std::vector<std::string>::iterator i;
    std::vector<std::string> soff;
    vme=NULL;
    boost::split(soff,_control_unit_param,boost::is_any_of(" \n"));
    i=soff.begin();
    for(cnt=0;cnt<soff.size();cnt++){
    	if(i->size()){
         DPRINT("[%d] Adding Offset \"%s\"",cnt,i->c_str());
         vme_offs.push_back(strtoll(i->c_str(),0,0));
    	}
         i++;
     }
}
void RTVme::unitDefineActionAndDataset() throw(chaos::CException) {
    //insert your definition code here
    addAttributeToDataSet("VME_BASE",
                        "Vme Base address",
                        DataType::TYPE_INT64,
                        DataType::Input);
    addAttributeToDataSet("VME_SIZE",
                        "Vme map size",
                        DataType::TYPE_INT32,
                        DataType::Input);

    addAttributeToDataSet("VME_ADDRESSING",
                        "VME addressing (16/24/32/64)",
                        DataType::TYPE_INT32,
                        DataType::Input);

    addAttributeToDataSet("VME_DW",
                        "VME data width access (8/16/32)",
                        DataType::TYPE_INT32,
                        DataType::Input);

    addAttributeToDataSet("VME_MASTER",
                        "VME Master (true)/Slave(false)",
                        DataType::TYPE_BOOLEAN,
                        DataType::Input);

    addAttributeToDataSet("VME_OPTIONS",
                        "VME Options, irq=0x1, dma=0x2, supervisor=0x4",
                        DataType::TYPE_INT32,
                        DataType::Input);

    std::vector<uint64_t>::iterator i;
    for(i=vme_offs.begin();i!=vme_offs.end();i++){
      std::stringstream ss;
      ss<<"OFF["<<hex<<*i<<"]";
      addAttributeToDataSet(ss.str(),
			    "VME offset to monitor",
			    DataType::TYPE_INT32,
			    DataType::Output);
    }
   
}


 void RTVme::unitInit() throw(chaos::CException){
	 vme_base_address = *getAttributeCache()->getROPtr<uint64_t>(DOMAIN_INPUT, "VME_BASE");
	 vme_base_size = *getAttributeCache()->getROPtr<uint32_t>(DOMAIN_INPUT, "VME_SIZE");
	 vme_addressing=*getAttributeCache()->getROPtr<uint32_t>(DOMAIN_INPUT, "VME_ADDRESSING");
	 vme_data_access=*getAttributeCache()->getROPtr<uint32_t>(DOMAIN_INPUT, "VME_DW");
	 vme_master=*getAttributeCache()->getROPtr<bool>(DOMAIN_INPUT, "VME_MASTER");
	 vme_options=*getAttributeCache()->getROPtr<uint32_t>(DOMAIN_INPUT, "VME_OPTIONS");
	 if(vme_master){
		 vme=vmewrap_vme_open_master(vme_base_address,vme_base_size,(vme_addressing_t)vme_addressing,(vme_access_t)vme_data_access,(vme_opt_t)0);
	 } else {
		 vme=vmewrap_vme_open_slave(vme_base_address,vme_base_size,(vme_addressing_t)vme_addressing,(vme_access_t)vme_data_access,(vme_opt_t)0);
	 }
	 if(vme==NULL){
		 throw CException(-1,__PRETTY_FUNCTION__,"cannot initialize VME");
	 }
 }
 
 void RTVme::unitStart() throw(chaos::CException){
	 int cnt;
	 for(cnt=0;cnt<vme_offs.size();cnt++){
		 uint32_t data=0;
		 vmewrap_read32(vme,vme_offs[cnt],&data);
		 getAttributeCache()->setOutputAttributeValue(cnt,&data,sizeof(data));
	 }
	if(vme_offs.size()){
		getAttributeCache()->setOutputDomainAsChanged();
	}

 }
 void RTVme::unitStop() throw(chaos::CException){

 }
 void RTVme::unitDeinit() throw(chaos::CException){
	 if(vme){
		 vmewrap_vme_close(vme);
		 vme=NULL;
	 }
 }
 void RTVme::unitRun() throw(chaos::CException){}
 
/*
 Destructor
 */
RTVme::~RTVme() {
	unitDeinit();

}
