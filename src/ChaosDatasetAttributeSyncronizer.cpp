/* 
 * File:   ChaosDatasetAttributeSyncronizer.cpp
 * Author: michelo
 * 
 * Created on September 2, 2015, 5:29 PM
 */

#include <vector>

#include "ChaosDatasetAttributeSyncronizer.h"

bool compareSet(ChaosDatasetAttributeSyncronizer::attr_t const& x,ChaosDatasetAttributeSyncronizer::attr_t const& y){
     return (x.second.tlastread - x.second.tchanged)>(y.second.tlastread - y.second.tchanged);
     
}
ChaosDatasetAttributeSyncronizer::ChaosDatasetAttributeSyncronizer() {
    interval=20000;
    timeo = 2*interval;
    base_time=0;
 }


ChaosDatasetAttributeSyncronizer::~ChaosDatasetAttributeSyncronizer() {
    ATTRDBG_<<" Deleting ChaosDatasetAttributeSyncronizer and all the content";
    for(cuset_t::iterator i=set.begin();i!=set.end();i++){
        delete (i->first);
       
    }
    set.clear();
    id2attr.clear();
    name2attrs.clear();
}

ChaosDatasetAttributeSyncronizer::syncInfo::syncInfo(){
    ChaosDatasetAttributeSyncronizer::syncInfo::tchanged=0;
    ChaosDatasetAttributeSyncronizer::syncInfo::tlastread=0;
    ChaosDatasetAttributeSyncronizer::syncInfo::changes=0;
}

void ChaosDatasetAttributeSyncronizer::add(ChaosDatasetAttribute& d){
    add(&d);
}
void ChaosDatasetAttributeSyncronizer::add(ChaosDatasetAttribute* d){
    if(id2attr.find(d->getPath())==id2attr.end()){
        ATTRDBG_<<"adding new dataset attribute <<"<<d->getPath();

        d->setTimeout(2*timeo/1000);
        set.push_back(std::make_pair(d,ChaosDatasetAttributeSyncronizer::syncInfo()));
        id2attr.insert(std::make_pair(d->getPath(),d));
        name2attrs[d->getName()].push_back(d);
    }
}

ChaosDatasetAttribute* ChaosDatasetAttributeSyncronizer::add(std::string path){
    ChaosDatasetAttribute*ret=NULL;
    boost::mutex::scoped_lock sl(lock_sync);
        

    if(id2attr.find(path)==id2attr.end()){
        ChaosDatasetAttribute*d = new ChaosDatasetAttribute(path);
        add(d);
        ret = d;
    } 
    return ret;
}

std::vector<ChaosDatasetAttribute*> ChaosDatasetAttributeSyncronizer::getAttributes(){
    std::vector<ChaosDatasetAttribute*> ret;
    cuset_t::iterator i;
    for(i=set.begin();i!=set.end();i++){
        ret.push_back(i->first);
    }
    return ret;

}
void ChaosDatasetAttributeSyncronizer::remove(std::string path){
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


 std::vector<ChaosDatasetAttribute*> ChaosDatasetAttributeSyncronizer::getAttrsByName(std::string name){
     return name2attrs[name];
 }
    
    
ChaosDatasetAttribute* ChaosDatasetAttributeSyncronizer::getAttr(std::string path){
        if(id2attr.find(path)==id2attr.end())
            return NULL;
        return id2attr[path];
}



int ChaosDatasetAttributeSyncronizer::sortedFetch(uint64_t&max_age){
    int changed=0;
    
    std::sort(set.begin(),set.end(),compareSet);
    for(cuset_t::iterator i=set.begin();i!=set.end();i++){
        uint64_t age;
 /*       if(i->second.changes>1)
            continue;
  */
        i->first->setUpdateMode(ChaosDatasetAttribute::EVERYTIME,0);
        i->first->get(NULL);
        ChaosDatasetAttribute::datinfo& info=i->first->getInfo();
        i->second.tlastread=info.tstamp;
        age=info.tstamp-i->second.tchanged;
        max_age=std::max(max_age,age);
        if(age > 0){      
            i->second.changes++;
            changed++;
            ATTRDBG_<<"Changed \""<<i->first->getPath()<<"\" at :"<<info.tstamp<<  " ms age:"<<age << " ms changes:"<<i->second.changes; 
            i->second.tchanged = info.tstamp;
        } else {
            if(i->second.changes){
                changed++;
  //              if(i->second.changes>1)
    //                ATTRDBG_<<"Warning \""<<i->first->getPath()<<"\" Re-read more than one time at:"<<info.tstamp<<  " age:"<<age << " ms changes:"<<i->second.changes; 

            }
        }
    }
    return changed;
}

int64_t ChaosDatasetAttributeSyncronizer::sync(){
    int ret;
    uint64_t max_age=0;
    uint64_t micro_spent=0;

    boost::posix_time::ptime start=boost::posix_time::microsec_clock::local_time();

    ATTRDBG_<<"syncing...";
    do{
        ret=sortedFetch(max_age);
        //if(ret)
          //  ATTRDBG_<<"Sync "<<ret<<"/"<<set.size()<<" attributes, max age:"<<max_age;
    } while((ret<set.size())&&(((micro_spent=((boost::posix_time::microsec_clock::local_time()-start).total_microseconds()))<timeo)));
    
    for(cuset_t::iterator i=set.begin();i!=set.end();i++){
                i->second.changes=0;
    }
    
    if(ret==set.size()){
            
             
        ATTRDBG_<<"syncing OK after "<<micro_spent<<"us"<< " Max age:"<<max_age;
    
        return max_age;
    }
    ATTRDBG_<<"TIMEOUT :"<<micro_spent;
    return -1;
}

 void ChaosDatasetAttributeSyncronizer::setTimeout(uint64_t inte){
     timeo =inte;
     for(cuset_t::iterator i=set.begin();i!=set.end();i++){
        i->first->setTimeout(2*timeo/1000);
    }
 }

 void ChaosDatasetAttributeSyncronizer::setInterval(uint64_t inte){
    interval=inte;
     
 }
 