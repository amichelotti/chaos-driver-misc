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
#include <chaos/common/exception/CException.h>
#include <chaos/common/data/CUSchemaDB.h>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <chaos/common/global.h>

#define ATTRAPP_ LAPP_ << "[ "<<__FUNCTION__<<" ]"
#define ATTRDBG_ LDBG_<< "[ "<<__PRETTY_FUNCTION__<<" ]"
#define ATTRERR_ LERR_ << "[ "<<__PRETTY_FUNCTION__<<" ]"

namespace chaos{
namespace common{
namespace data{
    class CDataWrapper;
}
}
namespace metadata_service_client{
	class ChaosMetadataServiceClient;
namespace node_controller{
	class CUController;

}
}
}
#define ChaosCuInput(path,var) ::driver::misc::ChaosDatasetAttribute var(path,::driver::misc::ChaosDatasetAttribute::INPUT)
#define ChaosCuOutput(path,var) ::driver::misc::ChaosDatasetAttribute var(path,::driver::misc::ChaosDatasetAttribute::OUTPUT)

namespace driver{
    
    namespace misc{
class ChaosDatasetAttribute{
    
  public:  
    typedef struct datinfo {
        std::string id;
        std::string pather;
        uint64_t tget;
        uint64_t tstamp;
        uint64_t pckid;
        void *ptr_cache;
        int32_t cache_size;
       // chaos::common::data::CDataWrapper*  data;
        datinfo(const std::string& _pather,const std::string& _id);
        ~datinfo();
        void set(void*buf,int size,int off=0);
        void resize(uint32_t size);

        uint64_t getTimeStamp()const {return tstamp;}
        
        uint64_t getLastGet()const {return tget;}
    } datinfo_t;
    
    typedef  boost::shared_ptr<datinfo_t> datinfo_psh;
    enum UpdateMode{
        EVERYTIME,
        NOTBEFORE,
        DONTUPDATE
    };
    
    typedef enum IO {
        INPUT,
        OUTPUT
    } iomode_t;
    datinfo_psh info;
    static int initialize_framework;
    ChaosDatasetAttribute(std::string path,iomode_t IO=OUTPUT,uint32_t timeo=5000);

    explicit ChaosDatasetAttribute(const ChaosDatasetAttribute& orig);
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
    typedef chaos::metadata_service_client::node_controller::CUController* ctrl_t;


private:
    iomode_t iomode;
    uint64_t update_time;
    uint32_t timeo;
    std::string attr_parent; //parent= cu name
    std::string attr_path; // full path name
    std::string attr_name; // variable name
    std::string attr_desc; // description
    std::string attr_unique_name; // variable name included direction (input,output)
    chaos::common::data::RangeValueInfo attr_type;

  //  int32_t cache_size;
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

    std::string getParent(){return attr_parent;}
    std::string getGroup();
    std::string getPath(){return attr_path;}
    std::string getName(){return attr_name;}
    std::string getDesc(){return attr_desc;}
    chaos::common::data::RangeValueInfo& getValueInfo(){return attr_type;}
    chaos::DataType::DataType getType(){return attr_type.valueType;}
    chaos::common::data::VectorBinSubtype getBinaryType(){return attr_type.binType;}
    uint64_t getTimestamp();
    uint64_t getPackSeq();

    chaos::DataType::DataSetAttributeIOAttribute getDir(){return attr_type.dir;}
    uint32_t getSize(){return attr_size;}

    template<typename T>
    operator T()  throw (chaos::CException){
            
        return *reinterpret_cast<T*>(get(NULL));
    }
    template <typename T> operator T*()  throw (chaos::CException){
            
        return reinterpret_cast<T*>(get(NULL));
    }
    template<class T>
     operator const std::vector<T> () throw (chaos::CException){
        std::vector<T> tmp;
        uint32_t size=0;
        T* d=(T*)get(&size);
        ATTRDBG_<<attr_path<<": vector byte size:"<<size<<" items:"<<size/sizeof(T) << " size type:"<<sizeof(T);
        for(int cnt=0;cnt<size/sizeof(T);cnt++){
            tmp.push_back(d[cnt]);
        }
        return tmp;
    }

    template<typename T>
    ChaosDatasetAttribute& operator=(const T& d) throw (chaos::CException) {
        if(set((void*)&d,sizeof(T))==0)
            return *this;
        throw chaos::CException(-1,"cannot assign to remote variable:"+attr_path,__FUNCTION__);
    }



    
};

    }}


#endif	/* ChaosDatasetAttribute_H */

