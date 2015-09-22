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

std::map< std::string,ChaosDatasetAttribute::datinfo* > ChaosDatasetAttribute::paramToDataset;

ChaosDatasetAttribute::ChaosDatasetAttribute(const char* path,uint32_t timeo_) {
    std::string cu =path;
    timeo=timeo_;
    if(cu.find_last_of(pathSeparator)==0){
        throw chaos::CException(-1, "bad attribute description",__FUNCTION__);
    }
    cu.erase(cu.find_last_of(pathSeparator),cu.size());
     ATTRDBG_ << "CU NAME:\""<<cu<<"\"";
    if(chaos::ui::ChaosUIToolkit::getInstance()->getServiceState()==chaos::common::utility::service_state_machine::InizializableServiceType::IS_INITIATED){
        ATTRDBG_ << "UI toolkit already initialized";
    } else if(chaos::cu::ChaosCUToolkit::getInstance()->getServiceState()==chaos::common::utility::service_state_machine::StartableServiceType::SS_STARTED){
        ATTRDBG_ << "CU toolkit has started, initializing UI";
        chaos::ui::ChaosUIToolkit::getInstance()->getGlobalConfigurationInstance()->setConfiguration(chaos::cu::ChaosCUToolkit::getInstance()->getGlobalConfigurationInstance()->getConfiguration());
       chaos::ui::ChaosUIToolkit::getInstance()->init(NULL);
       
    }
    
    attr_path=path;
    attr_name=path;
    attr_name.erase(0,attr_path.find_last_of(pathSeparator)+1);
    ATTRDBG_ << "ATTR NAME:\""<<attr_name<<"\"";

    controller= chaos::ui::HLDataApi::getInstance()->getControllerForDeviceID(cu, timeo);
    upd_mode=EVERYTIME;
    update_time=0;
    if(controller==NULL){
       throw chaos::CException(-1, "cannot allocate controller for:"+ cu,__FUNCTION__);

    }
    controller->setRequestTimeWaith(timeo);
    
    paramToDataset.insert(std::make_pair(attr_path,&info));
    
 }
ChaosDatasetAttribute::ChaosDatasetAttribute(const ChaosDatasetAttribute& orig) {
}

ChaosDatasetAttribute::~ChaosDatasetAttribute() {
}

void* ChaosDatasetAttribute::get(uint32_t*size){
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
        if(paramToDataset[attr_path]->data){
            tmp =(void*)paramToDataset[attr_path]->data->getRawValuePtr(attr_name);
            if(size){
                *size = paramToDataset[attr_path]->data->getValueSize(attr_name);
            }
        }

        return tmp;
    }
    return NULL;
}


  ChaosDatasetAttribute::datinfo& ChaosDatasetAttribute::getInfo(){
      return *paramToDataset[attr_path];
  }

 void ChaosDatasetAttribute::setTimeout(uint64_t timeo_ms){
    timeo=timeo_ms;
    controller->setRequestTimeWaith(timeo);
     
 }
 void ChaosDatasetAttribute::setUpdateMode(UpdateMode mode,uint64_t ustime){
     upd_mode = mode;
     update_time = ustime;
 }
 
  ChaosDatasetAttribute::operator int32_t()  {
      return *reinterpret_cast<int32_t*>(get(NULL));
 }
 
 ChaosDatasetAttribute::operator int64_t(){
           return *reinterpret_cast<int64_t*>(get(NULL));

 }
ChaosDatasetAttribute::operator double (){
           return *reinterpret_cast<double*>(get(NULL));

 }
