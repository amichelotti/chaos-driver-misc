/* 
 * File:   ChaosController.cpp
 * Author: michelo
 * 
 * Created on September 2, 2015, 5:29 PM
 */

#include "ChaosController.h"
#include <chaos/common/exception/CException.h>
#include <chaos/ui_toolkit/ChaosUIToolkit.h>
#include <chaos/cu_toolkit/ChaosCUToolkit.h>
#include <chaos/ui_toolkit/LowLevelApi/LLRpcApi.h>

 void ChaosController::setTimeout(uint64_t timeo_us){
         controller->setRequestTimeWaith(timeo_us/1000);
         timeo=timeo_us;
 }

int ChaosController::forceState(int dstState){
    int currState=-100,oldstate;
    boost::posix_time::ptime start;
    int retry=10;
    
    do{
        oldstate=currState;
        currState=getState();
        CTRLDBG_ << "Current state:"<<currState<<" destination state:"<<dstState;
        if(currState!=oldstate){
            start=boost::posix_time::microsec_clock::local_time();
        }
        if(currState<0)
            return currState;
        
        
          switch(currState){
              case chaos::CUStateKey::DEINIT:
                controller->initDevice();
            break;
            case chaos::CUStateKey::INIT:
                switch(dstState){
                    case chaos::CUStateKey::DEINIT:
                         controller->deinitDevice();
                         break;
                    case chaos::CUStateKey::START:
                    case chaos::CUStateKey::STOP:
                        controller->startDevice();
                        break;
                        
                }
                
                
            break;
            
            case chaos::CUStateKey::START:
                switch(dstState){
                    case chaos::CUStateKey::INIT:
                    case chaos::CUStateKey::STOP:
                        controller->stopDevice();
                        break;
                    case chaos::CUStateKey::DEINIT:
                       controller->initDevice();

                        break;

                            
                }
            break;
                
                
            case chaos::CUStateKey::STOP:
                switch(dstState){
                    case chaos::CUStateKey::DEINIT:
                    case chaos::CUStateKey::INIT:
                         controller->deinitDevice();
                         break;
                    case chaos::CUStateKey::START:
                        controller->startDevice();
                        break;
                        
                }
                
                
            break;
           default:
                controller->deinitDevice();
                controller->initDevice();
/*
                switch(dstState){
                    case chaos::CUStateKey::DEINIT:
                        break;
                    case chaos::CUStateKey::INIT:
                         break;
                    case chaos::CUStateKey::START:
                        controller->startDevice();
                        break;
                     case chaos::CUStateKey::STOP:
                        controller->stopDevice();
                        break;
                }*/
        }
          if((boost::posix_time::microsec_clock::local_time() - start).total_microseconds()> timeo){
              retry --;
            CTRLERR_ << "Timeout of "<<timeo <<" us elapsed:"<<(boost::posix_time::microsec_clock::local_time() - start).total_microseconds()<< "  Retry:"<<retry;
             start=boost::posix_time::microsec_clock::local_time();
            
        }
    } while((currState!=dstState)&& (retry>0));
        
     
    if(retry==0){
        CTRLERR_ << "Not Responding";
        return -100;
        
    }
       
    return 0;
    
}

int  ChaosController::init(int force){
   
    if(force){
        return forceState(chaos::CUStateKey::INIT);
    }
    return controller->initDevice();
}
int  ChaosController::stop(int force){
    if(force){
        return forceState(chaos::CUStateKey::STOP);
    }
    return controller->stopDevice();
}
int  ChaosController::start(int force){
    if(force){
        return forceState(chaos::CUStateKey::START);
    }
    return controller->startDevice();
}
int  ChaosController::deinit(int force){
    if(force){
        return forceState(chaos::CUStateKey::DEINIT);
    }
    return controller->deinitDevice();
}

int  ChaosController::getState(){
    
    if(controller->getState(state)>0){
        return state;
    }
    return -1;
}

uint64_t  ChaosController::getTimeStamp(){
    uint64_t ret;
    controller->getTimeStamp(ret);
    return ret;
}
ChaosController::command_t  ChaosController::prepareCommand(std::string alias){
    ChaosController::command_t cmd=boost::shared_ptr<command>(new command());
    cmd->alias = alias;
    return cmd;
}
int ChaosController::setSchedule(uint64_t us){
    
    schedule=us;
    return controller->setScheduleDelay(us);
}

int ChaosController::init(std::string p,uint64_t timeo_)  {
    path=p;
    state= chaos::CUStateKey::UNDEFINED;
    schedule=0;
    CTRLDBG_ << "init CU NAME:\""<<path<<"\"";
   /* CTRLDBG_<<" UI CONF:"<<chaos::ui::ChaosUIToolkit::getInstance()->getGlobalConfigurationInstance()->getConfiguration()->getJSONString();
    CTRLDBG_<<" CU CONF:"<<chaos::cu::ChaosCUToolkit::getInstance()->getGlobalConfigurationInstance()->getConfiguration()->getJSONString();
    CTRLDBG_<<" CU STATE:"<<chaos::cu::ChaosCUToolkit::getInstance()->getServiceState();
    CTRLDBG_<<" UI STATE:"<<chaos::ui::ChaosUIToolkit::getInstance()->getServiceState();
    */
    if(chaos::ui::ChaosUIToolkit::getInstance()->getServiceState()==chaos::common::utility::service_state_machine::InizializableServiceType::IS_INITIATED){
        CTRLDBG_ << "UI toolkit already initialized";
    } else if((chaos::cu::ChaosCUToolkit::getInstance()->getServiceState()==chaos::common::utility::service_state_machine::StartableServiceType::SS_STARTED) || (chaos::cu::ChaosCUToolkit::getInstance()->getServiceState()==chaos::common::utility::service_state_machine::InizializableServiceType::IS_INITIATED)){
        CTRLDBG_ << "CU toolkit has started, initializing UI";
        chaos::ui::ChaosUIToolkit::getInstance()->getGlobalConfigurationInstance()->setConfiguration(chaos::cu::ChaosCUToolkit::getInstance()->getGlobalConfigurationInstance()->getConfiguration());
     //  chaos::ui::ChaosUIToolkit::getInstance()->init(NULL);   
    }
    //chaos::common::utility::InizializableService::initImplementation(chaos::common::async_central::AsyncCentralManager::getInstance(), 0, "AsyncCentralManager", __PRETTY_FUNCTION__);

    //    chaos::ui::LLRpcApi::getInstance()->init();
    //chaos::ui::ChaosUIToolkit::getInstance()->init(NULL);
    if(controller==NULL){
        try {
            controller= chaos::ui::HLDataApi::getInstance()->getControllerForDeviceID(path, timeo_/1000);
        } catch (chaos::CException &e){
            CTRLERR_<<"Exception during get controller for device:"<<e.what();
            return -3;
        }
    }
    if(controller==NULL){
        return -1;
     
    }
    controller->setRequestTimeWaith(timeo_/1000);
    timeo=timeo_;
    if(getState()<0){
         CTRLERR_<<"Exception during getting state for device:"<<path;
        return -2;
    }
    return 0;
 }

int ChaosController::waitCmd(){
    return waitCmd(last_cmd);
}
int ChaosController::waitCmd(command_t&cmd){
    int ret;
    boost::posix_time::ptime start= boost::posix_time::microsec_clock::local_time();
    chaos::common::batch_command::CommandState command_state;
    command_state.command_id=cmd->command_id;
    do {
        if((ret=controller->getCommandState(command_state))!=0){
            return ret;
        }
      
    } while((command_state.last_event!=chaos::common::batch_command::BatchCommandEventType::EVT_COMPLETED) && (command_state.last_event!=chaos::common::batch_command::BatchCommandEventType::EVT_RUNNING)  && ((boost::posix_time::microsec_clock::local_time()-start).total_microseconds()<timeo));
    
    CTRLDBG_ <<" Command state last event:"<<command_state.last_event;
    if((command_state.last_event==chaos::common::batch_command::BatchCommandEventType::EVT_COMPLETED)||(command_state.last_event==chaos::common::batch_command::BatchCommandEventType::EVT_RUNNING)){
        return 0;
    }
    
    return -100;
}
int ChaosController::sendCmd(command_t& cmd,bool wait,uint64_t perform_at,uint64_t wait_for){
    if(cmd==NULL)
        return -2;
    if(perform_at){
        cmd->param.addInt64Value("perform_at",perform_at);
        CTRLDBG_ << "command will be performed at "<< perform_at; 

    } else if(wait_for){
        cmd->param.addInt64Value("wait_for",wait_for);
        CTRLDBG_ << "command will be performed in "<< wait_for<<" us"; 

        
    }
    CTRLAPP_ << "sending command \""<<cmd->alias <<"\" params:"<<cmd->param.getJSONString();
    if(controller->submitSlowControlCommand(cmd->alias,cmd->sub_rule,cmd->priority,cmd->command_id,0,cmd->scheduler_steps_delay,cmd->submission_checker_steps_delay,&cmd->param)!=0){
        CTRLERR_<<"error submitting";
        return -1;
    }
    
   
    
    return 0;
}
int ChaosController::executeCmd(command_t& cmd,bool wait,uint64_t perform_at,uint64_t wait_for){
    int ret=sendCmd(cmd,wait,perform_at,wait_for);
    if(ret!=0)
        return ret;
    last_cmd=cmd;
    if(wait){
        CTRLDBG_ << "waiting command id:"<<cmd->command_id;
        if((ret=waitCmd(cmd))!=0){
           CTRLERR_<<"error waiting ret:"<<ret;

                return ret;
        }
       CTRLDBG_ << "command performed"; 
    }
    return ret;
}

ChaosController::ChaosController(std::string p,uint32_t timeo_) throw (chaos::CException) {
    int ret;
    controller = NULL;
   
    if((ret=init(p,timeo_))!=0){
               throw chaos::CException(ret, "cannot allocate controller for:"+ path + " check if exists",__FUNCTION__);

    }
 
 }
ChaosController::ChaosController(const ChaosController& orig) {
}
ChaosController::ChaosController() {
    controller=NULL;
    state= chaos::CUStateKey::UNDEFINED;

}
ChaosController::~ChaosController() {
    
}

