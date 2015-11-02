/* 
 * File:   ChaosDatasetAttributeGroup.cpp
 * Author: michelo
 * 
 * Created on September 2, 2015, 5:29 PM
 */

#include <vector>

#include "ChaosDatasetAttributeGroup.h"
using namespace ::driver::misc;
ChaosDatasetAttributeGroup::ChaosDatasetAttributeGroup() {
    interval=20000;
    timeo = 2*interval;
 }


ChaosDatasetAttributeGroup::~ChaosDatasetAttributeGroup() {
    ATTRDBG_<<" Deleting ChaosDatasetAttributeGroup and all the content";
    for(std::map<std::string,ChaosDatasetAttribute* >::iterator i=id2attr.begin();i!=id2attr.end();i++){
        delete i->second;
    }
    id2attr.clear();
    name2attrs.clear();
}



void ChaosDatasetAttributeGroup::add(ChaosDatasetAttribute& d){
    add(&d);
}
void ChaosDatasetAttributeGroup::add(ChaosDatasetAttribute* d){
    if(id2attr.find(d->getPath())==id2attr.end()){
        int found=0;
        std::map<std::string,ChaosDatasetAttribute* >::iterator i;

        ATTRDBG_<<"adding new dataset attribute :"<<d->getPath();

        d->setTimeout(2*timeo/1000);
        for(i=id2attr.begin();i!=id2attr.end();i++){
            if(i->second->getParent()==d->getParent())
                found++;
        }
      
        id2attr.insert(std::make_pair(d->getPath(),d));
        name2attrs[d->getName()].push_back(d);
    } 
    
}

ChaosDatasetAttribute* ChaosDatasetAttributeGroup::add(std::string path){
    ChaosDatasetAttribute*ret=NULL;
    boost::mutex::scoped_lock sl(lock_sync);
        

    if(id2attr.find(path)==id2attr.end()){
        ChaosDatasetAttribute*d = new ChaosDatasetAttribute(path);
        add(d);
        ret = d;
    } 
    return ret;
}

std::vector<ChaosDatasetAttribute*> ChaosDatasetAttributeGroup::getAttributes(){
    std::vector<ChaosDatasetAttribute*> ret;
    
    for(std::map<std::string,ChaosDatasetAttribute* >::iterator i=id2attr.begin();i!=id2attr.end();i++){
        ret.push_back(i->second);
    }
    return ret;

}
void ChaosDatasetAttributeGroup::remove(std::string path){
    boost::mutex::scoped_lock sl(lock_sync);
    std::map<std::string,ChaosDatasetAttribute* >::iterator i=id2attr.find(path);
   
    if(i!=id2attr.end()){
        ATTRDBG_<<"removing dataset attribute path:<<"<<path;

        std::map<std::string,std::vector<ChaosDatasetAttribute*> >::iterator j=name2attrs.find(i->second->getName());
        if(j!=name2attrs.end()){
            ATTRDBG_<<"removing dataset attribute name:<<"<<i->second->getName();

            std::vector<ChaosDatasetAttribute*>::iterator k= std::find(j->second.begin(),j->second.end(),i->second);
            if(k!=j->second.end()){
                j->second.erase(k);
            }
        }
        delete i->second;
        
        id2attr.erase(i);
        
    }
}


 std::vector<ChaosDatasetAttribute*> ChaosDatasetAttributeGroup::getAttrsByName(std::string name){
     return name2attrs[name];
 }
    
    
ChaosDatasetAttribute* ChaosDatasetAttributeGroup::getAttr(std::string path){
        if(id2attr.find(path)==id2attr.end())
            return NULL;
        return id2attr[path];
}



 void ChaosDatasetAttributeGroup::setTimeout(uint64_t inte){
     timeo =inte;
     for(std::map<std::string,ChaosDatasetAttribute* >::iterator i=id2attr.begin();i!=id2attr.end();i++){
         
         i->second->setTimeout(2*timeo/1000);
    }
    
 }

 void ChaosDatasetAttributeGroup::setInterval(uint64_t inte){
    interval=inte;
     
 }
 
