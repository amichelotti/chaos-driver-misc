/* 
 * File:   ChaosDatasetAttribute.h
 * Author: michelo
 *
 * Created on September 2, 2015, 5:29 PM
 */

#ifndef ChaosDatasetAttribute_H
#define	ChaosDatasetAttribute_H
#include <map>
#include <string>
#include <chaos/ui_toolkit/ChaosUIToolkit.h>
#ifdef __CHAOS_UI__
#include <chaos/ui_toolkit/HighLevelApi/DeviceController.h>
#else
#include <ChaosMetadataServiceClient/node_controller/CUController.h>
#endif
#define ATTRAPP_ LAPP_ << "[ "<<__FUNCTION__<<" ]"
#define ATTRDBG_ LDBG_<< "[ "<<__PRETTY_FUNCTION__<<" ]"
#define ATTRERR_ LERR_ << "[ "<<__PRETTY_FUNCTION__<<" ]"

namespace driver{
    
    namespace misc{
class ChaosDatasetAttribute{
    
  public:  
    typedef struct datinfo {
        
        uint64_t tget;
        uint64_t tstamp;
        chaos::common::data::CDataWrapper*  data;
        datinfo(){tget=tstamp=0; data=NULL;}
        uint64_t getTimeStamp()const {return tstamp;}
        
        uint64_t getLastGet()const {return tget;}
    } datinfo_t;
    
    typedef  boost::shared_ptr<datinfo_t> datinfo_psh;
    enum UpdateMode{
        EVERYTIME,
        NOTBEFORE,
        DONTUPDATE
    };
    

    datinfo_psh info;
    static int initialize_framework;
    ChaosDatasetAttribute(std::string path,uint32_t timeo=5000);

    ChaosDatasetAttribute(const ChaosDatasetAttribute& orig);
    virtual ~ChaosDatasetAttribute();
    
   
    /**
     set the timeout for the remote access
     @param timeo_ms update time
     */
    void setTimeout(uint64_t timeo_ms);
    /**
     set the update mode
     @param mode mode
     @param ustime update time
     */
    void setUpdateMode(UpdateMode mode,uint64_t ustime);
    
    datinfo& getInfo();
#ifdef __CHAOS_UI__
    typedef boost::shared_ptr<chaos::ui::DeviceController> ctrl_t;
#else
    typedef chaos::metadata_service_client::node_controller::CUController* ctrl_t;

#endif

private:
    uint64_t update_time;
    uint32_t timeo;
    std::string attr_parent;
    std::string attr_path;
    std::string attr_name;
    std::string attr_desc;
    chaos::common::data::RangeValueInfo attr_type;
    void *ptr_cache;
    int32_t cache_size; 
    uint64_t cache_updated;
    //chaos::DataType::DataSetAttributeIOAttribute attr_dir;
    uint32_t attr_size;
    UpdateMode upd_mode;
    
    static std::map< std::string,datinfo_psh > paramToDataset;
    static std::map<std::string,ctrl_t> controllers;
    ctrl_t controller;
    boost::mutex data_access;
    int allocateController(std::string name);
public:
    int set(void*buf,int size);
    void* get(uint32_t* size);
    void resize(int32_t newsize) throw (chaos::CException);
    std::string getParent(){return attr_parent;}
    std::string getGroup();
    std::string getPath(){return attr_path;}
    std::string getName(){return attr_name;}
    std::string getDesc(){return attr_desc;}
    chaos::common::data::RangeValueInfo& getValueInfo(){return attr_type;}
    chaos::DataType::DataType getType(){return attr_type.valueType;}
    chaos::DataType::BinarySubtype getBinaryType(){return attr_type.binType;}

    chaos::DataType::DataSetAttributeIOAttribute getDir(){return attr_type.dir;}
    uint32_t getSize(){return attr_size;}
    template<typename T>
    operator T()  throw (chaos::CException){
            
        return *reinterpret_cast<T*>(get(NULL));
    }
    template <typename T> operator T*()  throw (chaos::CException){
            
        return reinterpret_cast<T*>(get(NULL));
    }
    template<typename T>
    T operator=(T& d) throw (chaos::CException) {
        if(set(&d,sizeof(T))==0)
            return d;
        throw chaos::CException(-1,"cannot assign to remote variable:"+attr_path,__FUNCTION__);
    }
    
};
    }}
/*
template <typename T>
class ChaosDatasetAttribute:public ChaosDatasetAttributeBase{
public:
    ChaosDatasetAttribute(const char* path,uint32_t timeo=5000):ChaosDatasetAttributeBase(path,timeo){}
    T get( ){
       return *reinterpret_cast<T*>(getAttribute(NULL));
    }
    
    T* get( uint32_t& size){
       return reinterpret_cast<T*>(getAttribute(&size));
    }
     void set(T& t){
         setAttribute((void*)&t,sizeof(T));
     }
   
};
*/

#endif	/* ChaosDatasetAttribute_H */

