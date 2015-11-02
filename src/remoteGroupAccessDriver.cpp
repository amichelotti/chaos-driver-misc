/* 
 * File:   remoteGroupAccessDriver.cpp
 * Author: michelo
 * 
 * Created on October 14, 2015, 10:00 AM
 */

#include "remoteGroupAccessDriver.h"
#include <boost/algorithm/string.hpp>
using namespace ::driver::misc;
OPEN_CU_DRIVER_PLUGIN_CLASS_DEFINITION(remoteGroupAccessDriver, 1.0.0, remoteGroupAccessDriver)
REGISTER_CU_DRIVER_PLUGIN_CLASS_INIT_ATTRIBUTE(remoteGroupAccessDriver, http_address / dnsname : port)
CLOSE_CU_DRIVER_PLUGIN_CLASS_DEFINITION

remoteGroupAccessDriver::remoteGroupAccessDriver() {
    group = NULL;
    data_group=NULL;
}

remoteGroupAccessDriver::~remoteGroupAccessDriver() {
    deinitIO();
    
}

int remoteGroupAccessDriver::read(void *buffer, int addr, int bcount){
    if(buffer==NULL)
        return -1;
    
    if((addr==0) && group){
        memcpy(buffer,(void*)&group,sizeof(ChaosControllerGroup<ChaosController>*));
        return 1;
    }
    if((addr==1) && data_group){
        memcpy(buffer,(void*)&data_group,sizeof(ChaosDatasetAttributeGroup*));
        return 1;
    }
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
    
   boost::split(vars,ctrl_vars,boost::is_any_of(" \n"));
   if(vars.size()==0){
       CTRLERR_ <<" no vars to connect";
       return -2;
   }
   
   CTRLDBG_<<" "<<vars.size()<<" Synchronizing:"<<ctrl_vars;
   group=new ChaosControllerGroup<ChaosController>();
   data_group=new ChaosDatasetAttributeGroup();
   if(group==NULL || data_group==NULL){
       CTRLERR_ <<" cannot create resources";
   }
    for( std::vector<std::string>::iterator i=vars.begin();i!=vars.end();i++){
        CTRLDBG_<<" Adding "<<*i<<" to the set";
        ChaosDatasetAttribute *ret=data_group->add(*i);
        if(ret){
            group->add(ret->getParent());
        }
      
    }
   group->setTimeout(10000000);
   data_group->setTimeout(10000000);
}
int remoteGroupAccessDriver::deinitIO(){
    if(group)
        delete group;
    group=NULL;
    if(data_group)
        delete data_group;
    
    data_group = NULL;
}
