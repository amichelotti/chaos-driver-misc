/* 
 * File:   ChaosDatasetAttribute.cpp
 * Author: michelo
 * 
 * Created on September 2, 2015, 5:29 PM
 */

#include "ChaosDatasetAttribute.h"
#include <chaos/common/exception/CException.h>
#include <chaos/ui_toolkit/ChaosUIToolkit.h>
#include <chaos/cu_toolkit/ChaosCUToolkit.h>

std::map< std::string,ChaosDatasetAttributeBase::datinfo* > ChaosDatasetAttributeBase::paramToDataset;

ChaosDatasetAttributeBase::ChaosDatasetAttributeBase(std::string path,uint32_t timeo_) {
    std::string cu =path;
    timeo=timeo_;
    if(cu.find_last_of(pathSeparator)==0){
        throw chaos::CException(-1, "bad attribute description",__FUNCTION__);
    }
    cu.erase(0,cu.find_last_of(pathSeparator));
    if(chaos::ui::ChaosUIToolkit::getInstance()->getServiceState()==chaos::common::utility::service_state_machine::InizializableServiceType::IS_INITIATED){
        ATTRDBG_ << "UI toolkit already initialized";
    } else if(chaos::cu::ChaosCUToolkit::getInstance()->getServiceState()==chaos::common::utility::service_state_machine::StartableServiceType::SS_STARTED){
        ATTRDBG_ << "CU toolkit has started, initializing UI";
        chaos::ui::ChaosUIToolkit::getInstance()->getGlobalConfigurationInstance()->setConfiguration(chaos::cu::ChaosCUToolkit::getInstance()->getGlobalConfigurationInstance()->getConfiguration());
       chaos::ui::ChaosUIToolkit::getInstance()->init(NULL);
       
    }
    
    attr_path=path;
    controller= chaos::ui::HLDataApi::getInstance()->getControllerForDeviceID(cu, timeo);
    upd_mode=EVERYTIME;
    update_time=0;
    if(controller==NULL){
       throw chaos::CException(-1, "cannot allocate controller for:"+ cu,__FUNCTION__);

    }
    controller->setRequestTimeWaith(timeo);
    
    paramToDataset.insert(std::make_pair(path,&info));
    
 }
ChaosDatasetAttributeBase::ChaosDatasetAttributeBase(const ChaosDatasetAttributeBase& orig) {
}

ChaosDatasetAttributeBase::~ChaosDatasetAttributeBase() {
}

void* ChaosDatasetAttributeBase::getAttribute(){
    void*tmp=NULL;
    if(paramToDataset.count(attr_path)){
        boost::posix_time::ptime pt=boost::posix_time::microsec_clock::local_time();
        uint64_t tget=pt.time_of_day().total_microseconds();
        pt= boost::posix_time::microsec_clock::local_time();
        if(upd_mode==EVERYTIME || ((upd_mode==NOTBEFORE)&& ((tget - paramToDataset[attr_path]->tget)> update_time)) ){
            controller->fetchCurrentDeviceValue();
            paramToDataset[attr_path]->tget = tget;
            paramToDataset[attr_path]->data = controller->getCurrentData();
            controller->getTimeStamp(paramToDataset[attr_path]->tstamp);        
        }
        if(paramToDataset[attr_path]->data)
            tmp =(void*)paramToDataset[attr_path]->data->getRawValuePtr(attr_path);

        return tmp;
    }
    return NULL;
}


   
 void ChaosDatasetAttributeBase::setTimeout(uint32_t timeo_ms){
    timeo=timeo_ms;
    controller->setRequestTimeWaith(timeo);
     
 }
 void ChaosDatasetAttributeBase::setUpdateMode(UpdateMode mode,uint64_t ustime){
     upd_mode = mode;
     update_time = ustime;
 }