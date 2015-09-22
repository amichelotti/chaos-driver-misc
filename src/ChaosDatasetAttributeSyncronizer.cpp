/* 
 * File:   ChaosDatasetAttributeSyncronizer.cpp
 * Author: michelo
 * 
 * Created on September 2, 2015, 5:29 PM
 */

#include "ChaosDatasetAttributeSyncronizer.h"


ChaosDatasetAttributeSyncronizer::ChaosDatasetAttributeSyncronizer() {
    interval=20000;
    timeo = 2*interval;
    base_time=0;
 }


ChaosDatasetAttributeSyncronizer::~ChaosDatasetAttributeSyncronizer() {
}

void ChaosDatasetAttributeSyncronizer::add(ChaosDatasetAttribute& d){
    d.setTimeout(2*timeo/1000);
    set.push_back(&d);
}
uint64_t ChaosDatasetAttributeSyncronizer::sync(){
    int ok=0;
    base_time=0;
    uint64_t micro_spent=0;
    if(set.size()==0)return 0;
    boost::posix_time::ptime start=boost::posix_time::microsec_clock::local_time();
    for(std::vector<ChaosDatasetAttribute*>::iterator i=set.begin();i!=set.end();i++){
        (*i)->setUpdateMode(ChaosDatasetAttribute::NOTBEFORE,interval/2);
        (*i)->get(NULL);
        ChaosDatasetAttribute::datinfo& info=(*i)->getInfo();
        base_time=std::max(info.tstamp,base_time);
        ATTRDBG_<<"attribute \""<<(*i)->getPath()<<"\" time :"<<info.tstamp<<" base time:"<<base_time;
       
    }
    while((ok!=set.size()) && (((micro_spent=((boost::posix_time::microsec_clock::local_time()-start).total_microseconds()))<timeo))){
       ok=0;
       for(std::vector<ChaosDatasetAttribute*>::iterator i=set.begin();i!=set.end();i++){
        (*i)->get(NULL);
        ChaosDatasetAttribute::datinfo& info=(*i)->getInfo();
        base_time=std::max(info.tstamp,base_time);
        if((base_time-info.tstamp)<=interval){
            ok++;
            ATTRDBG_<<" at "<<micro_spent<< " attribute \""<<(*i)->getPath()<<"\" time :"<<info.tstamp<<" ALIGNED to:"<<base_time;     
        } else {
                ATTRDBG_<<" at "<<micro_spent<< " attribute \""<<(*i)->getPath()<<"\" time :"<<info.tstamp<<" DIFF FOR US:"<<(base_time-info.tstamp) <<" greater than:"<< interval;     
        }
        
    } 
    } 
    if(ok==set.size()){
        return base_time;
    }
    return 0;
}
 void ChaosDatasetAttributeSyncronizer::setTimeout(uint64_t inte){
     timeo =inte;
     for(std::vector<ChaosDatasetAttribute*>::iterator i=set.begin();i!=set.end();i++){
        (*i)->setTimeout(2*timeo/1000);
    }
 }

 void ChaosDatasetAttributeSyncronizer::setInterval(uint64_t inte){
    interval=inte;
     
 }
 