/* 
 * File:   remoteGroupAccessDriver.cpp
 * Author: michelo
 * 
 * Created on October 14, 2015, 10:00 AM
 */

#include "remoteGroupAccessDriver.h"
//#include <boost/algorithm/string.hpp>
#include <chaos/common/ChaosCommon.h>
using namespace ::driver::misc;
OPEN_CU_DRIVER_PLUGIN_CLASS_DEFINITION(remoteGroupAccessDriver, 1.0.0, ::driver::misc::remoteGroupAccessDriver)

REGISTER_CU_DRIVER_PLUGIN_CLASS_INIT_ATTRIBUTE(::driver::misc::remoteGroupAccessDriver, http_address / dnsname : port)
CLOSE_CU_DRIVER_PLUGIN_CLASS_DEFINITION

#define CTRLDBG_ DBG_LOG(remoteGroupAccessDriver) 
#define CTRLERR_ ERR_LOG(remoteGroupAccessDriver)

remoteGroupAccessDriver::remoteGroupAccessDriver() {
    group = NULL;
    data_group=NULL;
}

remoteGroupAccessDriver::~remoteGroupAccessDriver() {
    deinitIO();
    
}

int remoteGroupAccessDriver::read(void *buffer, int addr, int bcount){
    if(buffer==NULL){
        CTRLERR_ <<"invalid buffer given";

    	return -1;
    }
    CTRLDBG_<<"reading addr "<<addr<< "buffer size"<<bcount<<" group "<<group <<" data_group "<<data_group;
    
    if((addr==0) && group){
        memcpy(buffer,(void*)&group,sizeof(ChaosControllerGroup<ChaosController>*));
        return 1;
    }
    if((addr==1) && data_group){
        memcpy(buffer,(void*)&data_group,sizeof(ChaosDatasetAttributeGroup*));
        return 1;
    }
    CTRLERR_ <<"error reading";
    return 0;
}
int remoteGroupAccessDriver::initIO(void *buffer, int sizeb){
    std::string ctrl_vars;
    std::vector<std::string> vars;
    std::vector<std::string>::iterator i;
    if(buffer==NULL){
         CTRLERR_ <<" no parameters given to the driver, expected list of remote datasets";
        return -1;
    }
    
   
   ctrl_vars.assign((const char*)buffer); 
    
   //boost::split(vars,ctrl_vars,boost::is_any_of(" \n"));
   vars=chaos::split(ctrl_vars," \n");
   if(vars.size()==0){
       CTRLERR_ <<" no vars to connect";
       return -2;
   }
   
   CTRLDBG_<<" "<<vars.size()<<" Synchronizing:"<<ctrl_vars;
   group=new ChaosControllerGroup<ChaosController>();
   data_group=new ChaosDatasetAttributeGroup();
   if(group==NULL || data_group==NULL){
       CTRLERR_ <<" cannot create resources";
       return -10;
   }
    for( std::vector<std::string>::iterator i=vars.begin();i!=vars.end();i++){
        if(!(*i).empty()){
            CTRLDBG_<<" Adding "<<*i<<" to the set";
            ChaosDatasetAttribute *ret=data_group->add(*i);
            if(ret){
                group->add(ret->getParent());
            }
        }
      
    }
   group->setTimeout(10000000);
   data_group->setTimeout(10000000);
	CTRLDBG_<<"DONE";

   return 0;
}
int remoteGroupAccessDriver::deinitIO(){
	CTRLDBG_<<"DEINITIALIZING";
    if(group)
        delete group;
    group=NULL;
    if(data_group)
        delete data_group;
    
    data_group = NULL;
    return 0;
}
