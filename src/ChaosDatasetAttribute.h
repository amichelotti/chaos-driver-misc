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
#define ATTRDBG_ LDBG_<< "[ "<<__FUNCTION__<<" ]"
#define ATTRERR_ LERR_ << "[ "<<__FUNCTION__<<" ]"


class ChaosDatasetAttributeBase {
    
    
    struct datinfo {
        
        uint64_t tget;
        uint64_t tstamp;
       
        chaos::common::data::CDataWrapper*  data;
        datinfo(){tget=tstamp=0; data=NULL;}
    };
    
    enum UpdateMode{
        EVERYTIME,
        NOTBEFORE
    };
    
public:
    static const char pathSeparator ='/';

    ChaosDatasetAttributeBase(std::string path,uint32_t timeo=5000);

    ChaosDatasetAttributeBase(const ChaosDatasetAttributeBase& orig);
    virtual ~ChaosDatasetAttributeBase();
    
   
    /**
     set the timeout for the remote access
     @param timeo_ms update time
     */
    void setTimeout(uint32_t timeo_ms);
    /**
     set the update mode
     @param mode mode
     @param ustime update time
     */
    void setUpdateMode(UpdateMode mode,uint64_t ustime);
private:
    datinfo info;
    uint64_t update_time;
    uint32_t timeo;
    chaos::ui::DeviceController* controller;
    std::string attr_path;
    
    UpdateMode upd_mode;
    static std::map< std::string,datinfo* > paramToDataset;
protected:
    void setAttribute(void*buf,int size);
    void* getAttribute();

};

template <typename T>
class ChaosDatasetAttribute:public ChaosDatasetAttributeBase{
public:
    
    T* get( ){
       return reinterpret_cast<T*>(getAttribute());
    }
     void set(T*t){
         setAttribute((void*)t,sizeof(T));
     }
};

#endif	/* ChaosDatasetAttribute_H */

