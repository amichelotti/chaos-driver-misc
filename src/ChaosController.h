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
#define CTRLDBG_ LDBG_<< "[ "<<__PRETTY_FUNCTION__<<" ] ["<<getPath()<<"] "
#define CTRLERR_ LERR_ << "[ "<<__PRETTY_FUNCTION__<<" ] ["<<getPath()<<"] "
#define DEFAULT_TIMEOUT_FOR_CONTROLLER 10000000

class ChaosController{
    
    
private:
     chaos::ui::DeviceController* controller;
     std::string path;
     chaos::CUStateKey::ControlUnitState state;
     uint64_t timeo,schedule;
    // NetworkBroker *broker;
     //chaos::common::message::MDSMessageChannel *mdsChannel;
                //! Device MEssage channel to control via chaos rpc the device
     //chaos::common::message::DeviceMessageChannel *deviceChannel;
                //! The io driver for accessing live data of the device
	
     int forceState(int dstState);
  public:  
    
    struct command {
        std::string alias;
        chaos::common::data::CDataWrapper param;
        template<typename T>
        void addParameter(std::string name,T value){
            param.addValue(name,value);
        }
        uint32_t priority;
        uint64_t scheduler_steps_delay;
        uint32_t submission_checker_steps_delay;
        uint64_t command_id;
        chaos::common::batch_command::SubmissionRuleType::SubmissionRule sub_rule;
        command(){priority=50; scheduler_steps_delay=0;submission_checker_steps_delay=0;sub_rule=chaos::common::batch_command::SubmissionRuleType::SUBMIT_AND_Kill;}

    };
    
    typedef boost::shared_ptr<command> command_t;
    ChaosController();
    ChaosController(std::string path,uint32_t timeo=DEFAULT_TIMEOUT_FOR_CONTROLLER) throw (chaos::CException);

    ChaosController(const ChaosController& orig);
    virtual ~ChaosController();
    
   
    int init(std::string path, uint64_t timeo);
    
    virtual int init(int force=0);
    virtual int stop(int force=0);
    virtual int start(int force=0);
    virtual int deinit(int force=0);
    virtual int setSchedule(uint64_t us);
    /**
     * @return the state or negative if error
     *  
     */
    virtual int getState();
    virtual uint64_t getTimeStamp();
    /**
     * send a command
     * @param [in] cmd command to send
     * @param [in] wait if 1 wait for end
     * @return 0 on success
     */
    virtual int executeCmd(command_t& cmd,bool wait,uint64_t perform_at=0,uint64_t wait_for=0);
    
    /**
     set the timeout for the remote access
     @param timeo_us timeout in microseconds
     */
    virtual void setTimeout(uint64_t timeo_us);
    
    /**
     * wait for a command sent with the wait =0
     * @return 0 on success
     */
    int waitCmd(command_t&);
    
    /**
     * wait for last command sent with the wait =0
     * @return 0 on success
     */
    int waitCmd();


    command_t prepareCommand(std::string alias);
    template <typename T>
    void addParameter(command_t& cmd,std::string name,T value){
        cmd->param.addValue(name,value);
    }
    
    command_t last_cmd;
    std::string getPath(){return path;}
protected:
      int sendCmd(command_t& cmd,bool wait,uint64_t perform_at=0,uint64_t wait_for=0);
      
};

#endif	/* ChaosController_H */

