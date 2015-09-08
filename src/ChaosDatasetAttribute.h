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

class ChaosDatasetAttribute {
    
    chaos::ui::DeviceController* controller;
    
    void setAttribute(chaos::common::data::CDataWrapper&data);
    chaos::common::data::CDataWrapper* getAttribute(std::string path);
    
    std::map< std::string,chaos::common::data::CDataWrapper > paramToDataset;

public:
    ChaosDatasetAttribute();
    
    ChaosDatasetAttribute(std::string path);

    ChaosDatasetAttribute(const ChaosDatasetAttribute& orig);
    virtual ~ChaosDatasetAttribute();
    template <typename T>
    T* get( std::string name){
        if(paramToDataset.count(name)){
            size_type pos=name.find_last_of("/");
            name.erase(name.begin(),pos);
           // return paramToDataset[name]
        }
    }
    
    template <typename T>
    int set(std::string path,T& data);
    /**
     set the timeout for the remote access
     @param timeo_ms update time
     */
    void setTimeout(int timeo_ms);
    /**
     set the update mode
     @param mode mode
     @param ustime update time
     */
    void setUpdateMode(int mode,int ustime);
private:
    int timeo_ms;
    
   
};

#endif	/* ChaosDatasetAttribute_H */

