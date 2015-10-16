/* 
 * File:   remoteGroupAccess.cpp
 * Author: michelo
 * 
 * Created on October 14, 2015, 9:40 AM
 */

#include "remoteGroupAccessInterface.h"
using namespace driver::misc;
remoteGroupAccessInterface::remoteGroupAccessInterface(chaos::cu::driver_manager::driver::DriverAccessor*da):BasicIODriverInterface(da) {
    ctrl_group=0;
    data_group=0;
}




remoteGroupAccessInterface::~remoteGroupAccessInterface() {
}


int remoteGroupAccessInterface::connect(){
    ChaosControllerGroup<ChaosController>* ctrl=NULL;
    ChaosDatasetAttributeGroup*data=NULL;
    if(read((void*)&ctrl,0,sizeof(ChaosControllerGroup<ChaosController>*))>0){
        ctrl_group = ctrl;
        
    } else {
        return -1;
    }
    if(read((void*)&data,1,sizeof(ChaosDatasetAttributeGroup*))>0){
        data_group = data;
    } else {
        return -2;
    }
    return 0;
}
  
    int  remoteGroupAccessInterface::init(int force){
        if(ctrl_group==0){
            CTRLERRG_<< "Control disconnected";
            return -2;
        }
        return ctrl_group->init(force);
    }
   
    int  remoteGroupAccessInterface::stop(int force){
         if(ctrl_group==0){
            CTRLERRG_<< "Control disconnected";
            return -2;
        }
        return ctrl_group->stop(force);
    
    }
   
    int remoteGroupAccessInterface::start(int force){
         if(ctrl_group==0){
            CTRLERRG_<< "Control disconnected";
            return -2;
        }
        return ctrl_group->start(force);
    }
    
    int remoteGroupAccessInterface::deinit(int force){
         if(ctrl_group==0){
            CTRLERRG_<< "Control disconnected";
            return -2;
        }
        return ctrl_group->deinit(force);
    
    }
   
    int remoteGroupAccessInterface::setSchedule(uint64_t us){
        if(ctrl_group==0){
            CTRLERRG_<< "Control disconnected";
            return -2;
        }
        
        return ctrl_group->setSchedule(us);
    
    }
    
    void remoteGroupAccessInterface::setTimeout(uint64_t timeo_us){
        if(ctrl_group==0){
            CTRLERRG_<< "Control disconnected";
            return ;
        }
        
        ctrl_group->setTimeout(timeo_us);
    
    }
  
    int remoteGroupAccessInterface::broadcastCmd(std::string alias,chaos::common::data::CDataWrapper*params){
           ChaosController::command_t c = ctrl_group->prepareCommand(alias);
           c->param.appendAllElement(*params);
           return ctrl_group->executeCmd(c,true);
    }
    
   
    
    std::vector<ChaosDatasetAttribute*> remoteGroupAccessInterface::getRemoteVariables(){
        return data_group->getAttributes();
    }
    
    std::vector<ChaosDatasetAttribute*> remoteGroupAccessInterface::getRemoteVariables(std::string name){
        return data_group->getAttrsByName(name);
    }
    
   
    ChaosDatasetAttribute* remoteGroupAccessInterface::getRemoteVariable(std::string path){
        return data_group->getAttr(path);
    }