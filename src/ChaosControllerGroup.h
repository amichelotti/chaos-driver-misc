/* 
 * File:   ChaosControllerGroupSyncronizer.h
 * Author: michelo
 *
 * Created on September 2, 2015, 5:29 PM
 */

#ifndef ChaosControllerGroupSyncronizer_H
#define	ChaosControllerGroupSyncronizer_H
#include <map>
#include <string>
#include "ChaosController.h"

#define CTRLAPPG_ LAPP_ << "[ "<<__FUNCTION__<<" ] "
#define CTRLDBGG_ LDBG_<< "[ "<<__PRETTY_FUNCTION__<<" ]"
#define CTRLERRG_ LERR_ << "## [ "<<__PRETTY_FUNCTION__<<" ]"

template <class T>
class ChaosControllerGroup:public T{
    
    typedef  std::vector<T*> ccgrp_t;
    
    ccgrp_t group;
    
    T*error;
    
public:

    ChaosControllerGroup(){}
    virtual ~ChaosControllerGroup(){}
    /**
     * add an existing opject
     * 
     */
    void add(T& d){
        group.push_back(&d);
    }
     int init(int force=0){
         error=0;
        for(typename ccgrp_t::iterator i=group.begin();i!=group.end();i++){
            if((*i)->T::init(force)!=0){
                CTRLERRG_<<" initializing "<<(*i)->getPath();  
                error=*i;
                return -1;
            }
        }
        return 0;
    }
    int stop(int force=0){
      error=0;
    for(typename ccgrp_t::iterator i=group.begin();i!=group.end();i++){
        if((*i)->T::stop(force)!=0){
            CTRLERRG_<<" stopping "<<(*i)->getPath();  
            error=*i;
            return -1;
        }
    }
        return 0;

}
    int start(int force=0){
        error=0;
    for(typename ccgrp_t::iterator i=group.begin();i!=group.end();i++){
        if((*i)->T::start(force)!=0){
          CTRLERRG_<<" starting "<<(*i)->getPath();  
            error=*i;
            return -1;
        }
    }
        return 0;

}
    int deinit(int force=0){
        error=0;
    for(typename ccgrp_t::iterator i=group.begin();i!=group.end();i++){
        if((*i)->T::deinit(force)!=0){
           CTRLERRG_<<" deinitializing "<<(*i)->getPath();  
           error=*i;
            return -1;
        }
    }
        return 0;

}
    
    int setSchedule(uint64_t us){
        error=0;
        for(typename ccgrp_t::iterator i=group.begin();i!=group.end();i++){
           if((*i)->T::setSchedule(us)!=0){
              CTRLERRG_<<" setting "<<(*i)->getPath()<<" to:"<<us;   
            error=*i;
            return -1;
           }
    }
    }
int getState(){
    int ret=0,prev=-1;
    error=0;
    for(typename ccgrp_t::iterator i=group.begin();i!=group.end();i++){
        if((ret=(*i)->T::getState())<0){
            error=*i;
            CTRLERRG_<<" getting state  "<<(*i)->getPath();   

            return ret;
        }
        if((prev>-1)&& (prev!=ret) ){
            //the states are different
            return -2;
        }
        prev=ret;
    }
    return ret;
}

uint64_t getTimeStamp(){
    uint64_t ret=0;
    if(group.size()==0) return 0;
    for(typename ccgrp_t::iterator i=group.begin();i!=group.end();i++){
        ret+=(*i)->T::getTimeStamp();
            
    }
    
    return ret/group.size();

}

int executeCmd(ChaosController::command_t& cmd,bool wait,uint64_t perform_at=0,uint64_t wait_for=0){
    boost::posix_time::ptime start_cmd;
    uint64_t del;
    error=0;
    start_cmd=boost::posix_time::microsec_clock::local_time();
    for(typename ccgrp_t::iterator i=group.begin();i!=group.end();i++){
        del=(boost::posix_time::microsec_clock::local_time() -start_cmd).total_microseconds();
        if(wait_for>= del){
            wait_for -=del;
        }
        if((*i)->T::executeCmd(cmd,false,perform_at,wait_for)!=0){
            CTRLERRG_<<" executing command  "<<(*i)->getPath();   
            error=*i;
            return -1;
        }
        
    }
    for(typename ccgrp_t::iterator i=group.begin();i!=group.end();i++){
        if((*i)->T::waitCmd()!=0){
            CTRLERRG_<<" waiting command  "<<(*i)->getPath();   
            error=*i;
            return -1;
        }
    }
    
    return 0;

}

T* getErrorCU(){return error;}
};

#endif	/* ChaosControllerGroupSyncronizer_H */

