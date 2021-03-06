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

#include <chaos/ui_toolkit/HighLevelApi/DeviceController.h>

#define ATTRAPP_ LAPP_ << "[ "<<__FUNCTION__<<" ]"
#define ATTRDBG_ LDBG_<< "[ "<<__PRETTY_FUNCTION__<<" ]"
#define ATTRERR_ LERR_ << "[ "<<__PRETTY_FUNCTION__<<" ]"

namespace driver{
    
    namespace misc{
class ChaosDatasetAttribute{
    
  public:  
    struct datinfo {
        
        uint64_t tget;
        uint64_t tstamp;
        chaos::common::data::CDataWrapper*  data;
        datinfo(){tget=tstamp=0; data=NULL;}
        uint64_t getTimeStamp()const {return tstamp;}
        
        uint64_t getLastGet()const {return tget;}
    };
    
    enum UpdateMode{
        EVERYTIME,
        NOTBEFORE
    };
    


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
    typedef boost::shared_ptr<chaos::ui::DeviceController> ctrl_t;
private:
    datinfo info;
    uint64_t update_time;
    uint32_t timeo;
    std::string attr_parent;
    std::string attr_path;
    std::string attr_name;
    std::string attr_desc;
    chaos::DataType::DataType attr_type;
    chaos::DataType::DataSetAttributeIOAttribute attr_dir;
    uint32_t attr_size;
    UpdateMode upd_mode;
    
    static std::map< std::string,datinfo* > paramToDataset;
    static std::map<std::string,ctrl_t> controllers;
    ctrl_t controller;

public:
    int set(void*buf,int size);
    void* get(uint32_t* size);
     std::string getParent(){return attr_parent;}

    std::string getPath(){return attr_path;}
    std::string getName(){return attr_name;}
    std::string getDesc(){return attr_desc;}
    chaos::DataType::DataType getType(){return attr_type;}
    chaos::DataType::DataSetAttributeIOAttribute getDir(){return attr_dir;}
    uint32_t getSize(){return attr_size;}
    template<typename T>
    operator T(){
        return *reinterpret_cast<T*>(get(NULL));
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

