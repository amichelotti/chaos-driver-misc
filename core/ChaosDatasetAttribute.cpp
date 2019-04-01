/* 
 * File:   ChaosDatasetAttribute.cpp
 * Author: michelo
 *
 * Created on September 2, 2015, 5:29 PM
 */

#include "ChaosDatasetAttribute.h"
#include <chaos/common/exception/CException.h>
#include <chaos/cu_toolkit/ChaosCUToolkit.h>
#include <chaos_metadata_service_client/ChaosMetadataServiceClient.h>


using namespace ::driver::misc;
using namespace chaos::cu::data_manager;
// parameter to cache
std::map< std::string,ChaosDatasetAttribute::datinfo_psh > ChaosDatasetAttribute::paramToDataset;
// pather to controller
std::map< std::string,ChaosDatasetAttribute::ctrl_t > ChaosDatasetAttribute::controllers;



template<class T>
int query_int( chaos::metadata_service_client::node_controller::CUController* controller, const std::string& attr_name, uint64_t ms_start,uint64_t ms_end,std::vector<T>&res){

    chaos::common::io::QueryCursor *query_cursor=NULL;

    controller->executeTimeIntervallQuery((chaos::metadata_service_client::node_controller::DatasetDomain)chaos::DataPackCommonKey::DPCK_DATASET_TYPE_OUTPUT,
                                          ms_start,
                                          ms_end,
                                          &query_cursor);
    if(query_cursor==NULL){
        ATTRERR_<<"NO CURSOR";
        return 0;
    }
    int nelem=0;
    while(query_cursor->hasNext() ){
        ChaosSharedPtr<chaos::common::data::CDataWrapper> q_result(query_cursor->next());
        if(q_result->hasKey(attr_name)){
            if(q_result->getValueType(attr_name) == chaos::DataType::TYPE_BYTEARRAY){
                uint32_t size;
                const char* tmp=q_result->getBinaryValue(attr_name,size);
                ATTRDBG_<<attr_name<<" type binary, subtype:"<<q_result->getBinarySubtype(attr_name)<<" elements:"<<size/sizeof(T);
                for(int cnt=0;cnt<size/sizeof(T);cnt++){
                    res.push_back(*((reinterpret_cast<const T*>(tmp)) + cnt));
                    nelem++;
                }
            } else {
                res.push_back(q_result->getValue<T>(attr_name));
                nelem++;

            }

        }

    }
    controller->releaseQuery(query_cursor);
    return nelem;
}

template<class T>
int query_vec_int( chaos::metadata_service_client::node_controller::CUController* controller, const std::string& attr_name, uint64_t ms_start,uint64_t ms_end,std::vector< std::vector<T> >&res){

    chaos::common::io::QueryCursor *query_cursor=NULL;

    controller->executeTimeIntervallQuery((chaos::metadata_service_client::node_controller::DatasetDomain)chaos::DataPackCommonKey::DPCK_DATASET_TYPE_OUTPUT,
                                          ms_start,
                                          ms_end,
                                          &query_cursor);
    if(query_cursor==NULL){
        ATTRERR_<<"NO CURSOR";
        return 0;
    }
    int nelem=0;
    while(query_cursor->hasNext() ){
        ChaosSharedPtr<chaos::common::data::CDataWrapper> q_result(query_cursor->next());
        if(q_result->hasKey(attr_name) && q_result->isVector(attr_name)){
            ChaosSharedPtr<chaos::common::data::CMultiTypeDataArrayWrapper> vect=q_result->getVectorValue(attr_name);
            std::vector<T> tt;
            for (int idx = 0; idx < vect->size(); idx++) {
                tt.push_back(vect->getElementAtIndex<T>(idx));
            }
            res.push_back(tt);
            nelem++;
        }
    }

    controller->releaseQuery(query_cursor);
    return nelem;
}

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
        //chaos::metadata_service_client::ChaosMetadataServiceClient::getInstance()->enableMonitor();

        initialize_framework++;
        ATTRDBG_<<" END ChaosMetadataServiceClient Framework ...";

    }
    try{
        if(controllers.find(cu)==controllers.end()){

            chaos::metadata_service_client::node_controller::CUController *cu_ctrl = NULL;
            chaos::metadata_service_client::ChaosMetadataServiceClient::getInstance()->getNewCUController(cu,&cu_ctrl);
            controller=cu_ctrl;
            controllers[cu]=controller;

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
ChaosDatasetAttribute::ChaosDatasetAttribute(std::string path,iomode_t io,uint32_t timeo_):iomode(io),attr_path(path),timeo(timeo_) {

    std::string cu =path;
    //ptr_cache=NULL;
    //cache_size=0;
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
    attr_unique_name= ((iomode==INPUT)?std::string("INPUT/"):std::string("OUTPUT/"))+ attr_name;
    controller->setRequestTimeWaith(timeo);
    controller->getAttributeDescription(attr_name,attr_desc);
    controller->getDeviceAttributeRangeValueInfo(attr_name,attr_type);
    if(paramToDataset.find(attr_unique_name)==paramToDataset.end()){
        datinfo_psh inf=boost::shared_ptr<datinfo_t>(new datinfo_t(attr_parent,attr_unique_name));
        paramToDataset.insert(std::make_pair(attr_unique_name,inf));
        info = inf;
    } else {
        info = paramToDataset[attr_unique_name];
    }

    get(&attr_size);


    info->resize(attr_size);
    if(attr_type.binType.size()){
        ATTRDBG_<<"Retrieved "<<attr_path<<" desc:\""<<attr_desc<<"\" "<< " type:"<<attr_type.valueType<<" subtype:"<<attr_type.binType[0]<<" size:"<<attr_size << " Direction:"<<((attr_type.dir == chaos::DataType::Input)?"Input": ((attr_type.dir == chaos::DataType::Output)?"Output":"Other"))<<")";
    } else {
        ATTRDBG_<<"Retrieved "<<attr_path<<" desc:\""<<attr_desc<<"\" "<< " type:"<<attr_type.valueType<<"  size:"<<attr_size << " Direction:"<<((attr_type.dir == chaos::DataType::Input)?"Input": ((attr_type.dir == chaos::DataType::Output)?"Output":"Other"))<<")";
    }
}
ChaosDatasetAttribute::ChaosDatasetAttribute(const ChaosDatasetAttribute& orig) {
}

void ChaosDatasetAttribute::datinfo::resize(uint32_t newsize){
    if(cache_size<newsize){
        ptr_cache=realloc(ptr_cache,newsize);
        cache_size = newsize;
        if(ptr_cache==NULL){
            ATTRERR_<<"cannot allcate memory cache for:"<<id;
            throw chaos::CException(-1001,"cannot allocate memory cache for:"+id,__PRETTY_FUNCTION__);

        }
    }
    //   attr_size=newsize;

}

ChaosDatasetAttribute::~ChaosDatasetAttribute() {
    ATTRDBG_<<" delete attribute:"<<attr_path;
    std::map< std::string,ChaosDatasetAttribute::ctrl_t >::iterator i=controllers.find(attr_parent);
    std::map< std::string,ChaosDatasetAttribute::datinfo_psh >::iterator j=paramToDataset.find(attr_unique_name);
    info.reset();
    if(j!=paramToDataset.end()){
        ATTRDBG_<<"cache :"<<i->first<<" in use by:"<<j->second.use_count()<<" Objects";

        if(j->second.use_count()==1){
            bool can_remove_controller=true;
            ATTRDBG_<<" remove cached data and controller from list :"<<attr_parent;
            paramToDataset.erase(j);
            for(std::map< std::string,ChaosDatasetAttribute::datinfo_psh >::iterator k=paramToDataset.begin();k!=paramToDataset.end();k++){
                if(k->second->pather == attr_parent){
                    can_remove_controller=false;
                }
            }
            if(can_remove_controller){
                chaos::metadata_service_client::ChaosMetadataServiceClient::getInstance()->deleteCUController(controller);
                if(i!=controllers.end()){
                    ATTRDBG_<<" remove controller from list :"<<attr_parent;
                    controllers.erase(i);
                }
            }
        }
    }


}

int ChaosDatasetAttribute::set(void* buf, int size){
    int ret=0;
    info->set(buf,size);
    //  std::memcpy(ptr_cache,buf,size);
    if(iomode == INPUT){
        ATTRDBG_<<"setting remote variable \""<<attr_path<<"\"";
        ret=controller->setAttributeToValue(attr_name.c_str(),buf,1,size);
    }
    return ret;
}

void* ChaosDatasetAttribute::get(uint32_t*size){
    void*tmp=NULL;
    boost::mutex::scoped_lock l(data_access);

    boost::posix_time::ptime pt=boost::posix_time::microsec_clock::local_time();
    uint64_t tget=pt.time_of_day().total_microseconds();
    pt= boost::posix_time::microsec_clock::local_time();
    if(upd_mode==EVERYTIME || ((upd_mode==NOTBEFORE)&& ((tget - info->tget)> update_time)) ){
        chaos::common::data::CDataWrapper*tmpw=controller->fetchCurrentDatatasetFromDomain((iomode == INPUT)?KeyDataStorageDomainInput:KeyDataStorageDomainOutput).get();

        if(tmpw==NULL){
            throw chaos::CException(-1000,"cannot retrieve data for:"+attr_path,__PRETTY_FUNCTION__);
        }
        //      ATTRDBG_<<"fetched  ptr:"<<paramToDataset[attr_parent]<<" \""<<attr_path<<"\" last:"<<(tget - paramToDataset[attr_parent]->tget) <<" us ago update mode:"<<upd_mode;

        info->tget = tget;
        controller->getTimeStamp(info->tstamp,true);
        controller->getPackSeq(info->pckid);
        if(tmpw->isVector(attr_name)){
            attr_size=0;
            uint32_t sizev,sizexp;

            ChaosSharedPtr<chaos::common::data::CMultiTypeDataArrayWrapper> cu_t_arr=tmpw->getVectorValue(attr_name);
            ATTRDBG_<<"fetching Vector of size:"<<cu_t_arr->size();
            bool isInt32=false,isDouble=false,isInt64=false;
            isDouble=cu_t_arr->isDoubleElementAtIndex(0);
            isInt32=cu_t_arr->isInt32ElementAtIndex(0);
            isInt64=cu_t_arr->isInt64ElementAtIndex(0);
            cu_t_arr->getRawValueAtIndex(0,sizexp);

            for(int idx = 0;
                idx < cu_t_arr->size();
                idx++) {
                const char*tmp=cu_t_arr->getRawValueAtIndex(idx,sizev);

                if(sizev<sizexp){
                    //probabilmente intero dentro double
                    int32_t*pi=(int32_t*)tmp;
                    double ftmp=*pi;
                    info->set((void*)&ftmp,sizexp,idx*sizexp);
                } else {
                    info->set((void*)tmp,sizexp,idx*sizexp);

                }

               /*
                if((old_size!=0) && (old_size!=sizev)){
                    ATTRDBG_<<"type vector size mismatch at ["<<idx<<"] old:"<<old_size<<" size:"<<sizev<<" totsize:"<<attr_size<<" is int:"<<cu_t_arr->isInt32ElementAtIndex(idx)<<" is double:"<<cu_t_arr->isDoubleElementAtIndex(idx);
                    double*pd=(double*)tmp;
                    int32_t*pi=(int32_t*)tmp;
                    float*pf=(float*)tmp;
                    ATTRDBG_<<"double val:"<<*pd<<" int val:"<<*pi<<" float val:"<<*pf;

                }
                old_size=sizev;
    */
                attr_size+=sizexp;
            }
        } else {
            tmp=(void*)tmpw->getRawValuePtr(attr_name);
            if(tmp){
                attr_size=tmpw->getValueSize(attr_name);
                if(attr_size){
                    info->set(tmp,attr_size);
                }

            } else {
                std::stringstream ss;

                ss<<"cannot access variable \""<<attr_name <<"\" ("<<((attr_type.dir == chaos::DataType::Input)?"Input": "Output")<<") not found in:"<<attr_path<<" json:"<<tmpw->getJSONString();
                ATTRERR_<<ss.str();
                throw chaos::CException(-1000,ss.str().c_str(),__PRETTY_FUNCTION__);
            }
        }
        cache_updated=tget;
    }


    if(size){
        *size = attr_size;
    }
    return info->ptr_cache;

    
    
}

uint64_t ChaosDatasetAttribute::getTimestamp(){
    return info->tstamp;
}

uint64_t ChaosDatasetAttribute::getPackSeq(){
    return info->pckid;
}
ChaosDatasetAttribute::datinfo& ChaosDatasetAttribute::getInfo(){
    ChaosDatasetAttribute::datinfo* ptr=info.get();
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
ChaosDatasetAttribute::datinfo::datinfo(const std::string&_pather,const std::string& _id):id(_id),pather(_pather){
    ATTRDBG_<<"creating cache:"<<_id<<" pather:"<<pather;

    tget=tstamp=0;
    ptr_cache=NULL;
    cache_size=0;
}
ChaosDatasetAttribute::datinfo::~datinfo(){
    ATTRDBG_<<"removing cache:"<<id;

    free(ptr_cache);cache_size=0;ptr_cache=NULL;
}
void ChaosDatasetAttribute::datinfo::set(void*buf,int size,int off){
    if((off+size)>cache_size)
        resize((off+size));
    char *ptr=((char*)ptr_cache)+off;
    memcpy((void*)ptr,buf,size);
}

int ChaosDatasetAttribute::query(uint64_t ms_start,uint64_t ms_end,std::vector<double>&v){
    return query_int(controller,attr_name,ms_start,ms_end,v);
}
int ChaosDatasetAttribute::query(uint64_t ms_start,uint64_t ms_end,std::vector<int32_t>&v){
    return query_int(controller,attr_name,ms_start,ms_end,v);

}
int ChaosDatasetAttribute::query(uint64_t ms_start,uint64_t ms_end,std::vector<int64_t>&v){
    return query_int(controller,attr_name,ms_start,ms_end,v);

}
int ChaosDatasetAttribute::query(uint64_t ms_start,uint64_t ms_end,std::vector<char>&v){
    return query_int(controller,attr_name,ms_start,ms_end,v);

}
int ChaosDatasetAttribute::query(uint64_t ms_start,uint64_t ms_end,char*v){
    return 0;
}

int ChaosDatasetAttribute::query(uint64_t ms_start,uint64_t ms_end,std::vector<std::vector<double> > &v){
    return query_vec_int(controller,attr_name,ms_start,ms_end,v);
}
int ChaosDatasetAttribute::query(uint64_t ms_start,uint64_t ms_end,std::vector<bool>& v){
    return query_int(controller,attr_name,ms_start,ms_end,v);

}

int ChaosDatasetAttribute::query(uint64_t ms_start,uint64_t ms_end,std::vector<std::vector<int32_t> > &v){
    return query_vec_int(controller,attr_name,ms_start,ms_end,v);

}
int ChaosDatasetAttribute::query(uint64_t ms_start,uint64_t ms_end,std::vector<std::vector<int64_t> > &v){
    return query_vec_int(controller,attr_name,ms_start,ms_end,v);

}
int ChaosDatasetAttribute::query(uint64_t ms_start,uint64_t ms_end,std::vector<std::vector<bool> > &v){
    return query_vec_int(controller,attr_name,ms_start,ms_end,v);

}
