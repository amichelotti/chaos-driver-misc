/* 
 * File:   ChaosController.h
 * Author: michelo
 *
 * Created on September 2, 2015, 5:29 PM
 */

#ifndef __ChaosController_H
#define __ChaosController_H

//#include <chaos/common/chaos_constants.h>
#include <chaos/common/data/DatasetDB.h>

#include <chaos/cu_toolkit/data_manager/KeyDataStorage.h>

#include <common/misc/scheduler/SchedTimeElem.h>
#include <chaos/common/data/CDataWrapper.h>
#include <chaos/common/io/IODataDriver.h>

#include <chaos_service_common/ChaosManager.h>
#include <chaos/common/batch_command/BatchCommandTypes.h>
#define CTRLAPP_ LAPP_ << "[ " << __FUNCTION__ << "]"
//#define CTRLDBG_ LDBG_ << "[ " << __FUNCTION__ << "]"
//#define CTRLERR_ LERR_ << "[ " << __PRETTY_FUNCTION__ << "]"
#define MDS_TIMEOUT 10000
#define MDS_STEP_TIMEOUT 1000
#define MDS_RETRY 3
#define HEART_BEAT_MAX 60000000
#define CALC_AVERAGE_REFRESH 5
#define DEFAULT_DBTYPE "cassandra"
#define DEFAULT_DBNAME "chaos"
#define DEFAULT_DBREPLICATION "2"
#define DEFAULT_PAGE 1000
#define MAX_CONCURRENT_QUERY 100
#define MAX_QUERY_ELEMENTS 10000
#define QUERY_PAGE_MAX_TIME 1000 * 60 * 2 // 1 min
#define CHECK_HB 10 * 1000 * 1000         //10 s
#define HALF_HEALT_REFRESH 2500
namespace chaos
{
namespace metadata_service_client
{
class ChaosMetadataServiceClient;
}
namespace common
{
namespace message
{
class MDSMessageChannel;
};
} // namespace common
} // namespace chaos
namespace driver
{

namespace misc
{
class ChaosController /*: public ::common::misc::scheduler::SchedTimeElem*/
{

  private:
    static chaos::common::message::MDSMessageChannel *mdsChannel;
    static chaos::service_common::ChaosManager*manager;
    //chaos::metadata_service_client::ChaosMetadataServiceClient *mds_client;
    static chaos::common::io::IODataDriver* live_driver;
    std::vector<std::string> mds_server_l;
    std::string path;
    chaos::common::data::DatasetDB datasetDB;
  //  chaos::common::data::VectorCDWShrdPtr cached_channels;
    chaos::common::cache_system::CacheDriver* cache_driver;
    chaos::service_common::persistence::data_access::AbstractPersistenceDriver* persistence_driver;

    chaos::CUStateKey::ControlUnitState state, last_state, next_state;
    uint64_t timeo, schedule,update_all_channels_ts;
    std::map<std::string, int> binaryToTranslate;
    uint64_t last_access, heart, reqtime, tot_us, naccess, refresh;
    int wostate;
    //  ::common::misc::data::DBbase* db;
    // NetworkBroker *broker;
    //chaos::common::message::MDSMessageChannel *mdsChannel;
    //! Device MEssage channel to control via chaos rpc the device
    //chaos::common::message::DeviceMessageChannel *deviceChannel;
    //! The io driver for accessing live data of the device
    void updateCacheLive(const chaos::common::data::CDataWrapper&);
    std::string json_dataset;
    chaos::common::data::CDataWrapper data_out;
    int32_t max_cache_duration_ms;
    std::map<int, std::string> cachedJsonChannels;
    std::map<int, int64_t> cachedJsonChannelsTS;

    uint32_t queryuid;
    ChaosMutex iomutex;
    ChaosMutex ioctrl;

    typedef struct
    {
        uint64_t qt;
        chaos::common::io::QueryCursor *qc;
    } qc_t;
    uint64_t offsetToTimestamp(const std::string &);
    typedef std::map<uint64_t, qc_t> query_cursor_map_t;
    query_cursor_map_t query_cursor_map;
    int forceState(int dstState);
    std::map<std::string, std::string> zone_to_cuname;
    std::map<std::string, std::string> class_to_cuname;

    void parseClassZone(ChaosStringVector &v);
    std::string vector2Json(ChaosStringVector &v);
    std::string map2Json(std::map<uint64_t, std::string> &v);
    std::string dataset2Var(chaos::common::data::CDataWrapper *c, std::string &name);
    void cleanUpQuery();
    void initializeClient();
    void deinitializeClient();
    uint64_t last_health_ts,delta_update;
    //uint64_t last_ts[DPCK_LAST_DATASET_INDEX + 1], ;
    //uint64_t last_pckid[DPCK_LAST_DATASET_INDEX + 1];
    std::map<std::string, chaos::common::data::RangeValueInfo> attributeValueMap;
    std::vector<chaos::common::data::RangeValueInfo> getDeviceValuesInfo();
    void initializeAttributeIndexMap();

    void update();
    std::vector<std::string> filterByState(const std::vector<std::string> &CUNAMEs,const std::string& state) ;

  public:
    typedef chaos::cu::data_manager::KeyDataStorageDomain DatasetDomain;

    typedef enum
    {
        CHAOS_DEV_OK = 0,
        CHAOS_DEV_UKN,
        CHAOS_DEV_UNX,
        CHAOS_DEV_TIMEOUT = -100,
        CHAOS_DEV_HB_TIMEOUT = -101,
        CHAOS_DEV_RECOVERABLE_ERROR = -102,
        CHAOS_DEV_FATAL_ERROR = -102,
        CHAOS_DEV_INIT = -103,   // error initializing
        CHAOS_DEV_START = -104,  // error starting
        CHAOS_DEV_STOP = -105,   // error stopping
        CHAOS_DEV_DEINIT = -106, // error deinit
        CHAOS_DEV_CMD = -107

    } chaos_controller_error_t;

    struct dev_info_status
    {
        std::stringstream dev_status;
        std::stringstream error_status;
        std::stringstream log_status;
        chaos::common::data::CDataWrapper data_wrapper;
        dev_info_status();
        void status(chaos::CUStateKey::ControlUnitState deviceState);
        void append_log(const std::string &log);
        void append_error(const std::string &log);
        void reset();
        chaos::common::data::CDataWrapper *getData();
    };

    struct command
    {
        std::string alias;
        chaos::common::data::CDataWrapper param;
        template <typename T>
        void addParameter(std::string name, T value)
        {
            param.append(name, value);
        }
        uint32_t priority;
        uint64_t scheduler_steps_delay;
        uint32_t submission_checker_steps_delay;
        uint64_t command_id;
        chaos::common::batch_command::SubmissionRuleType::SubmissionRule sub_rule;
        command()
        {
            priority = 50;
            scheduler_steps_delay = 0;
            submission_checker_steps_delay = 0;
            sub_rule = chaos::common::batch_command::SubmissionRuleType::SUBMIT_AND_KILL;
        }
    };

    typedef boost::shared_ptr<command> command_t;
    dev_info_status bundle_state;
    ChaosController();
    ChaosController(std::string path, uint32_t timeo = chaos::RpcConfigurationKey::GlobalRPCTimeoutinMSec);

    //virtual uint64_t sched(uint64_t ts);
    virtual ~ChaosController();
    std::vector<std::string> searchAlive(const std::string& name="",const std::string& what="cu");
    int initDevice(const std::string &dev = "");
    int stopDevice(const std::string &dev = "");
    int startDevice(const std::string &dev = "");
    int deinitDevice(const std::string &dev = "");
    int loadDevice(const std::string &dev = "");
    int unloadDevice(const std::string &dev = "");
    int setAttributeToValue(const char *attributeName, const char *attributeValue, const std::string &cuname = "");

    chaos::common::data::CDWUniquePtr executeAPI(const std::string&group,const std::string&name,chaos::common::data::CDWUniquePtr& msg,int&err);

    int init(const std::string &p, uint64_t timeo);
    virtual int init(int force = 0);
    virtual int stop(int force = 0);
    virtual int start(int force = 0);
    virtual int deinit(int force = 0);
    virtual int setSchedule(uint64_t us, const std::string &cuname = "");
    /**
     * @return the state or negative if error
     *  
     */
    uint64_t getState(chaos::CUStateKey::ControlUnitState &state, const std::string &dev = "");

    uint64_t getCachedHealthTimeStamp(){return last_health_ts;}
    /**
     * Return the timestamp of a given CU and dataset, defaults itself and output
    */
    uint64_t getTimeStamp(const std::string &CUNAME = "", int domain = chaos::DataPackCommonKey::DPCK_DATASET_TYPE_HEALTH);
    /**
     * send a command
     * @param [in] cmd command to send
     * @param [in] wait if 1 wait for end
     * @return 0 on success
     */
    virtual int executeCmd(command_t &cmd, bool wait, uint64_t perform_at = 0, uint64_t wait_for = 0);

    /**
     set the timeout for the remote access
     @param timeo_us timeout in microseconds
     */
    virtual void setTimeout(uint64_t timeo_us);

    /**
     * wait for a command sent with the wait =0
     * @return 0 on success
     */
    //int waitCmd(command_t&);

    /**
     * wait for last command sent with the wait =0
     * @return 0 on success
     */
    //int waitCmd();
  int searchNodeInt(const std::string& unique_id_filter,
                               chaos::NodeType::NodeSearchType node_type_filter,
                               bool alive_only,
                               unsigned int last_node_sequence_id,
                               unsigned int page_length,
                               unsigned int& num_of_page,
                               ChaosStringVector& node_found,
                               uint32_t millisec_to_wait,
                               const std::string& impl);
int searchNode(const std::string& unique_id_filter,
                               chaos::NodeType::NodeSearchType node_type_filter,
                               bool alive_only,
                               unsigned int start_page,
                               unsigned int page_length,
                               unsigned int& num_of_page,
                               ChaosStringVector& node_found,
                               uint32_t millisec_to_wait=5000,const std::string& impl="",const std::string& state="");
int searchNode(const std::string& unique_id_filter,
                               const std::string& node_type_filter,
                               bool alive_only,
                               unsigned int start_page,
                               unsigned int page_length,
                               unsigned int& num_of_page,
                               ChaosStringVector& node_found,
                               uint32_t millisec_to_wait=5000,const std::string& impl="",const std::string& state="");
    void executeTimeIntervalQuery(DatasetDomain domain,
                                   uint64_t start_ts,
                                   uint64_t end_ts,
                                   chaos::common::io::QueryCursor **query_cursor,
                                   const std::string&name="",
                                   uint32_t page= DEFAULT_PAGE_LEN);

    void executeTimeIntervalQuery(DatasetDomain domain,
                                   uint64_t start_ts,
                                   uint64_t end_ts,
                                   const ChaosStringSet &meta_tags,
                                   chaos::common::io::QueryCursor **query_cursor,
                                   const ChaosStringSet &projection=ChaosStringSet(),

                                   const std::string&name="",

                                   uint32_t page= DEFAULT_PAGE_LEN);
    void executeTimeIntervalQuery(const DatasetDomain domain,
                                                   const uint64_t start_ts,
                                                   const uint64_t end_ts,
                                                   const uint64_t seqid,
                                                   const uint64_t runid,
                                                   chaos::common::io::QueryCursor **query_cursor,
                                                   const std::string&name="",

                                                   const uint32_t page_len= DEFAULT_PAGE_LEN);
    void executeTimeIntervalQuery(const DatasetDomain domain,
                                  const uint64_t start_ts,
                                  const uint64_t end_ts,
                                  const uint64_t seqid,
                                  const uint64_t runid,
                                  const ChaosStringSet &meta_tags,

                                  chaos::common::io::QueryCursor **query_cursor,
                                  const ChaosStringSet &projection=ChaosStringSet(),

                                  const std::string&name="",

                                  const uint32_t page_len= DEFAULT_PAGE_LEN);
    //! restore from a tag a dataset associated to a key
    int createNewSnapshot(const std::string &snapshot_tag,
                          const std::vector<std::string> &other_snapped_device);
    //!delete the snapshot
    int deleteSnapshot(const std::string &snapshot_tag);

    //!return the snapshot list for device controlled by this isntance
    int getSnapshotList(ChaosStringVector &snapshot_list);

    int searchNodeForSnapshot(const std::string &snapshot_tag,ChaosStringVector &snapshot_list);

    //!return the dataset of the specified cu associated to a given snapshot
    // 
    chaos::common::data::CDWUniquePtr getSnapshotDataset(const std::string&snapname,const std::string& cuname);
    
    command_t prepareCommand(std::string alias);
    template <typename T>
    void addParameter(command_t &cmd, std::string name, T value)
    {
        cmd->param.append(name, value);
    }

    command_t last_cmd;
    std::string getPath() { return path; }
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

    chaos_controller_error_t get(const std::string &cmd, char *args, int timeout, int prio, int sched, int submission_mode, int channel, std::string &json_buf);
    std::string getJsonState();
    /*
     * Update device state
     * @return negative if error
     */
    int updateState();
    uint64_t checkHB();
    chaos::common::data::CDWUniquePtr fetch(int channel);
    const std::string fetchJson(int channel);
    /*
     * perform a history query from start to end, return a vector of result
     * @param[in] start epoch timestamp in ms or string offset start search
     *  @param[in] end epoch timestamp in ms or string offset end search
     *  @param[out] res result
     *  @param[in]  page page len =o if full search
     *  @return 0 if success and end search, >0 is an uid to be use with next to get remaining results, <0 an error occurred
     * */
    int32_t queryHistory(const std::string& start, const std::string& end, uint64_t& runid, uint64_t& seqid, const std::vector<std::string>& tags, int channel, chaos::common::data::VectorCDWShrdPtr& res, const ChaosStringSet& projection, int page);

    int32_t queryHistory(const std::string &start, const std::string &end, int channel, chaos::common::data::VectorCDWShrdPtr &res,  const ChaosStringSet& projection=ChaosStringSet(), int page = 0);

    int32_t queryNext(int32_t id, chaos::common::data::VectorCDWShrdPtr &res);
    bool queryHasNext(int32_t id);
    int getSnapshotsofCU(const std::string &cuname, std::map<uint64_t, std::string> &res);
    /*void dumpHistoryToTgz(const std::string& fname,const std::string& start,const std::string& end,int channel,std::string tagname);*/
    uint64_t lastAccess() { return reqtime; }
    /**
      * Return one live channel corresponding to a CUname and a domain
     */
    chaos::common::data::CDWShrdPtr getLiveChannel(const std::string &CUNAME = "", int domain = chaos::DataPackCommonKey::DPCK_DATASET_TYPE_HEALTH);
    /**
     * Return an array of live channels referring to an array of keys 
     * */
    chaos::common::data::VectorCDWShrdPtr getLiveChannel(const std::vector<std::string> &keys);


    /**
     * Return an array of live channels referring to an array of CUnames of a particular domain
     * */
    chaos::common::data::VectorCDWShrdPtr getLiveChannel(const std::vector<std::string> &CUNAMEs, int domain);
    /**
     * Return all the available live channels of a given CUNAME
     * */
    chaos::common::data::VectorCDWShrdPtr getLiveAllChannels(const std::string &CUNAME = "");

    chaos::common::data::VectorCDWShrdPtr getLiveChannel(chaos::common::data::CMultiTypeDataArrayWrapper *keys, int domain = chaos::DataPackCommonKey::DPCK_DATASET_TYPE_HEALTH);
    void releaseQuery(chaos::common::io::QueryCursor *query_cursor);
    int restoreDeviceToTag(const std::string &restore_tag);
    chaos::common::data::VectorCDWUniquePtr getNodeInfo(const std::string& search,const std::string& what="agent",bool alive=true);
    
    chaos::common::data::CDWUniquePtr sendRPCMsg(const std::string& search,const std::string&rpcmsg, chaos::common::data::CDWUniquePtr datapack,const std::string& what="agent",bool alive=true);
chaos::common::data::CDWUniquePtr sendRPCMsg(const std::string& uid,const std::string& domain,const std::string&rpcmsg,chaos::common::data::CDWUniquePtr& data_pack);

    chaos::common::data::CDWUniquePtr getNodeDesc(const std::string& search,int&err);

    chaos::common::data::CDWUniquePtr getBuildProcessInfo(const std::string& search,const std::string& what="agent",bool alive=true);

    int recoverDeviceFromError(const std::string& cu="");

  protected:
    int sendCmd(command_t &cmd, bool wait, uint64_t perform_at = 0, uint64_t wait_for = 0);
    int sendMDSCmd(command_t &cmd);
    chaos::common::data::CDWShrdPtr normalizeToJson(chaos::common::data::CDataWrapper *src, std::map<std::string, int> &list);

    chaos::common::data::CDWShrdPtr combineDataSets(std::map<int, chaos::common::data::CDataWrapper *>);
};
} // namespace misc
} // namespace driver
#endif /* ChaosController_H */
