/* 
 * File:   ChaosController.h
 * Author: michelo
 *
 * Created on September 2, 2015, 5:29 PM
 */

#ifndef __ChaosController_H
#define	__ChaosController_H


#include <map>
#include <string>
#include <chaos/ui_toolkit/ChaosUIToolkit.h>
#include <boost/shared_ptr.hpp>
#include <common/misc/scheduler/SchedTimeElem.h>

#ifdef __CHAOS_UI__
#include <chaos/ui_toolkit/HighLevelApi/DeviceController.h>
#define UI_PREFIX chaos::ui
#else
#define UI_PREFIX chaos::metadata_service_client::node_controller

#include <ChaosMetadataServiceClient/ChaosMetadataServiceClient.h>

#endif
using namespace chaos::cu::data_manager;

#define CTRLAPP_ LAPP_ << "[ "<<__FUNCTION__<<"]"
#define CTRLDBG_ LDBG_<< "[ "<<__FUNCTION__<<"]"
#define CTRLERR_ LERR_ << "[ "<<__PRETTY_FUNCTION__<<"]"
#define DEFAULT_TIMEOUT_FOR_CONTROLLER 10000000
#define MDS_TIMEOUT 3000
#define MDS_STEP_TIMEOUT 1000
#define MDS_RETRY 3
#define HEART_BEAT_MAX 60000000
#define CALC_AVERAGE_REFRESH 5
#include <common/misc/data/core/DBbaseFactory.h>
#define DEFAULT_DBTYPE "cassandra"
#define DEFAULT_DBNAME "chaos"
#define DEFAULT_DBREPLICATION "2"
#define DEFAULT_PAGE 1000
#define MAX_CONCURRENT_QUERY 100
#define MAX_QUERY_ELEMENTS 1000
#define QUERY_PAGE_MAX_TIME 1000*60*1 // 1 min
#define CHECK_HB 10*1000*1000 //10 s

namespace driver{
    
    namespace misc{
class ChaosController: public ::common::misc::scheduler::SchedTimeElem {
    
    
private:
     chaos::common::message::MDSMessageChannel *mdsChannel;
     chaos::metadata_service_client::ChaosMetadataServiceClient*mds_client;
#ifdef __CHAOS_UI__
     chaos::ui::DeviceController* controller;
#else
     chaos::metadata_service_client::node_controller::CUController* controller;
#endif
     std::vector<std::string> mds_server_l;
     std::string path;
     chaos::CUStateKey::ControlUnitState state,last_state,next_state;
     uint64_t timeo,schedule;
     std::map<std::string,int> binaryToTranslate;
     uint64_t last_access,heart,reqtime,tot_us,naccess,refresh;
     int wostate;
   //  ::common::misc::data::DBbase* db;
    // NetworkBroker *broker;
     //chaos::common::message::MDSMessageChannel *mdsChannel;
                //! Device MEssage channel to control via chaos rpc the device
     //chaos::common::message::DeviceMessageChannel *deviceChannel;
                //! The io driver for accessing live data of the device

     std::string json_dataset;
     chaos::common::data::CDataWrapper data_out;
    std::map<int,std::string> cachedJsonChannels;



    uint32_t queryuid;
    boost::mutex iomutex;
    boost::mutex ioctrl;

    typedef struct {uint64_t qt;chaos::common::io::QueryCursor * qc;} qc_t;
    uint64_t offsetToTimestamp(const std::string&);
    typedef std::map<uint64_t,qc_t> query_cursor_map_t;
    query_cursor_map_t query_cursor_map;
     int forceState(int dstState);
     std::map<std::string,std::string> zone_to_cuname;
     std::map<std::string,std::string> class_to_cuname;

     std::vector<std::string> cachedChannel_v;

     void parseClassZone(ChaosStringVector&v);
     std::string vector2Json(ChaosStringVector& v);
     std::string map2Json(std::map<uint64_t,std::string> & v);
     std::string dataset2Var(chaos::common::data::CDataWrapper*c,std::string& name);
     void cleanUpQuery();
     void initializeClient();
     void deinitializeClient();
     uint64_t  last_input,last_system,last_health,last_custom,last_output,calc_freq,last_packid,last_ts;
  public:  

     typedef enum {
    	 CHAOS_DEV_OK=0,
		 CHAOS_DEV_UKN,
		 CHAOS_DEV_UNX,
    	 CHAOS_DEV_TIMEOUT=-100,
		 CHAOS_DEV_HB_TIMEOUT=-101,
		 CHAOS_DEV_RECOVERABLE_ERROR=-102,
		 CHAOS_DEV_FATAL_ERROR=-102,
		 CHAOS_DEV_INIT=-103, // error initializing
		 CHAOS_DEV_START=-104, // error starting
		 CHAOS_DEV_STOP=-105, // error stopping
		 CHAOS_DEV_DEINIT=-106, // error deinit
		 CHAOS_DEV_CMD=-107


     } chaos_controller_error_t;

     struct dev_info_status {
         char dev_status[256];
         char error_status[256];
         char log_status[256];
         chaos::common::data::CDataWrapper data_wrapper;
         dev_info_status();
         void status(chaos::CUStateKey::ControlUnitState deviceState);
         void append_log(std::string log);
         void append_error(std::string log);
         void reset();
         chaos::common::data::CDataWrapper * getData();
     };

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
        command(){priority=50; scheduler_steps_delay=0;submission_checker_steps_delay=0;sub_rule=chaos::common::batch_command::SubmissionRuleType::SUBMIT_AND_KILL;}

    };
    
    typedef boost::shared_ptr<command> command_t;
    dev_info_status bundle_state;
    ChaosController();
    ChaosController(std::string path,uint32_t timeo=DEFAULT_TIMEOUT_FOR_CONTROLLER);
    
    virtual uint64_t sched(uint64_t ts);
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
    virtual uint64_t getState( chaos::CUStateKey::ControlUnitState & state);
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
    /*
     * Interface for third party software http/labview
     * @param cmd command identifier, null if no command is required
     * @param args arguments in json format, null if no args are required
     * @param prio priority of the command
     * @param sched scheduling of the command
     * @param submission_mode submission mode of the command
     * @param channel dataset channel you want to get into json_buf (negative = last acquisition)
     * @param [out] json_buf returning dataset
     * @return negative if error, dataset size on success
     */

    chaos_controller_error_t get(const std::string&  cmd,char* args,int timeout, int prio,int sched,int submission_mode,int channel, std::string &json_buf);
    std::string  getJsonState();
    /*
     * Update device state
     * @return negative if error
     */
    int updateState();
    uint64_t checkHB();

protected:
      int sendCmd(command_t& cmd,bool wait,uint64_t perform_at=0,uint64_t wait_for=0);
      chaos::common::data::CDataWrapper*normalizeToJson(chaos::common::data::CDataWrapper*src,std::map<std::string,int>& list);

  	chaos::common::data::CDataWrapper*fetch(int channel);
  	const std::string& fetchJson(int channel);

    chaos::common::data::CDataWrapper*combineDataSets(std::map<int, chaos::common::data::CDataWrapper*>);


};
    }}
#endif	/* ChaosController_H */

