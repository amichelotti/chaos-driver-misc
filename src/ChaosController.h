/* 
 * File:   ChaosController.h
 * Author: michelo
 *
 * Created on September 2, 2015, 5:29 PM
 */

#ifndef ChaosController_H
#define	ChaosController_H
#include <map>
#include <string>
#include <chaos/ui_toolkit/ChaosUIToolkit.h>
#include <boost/shared_ptr.hpp>
#include <chaos/ui_toolkit/HighLevelApi/DeviceController.h>

#define CTRLAPP_ LAPP_ << "[ "<<__FUNCTION__<<" ] ["<<getPath()<<"] "
#define CTRLDBG_ LDBG_<< "[ "<<__FUNCTION__<<" ] ["<<getPath()<<"] "
#define CTRLERR_ LERR_ << "[ "<<__FUNCTION__<<" ] ["<<getPath()<<"] "


class ChaosController{
    
    
private:
     chaos::ui::DeviceController* controller;
     std::string path;
     chaos::CUStateKey::ControlUnitState state;
     uint64_t timeo;

      
  public:  
    
    struct command {
        std::string alias;
        chaos::common::data::CDataWrapper param;
        template<typename T>
        void addParameter(const char*name,T value){
            param.addValue(name,value);
        }
        uint32_t priority;
        uint64_t scheduler_steps_delay;
        uint32_t submission_checker_steps_delay;
        uint64_t command_id;
        chaos::common::batch_command::SubmissionRuleType::SubmissionRule sub_rule;
        command(){priority=50; scheduler_steps_delay=0;submission_checker_steps_delay=0;sub_rule=chaos::common::batch_command::SubmissionRuleType::SUBMIT_AND_Stack;}

    };
    
    typedef boost::shared_ptr<command> command_t;
    ChaosController();
    ChaosController(const char* path,uint32_t timeo=5000) throw (chaos::CException);

    ChaosController(const ChaosController& orig);
    virtual ~ChaosController();
    
   
    int init(const char*path, uint32_t timeo);
    
    virtual int init();
    virtual int stop();
    virtual int start();
    virtual int deinit();
    /**
     * @return the state or negative if error
     * @return 
     */
    virtual int getState();
    virtual uint64_t getTimeStamp();
    /**
     * send a command
     * @param [in] cmd command to send
     * @param [in] wait if 1 wait for end
     * @return 0 on success
     */
    virtual int executeCmd(command_t& cmd,bool wait);
    
    /**
     set the timeout for the remote access
     @param timeo_ms update time
     */
    void setTimeout(uint64_t timeo_ms);
    
    /**
     * wait for a command sent with the wait =0
     * @return 0 on success
     */
    int waitCmd(command_t&);
    command_t prepareCommand(std::string alias);
    template <typename T>
    void addParameter(command_t& cmd,std::string name,T value){
        cmd->param.addValue(name,value);
    }
    
    std::string getPath(){return path;}
protected:
      int sendCmd(command_t& cmd,bool wait);
};

#endif	/* ChaosController_H */

