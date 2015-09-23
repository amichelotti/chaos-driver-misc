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


 void ChaosController::setTimeout(uint64_t timeo_ms){
         controller->setRequestTimeWaith(timeo_ms);
         timeo=timeo_ms;
 }
    
int  ChaosController::init(){
    return controller->initDevice();
}
int  ChaosController::stop(){
    return controller->stopDevice();
}
int  ChaosController::start(){
    return controller->startDevice();
}
int  ChaosController::deinit(){
    return controller->deinitDevice();
}

int  ChaosController::getState(){
    
    if(controller->getState(state)==0){
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

int ChaosController::init(const char* p,uint32_t timeo_)  {
    path=p;
    state= chaos::CUStateKey::UNDEFINED;
    
    CTRLDBG_ << "init CU NAME:\""<<path<<"\"";
    if(chaos::ui::ChaosUIToolkit::getInstance()->getServiceState()==chaos::common::utility::service_state_machine::InizializableServiceType::IS_INITIATED){
        CTRLDBG_ << "UI toolkit already initialized";
    } else if(chaos::cu::ChaosCUToolkit::getInstance()->getServiceState()==chaos::common::utility::service_state_machine::StartableServiceType::SS_STARTED){
        CTRLDBG_ << "CU toolkit has started, initializing UI";
        chaos::ui::ChaosUIToolkit::getInstance()->getGlobalConfigurationInstance()->setConfiguration(chaos::cu::ChaosCUToolkit::getInstance()->getGlobalConfigurationInstance()->getConfiguration());
       chaos::ui::ChaosUIToolkit::getInstance()->init(NULL);
       
    }
    if(controller==NULL){
        controller= chaos::ui::HLDataApi::getInstance()->getControllerForDeviceID(path, timeo_);
    }
    if(controller==NULL){
        return -1;
     
    }
    controller->setRequestTimeWaith(timeo_);
    timeo=timeo_;
    if(getState()!=0){
        return -2;
    }
    return 0;
 }
int ChaosController::waitCmd(command_t&cmd){
    int ret;
    boost::posix_time::ptime start= boost::posix_time::microsec_clock::local_time();
    chaos::common::batch_command::CommandState command_state;
    command_state.command_id=cmd->command_id;
    do {
        if((ret=controller->getCommandState(command_state))!=0)
            return ret;
        
    } while((command_state.last_event!=chaos::common::batch_command::BatchCommandEventType::EVT_COMPLETED) && ((boost::posix_time::microsec_clock::local_time()-start).total_milliseconds()<timeo));
    
    if(command_state.last_event==chaos::common::batch_command::BatchCommandEventType::EVT_COMPLETED){
        return 0;
    }
    
    return -100;
}
int ChaosController::sendCmd(command_t& cmd,bool wait){
    if(cmd==NULL)
        return -2;
    CTRLAPP_ << "sending command \""<<cmd->alias <<"\" params:"<<cmd->param.getJSONString();
    if(controller->submitSlowControlCommand(cmd->alias,cmd->sub_rule,cmd->priority,cmd->command_id,0,cmd->scheduler_steps_delay,cmd->submission_checker_steps_delay,&cmd->param)!=0){
        CTRLERR_<<"error submitting";
        return -1;
    }
    
    if(wait){
        CTRLDBG_ << "waiting command id:"<<cmd->command_id;
        if(waitCmd(cmd)!=0){
           CTRLERR_<<"error waiting";

            return -3;
        }
       CTRLDBG_ << "command performed"; 
    }
    
    return 0;
}
int ChaosController::executeCmd(command_t& cmd,bool wait){
    return sendCmd(cmd,wait);
}

ChaosController::ChaosController(const char* p,uint32_t timeo_) throw (chaos::CException) {
    controller = NULL;
    if(init(p,timeo_)!=0){
               throw chaos::CException(-1, "cannot allocate controller for:"+ path,__FUNCTION__);

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

