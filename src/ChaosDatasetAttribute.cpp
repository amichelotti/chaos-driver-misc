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
using namespace driver::misc;

std::map< std::string,ChaosDatasetAttribute::datinfo* > ChaosDatasetAttribute::paramToDataset;
std::map< std::string,ChaosDatasetAttribute::ctrl_t > ChaosDatasetAttribute::controllers;
ChaosDatasetAttribute::ChaosDatasetAttribute(std::string path,uint32_t timeo_) {
    std::string cu =path;
    timeo=timeo_;
    if(cu.find_last_of(chaos::PATH_SEPARATOR)==0){
        throw chaos::CException(-1, "bad attribute description",__FUNCTION__);
    }
    cu.erase(cu.find_last_of(chaos::PATH_SEPARATOR),cu.size());
     ATTRDBG_ << "CU NAME:\""<<cu<<"\"";
    
    attr_path=path;
    attr_name=path;
    attr_name.erase(0,attr_path.find_last_of(chaos::PATH_SEPARATOR)+1);
    ATTRDBG_ << "ATTR NAME:\""<<attr_name<<"\"";
    attr_parent=cu;
    if(controllers.find(cu)==controllers.end()){
        controller= ctrl_t (chaos::ui::HLDataApi::getInstance()->getControllerForDeviceID(cu, timeo));
        controllers[cu]=controller;
        controller->setupTracking();
        ATTRDBG_ << " Allocating New controller:"<<controller.get();

    } else {
        controller = controllers[cu];
        ATTRDBG_ << " REUsing controller:"<<controller;
    }
    upd_mode=EVERYTIME;
    update_time=0;
    if(controller==NULL){
       throw chaos::CException(-1, "cannot allocate controller for:"+ cu,__FUNCTION__);

    }
    attr_size =0;
    attr_type=(chaos::DataType::DataType)0;
    controller->setRequestTimeWaith(timeo);
    
    controller->getAttributeDescription(attr_name,attr_desc);
                
    if(controller->getDeviceAttributeType(attr_name,attr_type)!=0){
               throw chaos::CException(-1, "cannot retrieve type of:"+ attr_name,__FUNCTION__);

    }
    if(controller->getDeviceAttributeDirection(attr_name,attr_dir)!=0){
                        throw chaos::CException(-1, "cannot retrieve direction of:"+ attr_name,__FUNCTION__);

    }
    paramToDataset.insert(std::make_pair(attr_path,&info));
    if(get(&attr_size)==NULL){
        throw chaos::CException(-1, "cannot fetch variable:"+ path+ " var name:"+attr_name,__FUNCTION__);
    }
    
    ATTRDBG_<<"Retrived "<<attr_path<<" desc:\""<<attr_desc<<"\" "<< " type:"<<attr_type<<" size:"<<attr_size;
 }
ChaosDatasetAttribute::ChaosDatasetAttribute(const ChaosDatasetAttribute& orig) {
}

ChaosDatasetAttribute::~ChaosDatasetAttribute() {
     ATTRDBG_<<" delete attribute:"<<attr_path;
    if(controllers.find(attr_parent)!=controllers.end()){
        ATTRDBG_<<" remove controller:"<<attr_parent;

        controllers.erase(controllers.find(attr_parent));
    }
}

int ChaosDatasetAttribute::set(void* buf, int size){
    return controller->setAttributeToValue(attr_name.c_str(),buf,1,size);
}
void* ChaosDatasetAttribute::get(uint32_t*size){
    void*tmp=NULL;

    if(paramToDataset.count(attr_path)){
        boost::posix_time::ptime pt=boost::posix_time::microsec_clock::local_time();
        uint64_t tget=pt.time_of_day().total_microseconds();
        pt= boost::posix_time::microsec_clock::local_time();
        if(upd_mode==EVERYTIME || ((upd_mode==NOTBEFORE)&& ((tget - paramToDataset[attr_path]->tget)> update_time)) ){
            chaos::common::data::CDataWrapper*tmpw=controller->fetchCurrentDatatasetFromDomain(chaos::ui::DatasetDomainOutput);
            if(tmpw==NULL){
                ATTRERR_<<"cannot retrieve data for:"+attr_path+ " controller:"<<controller; 
                return NULL;
            }
            paramToDataset[attr_path]->tget = tget;
            paramToDataset[attr_path]->data = tmpw;
            controller->getTimeStamp(paramToDataset[attr_path]->tstamp);     
        }
        if(paramToDataset[attr_path]->data){
            tmp =(void*)paramToDataset[attr_path]->data->getRawValuePtr(attr_name);
            attr_size =paramToDataset[attr_path]->data->getValueSize(attr_name);
            if(size){
                *size = attr_size;
            }
        }

        return tmp;
    }
    ATTRERR_<<attr_path<<"  NOT FOUND";
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