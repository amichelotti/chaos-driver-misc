/* 
 * File:   ChaosControllerGroup.cpp
 * Author: michelo
 * 
 * Created on September 2, 2015, 5:29 PM
 */

#include "ChaosControllerGroup.h"


ChaosControllerGroup::ChaosControllerGroup():ChaosController() {
 
}


ChaosControllerGroup::~ChaosControllerGroup() {
}

void ChaosControllerGroup::add(ChaosController& d){
    group.push_back(&d);
}

int ChaosControllerGroup::init(){
    
    for(ccgrp_t::iterator i=group.begin();i!=group.end();i++){
        if((*i)->ChaosController::init()!=0)
            return -1;
    }
    return 0;
}
int ChaosControllerGroup::stop(){
      
    for(ccgrp_t::iterator i=group.begin();i!=group.end();i++){
        if((*i)->ChaosController::stop()!=0)
            return -1;
    }
        return 0;

}
int ChaosControllerGroup::start(){
      
    for(ccgrp_t::iterator i=group.begin();i!=group.end();i++){
        if((*i)->ChaosController::start()!=0)
            return -1;
    }
        return 0;

}
int ChaosControllerGroup::deinit(){
      
    for(ccgrp_t::iterator i=group.begin();i!=group.end();i++){
        if((*i)->ChaosController::deinit()!=0)
            return -1;
    }
        return 0;

}
int ChaosControllerGroup::getState(){
    int ret=0,prev=-1;
    for(ccgrp_t::iterator i=group.begin();i!=group.end();i++){
        if((ret=(*i)->ChaosController::getState())<0)
            return ret;
        if((prev>-1)&& (prev!=ret) ){
            //the states are different
            return -2;
        }
        prev=ret;
    }
    return ret;
}

uint64_t ChaosControllerGroup::getTimeStamp(){
    uint64_t ret=0;
    if(group.size()==0) return 0;
    for(ccgrp_t::iterator i=group.begin();i!=group.end();i++){
        ret+=(*i)->ChaosController::getTimeStamp();
            
    }
    
    return ret/group.size();

}

int ChaosControllerGroup::executeCmd(command_t& cmd,bool wait){
    
    for(ccgrp_t::iterator i=group.begin();i!=group.end();i++){
        if((*i)->ChaosController::executeCmd(cmd,false)!=0){
            CTRLDBG_<<" executing command "<<cmd.
            return -1;
        }
        
    }
    if(wait){
        for(ccgrp_t::iterator i=group.begin();i!=group.end();i++){
             if((*i)->ChaosController::wait()!=0)
                return -1;
        }
    }
    return 0;

} 