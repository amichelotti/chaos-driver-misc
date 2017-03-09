/* 
 * File:   ChaosDatasetAttribute.cpp
 * Author: michelo
 * 
 * Created on September 2, 2015, 5:29 PM
 */

#include "ChaosDatasetAttribute.h"
#include <chaos/common/exception/CException.h>
#include <chaos/cu_toolkit/ChaosCUToolkit.h>

#ifdef __CHAOS_UI__
#include <chaos/ui_toolkit/ChaosUIToolkit.h>
#else
#include <ChaosMetadataServiceClient/ChaosMetadataServiceClient.h>

#endif
using namespace ::driver::misc;

std::map< std::string,ChaosDatasetAttribute::datinfo_psh > ChaosDatasetAttribute::paramToDataset;
std::map< std::string,ChaosDatasetAttribute::ctrl_t > ChaosDatasetAttribute::controllers;

std::string ChaosDatasetAttribute::getGroup(){
    std::string res=attr_parent;
    res.erase(0,attr_parent.find_last_of(chaos::PATH_SEPARATOR)+1);
    return res;
}
 int ChaosDatasetAttribute::initialize_framework=0;

int ChaosDatasetAttribute::allocateController(std::string cu){
	if(!initialize_framework){
		ATTRDBG_<<" Initializing ChaosMetadataServiceClient Framework ...";
		chaos::metadata_service_client::ChaosMetadataServiceClient::getInstance()->init();
		chaos::metadata_service_client::ChaosMetadataServiceClient::getInstance()->start();
        chaos::metadata_service_client::ChaosMetadataServiceClient::getInstance()->enableMonitor();

		initialize_framework++;
		ATTRDBG_<<" END ChaosMetadataServiceClient Framework ...";

	}
	try{
	  if(controllers.find(cu)==controllers.end()){
#ifdef __CHAOS_UI__
	        controller= ctrl_t (chaos::ui::HLDataApi::getInstance()->getControllerForDeviceID(cu, timeo));
	        controllers[cu]=controller;
#else
	        chaos::metadata_service_client::node_controller::CUController *cu_ctrl = NULL;
	        chaos::metadata_service_client::ChaosMetadataServiceClient::getInstance()->getNewCUController(cu,&cu_ctrl);

		controller=cu_ctrl;
	        controllers[cu]=controller;

#endif
	        controller->setupTracking();
	        ATTRDBG_ << " Allocating New controller:"<<controller;

	    } else {
	    	//CDataWrapper data;
	    	//data.setSerializedJsonData("{\"delay\":1}");

	        controller = controllers[cu];
	        ATTRDBG_ << " REUsing controller:"<<controller;
	    }
	} catch(chaos::CException e){
        ATTRDBG_<<"%% WARNING exception during controller initialization of \""<<cu<<"\" :"<<e.what();
        return -1;
	}
	  return (controller!=NULL)?0:-1;
}
ChaosDatasetAttribute::ChaosDatasetAttribute(std::string path,uint32_t timeo_) {

    std::string cu =path;
    ptr_cache=NULL;
    cache_size=0;
    timeo=timeo_;
    cache_updated=0;
    int retry=10;
    int ret;
    if(cu.find_last_of(chaos::PATH_SEPARATOR)==0){
        throw chaos::CException(-1, "bad attribute description",__FUNCTION__);
    }
    cu.erase(cu.find_last_of(chaos::PATH_SEPARATOR),cu.size());
     ATTRDBG_ << "CU NAME:\""<<cu<<"\"";
    
    attr_path=path;
    attr_name=path;
    attr_name.erase(0,attr_path.find_last_of(chaos::PATH_SEPARATOR)+1);
    ATTRDBG_ << "ATTR NAME:\""<<attr_name<<"\", allocating controller";
    attr_parent=cu;
    do {
    	ret= allocateController(cu);
    	if(ret<0){
    		sleep (1);
    		ATTRDBG_ << " controller of "<<cu<<" is not ready, retry:"<<retry;
    	}
    } while((ret<0)&& (retry--));

    upd_mode=EVERYTIME;
    update_time=0;
    if(ret<0){
    	ATTRERR_<<"## cannot allocate controller for cu:\""+cu+"\"";
       throw chaos::CException(-1, "cannot allocate controller for:"+ cu,__FUNCTION__);

    }
    attr_size =0;
    controller->setRequestTimeWaith(timeo);
    controller->getAttributeDescription(attr_name,attr_desc);
    controller->getDeviceAttributeRangeValueInfo(attr_name,attr_type);
    if(paramToDataset.find(attr_parent)==paramToDataset.end()){
    	datinfo_psh inf=boost::shared_ptr<datinfo_t>(new datinfo_t());
    	paramToDataset.insert(std::make_pair(attr_parent,inf));
    	info = inf;
    } else {
    	info = paramToDataset[attr_parent];
    }
    try{
        get(&attr_size);
    } catch(chaos::CException e){
        ATTRDBG_<<"%% WARNING  no data present in live for:"<<attr_path;
    }

    resize(attr_size);
    
    ATTRDBG_<<"Retrieved "<<attr_path<<" desc:\""<<attr_desc<<"\" "<< " type:"<<attr_type.valueType<<" subtype:"<<attr_type.binType<<" size:"<<attr_size;
 }
ChaosDatasetAttribute::ChaosDatasetAttribute(const ChaosDatasetAttribute& orig) {
}
void ChaosDatasetAttribute::resize(int32_t newsize) throw (chaos::CException){
    if(cache_size<newsize){
        ptr_cache=realloc(ptr_cache,newsize);
        cache_size = newsize;
        if(ptr_cache==NULL){
                throw chaos::CException(-1001,"cannot allocate memory cache for:"+attr_path,__PRETTY_FUNCTION__);

        }
    }
    attr_size=newsize;

}

ChaosDatasetAttribute::~ChaosDatasetAttribute() {
     ATTRDBG_<<" delete attribute:"<<attr_path;
    if(controllers.find(attr_parent)!=controllers.end()){
        ATTRDBG_<<" remove controller:"<<attr_parent;
        controllers.erase(controllers.find(attr_parent));
    }
    info.reset();
    if(paramToDataset[attr_parent])
     if(ptr_cache){
         free(ptr_cache);
         cache_size=0;
     }
     ptr_cache=NULL;
     
}

int ChaosDatasetAttribute::set(void* buf, int size){
    int ret=0;
    resize(size);
    
    std::memcpy(ptr_cache,buf,size);
    if(attr_type.dir == chaos::DataType::Input){
        ATTRDBG_<<"setting remote variable \""<<attr_path<<"\"";
        ret=controller->setAttributeToValue(attr_name.c_str(),buf,1,size);
    }
    return ret;
}

void* ChaosDatasetAttribute::get(uint32_t*size){
    void*tmp=NULL;
    boost::mutex::scoped_lock l(data_access);
    if(paramToDataset.count(attr_parent)){
        boost::posix_time::ptime pt=boost::posix_time::microsec_clock::local_time();
        uint64_t tget=pt.time_of_day().total_microseconds();
        pt= boost::posix_time::microsec_clock::local_time();
        if(upd_mode==EVERYTIME || ((upd_mode==NOTBEFORE)&& ((tget - paramToDataset[attr_parent]->tget)> update_time)) ){
#ifdef __CHAOS_UI__
            chaos::common::data::CDataWrapper*tmpw=controller->fetchCurrentDatatasetFromDomain((attr_type.dir == chaos::DataType::Input)?chaos::ui::DatasetDomainInput: chaos::ui::DatasetDomainOutput);
#else
            chaos::common::data::CDataWrapper*tmpw=controller->fetchCurrentDatatasetFromDomain((attr_type.dir == chaos::DataType::Input)?chaos::metadata_service_client::node_controller::DatasetDomainInput:chaos::metadata_service_client::node_controller::DatasetDomainOutput);

#endif
            if(tmpw==NULL){
                throw chaos::CException(-1000,"cannot retrieve data for:"+attr_path,__PRETTY_FUNCTION__);
            }
      //      ATTRDBG_<<"fetched  ptr:"<<paramToDataset[attr_parent]<<" \""<<attr_path<<"\" last:"<<(tget - paramToDataset[attr_parent]->tget) <<" us ago update mode:"<<upd_mode;

            paramToDataset[attr_parent]->tget = tget;
            paramToDataset[attr_parent]->data = tmpw;
            controller->getTimeStamp(paramToDataset[attr_parent]->tstamp);
            tmp=(void*)tmpw->getRawValuePtr(attr_name);
            if(tmp){
            	attr_size=tmpw->getValueSize(attr_name);
            	if(attr_size){
            	       resize(attr_size);
            	       std::memcpy(ptr_cache,tmp,attr_size);
            	}
            } else {
            	 throw chaos::CException(-1000,"cannot variable \""+attr_name +"\" not found in:"+attr_path,__PRETTY_FUNCTION__);
            }
            cache_updated=tget;
        } else if((upd_mode==NOTBEFORE)){
        	//ATTRDBG_<<" not fetch because \""<<attr_path<<"\" has been fetched "<<(tget - paramToDataset[attr_parent]->tget) <<" us ago updating each:"<<update_time;
        	if(cache_updated!=paramToDataset[attr_parent]->tget){
				tmp =(void*)paramToDataset[attr_parent]->data->getRawValuePtr(attr_name);
				if(tmp){
					attr_size =paramToDataset[attr_parent]->data->getValueSize(attr_name);
					if(attr_size){
										   resize(attr_size);
										   std::memcpy(ptr_cache,tmp,attr_size);
									}
				}
				cache_updated=paramToDataset[attr_parent]->tget;
        	}
        }

     
        if(size){
            *size = attr_size;
        }
        return ptr_cache;
    }
    throw chaos::CException(-1000,"Attribute "+attr_path+" not found",__PRETTY_FUNCTION__);
    
    
}


  ChaosDatasetAttribute::datinfo& ChaosDatasetAttribute::getInfo(){
	  ChaosDatasetAttribute::datinfo* ptr=paramToDataset[attr_parent].get();
      return *ptr;
  }

 void ChaosDatasetAttribute::setTimeout(uint64_t timeo_ms){
    timeo=timeo_ms;
    controller->setRequestTimeWaith(timeo);
     
 }
 void ChaosDatasetAttribute::setUpdateMode(UpdateMode mode,uint64_t ustime){
     upd_mode = mode;
     update_time = ustime;
 }
