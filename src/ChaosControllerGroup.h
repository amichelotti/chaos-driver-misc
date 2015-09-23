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
template <class T>
class ChaosControllerGroup:public T{
    
    typedef  std::vector<T*> ccgrp_t;
    
    ccgrp_t group;
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
     int init(){
          
        for(typename ccgrp_t::iterator i=group.begin();i!=group.end();i++){
            if((*i)->T::init()!=0)
                return -1;
        }
        return 0;
    }
    int stop(){
      
    for(typename ccgrp_t::iterator i=group.begin();i!=group.end();i++){
        if((*i)->T::stop()!=0)
            return -1;
    }
        return 0;

}
    int start(){
      
    for(typename ccgrp_t::iterator i=group.begin();i!=group.end();i++){
        if((*i)->T::start()!=0)
            return -1;
    }
        return 0;

}
    int deinit(){
      
    for(typename ccgrp_t::iterator i=group.begin();i!=group.end();i++){
        if((*i)->T::deinit()!=0)
            return -1;
    }
        return 0;

}
int getState(){
    int ret=0,prev=-1;
    for(typename ccgrp_t::iterator i=group.begin();i!=group.end();i++){
        if((ret=(*i)->T::getState())<0)
            return ret;
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

int executeCmd(ChaosController::command_t& cmd,bool wait){
      
    for(typename ccgrp_t::iterator i=group.begin();i!=group.end();i++){
        if((*i)->T::executeCmd(cmd,wait)!=0)
            return -1;
    }
    return 0;

}
};

#endif	/* ChaosControllerGroupSyncronizer_H */

