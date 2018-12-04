/* 
 * File:   ChaosController.h
 * Author: michelo
 *
 * Created on September 2, 2015, 5:29 PM
 */

#ifndef __ChaosController_H
#define __ChaosController_H

#include <map>
#include <string>
#include <chaos/common/chaos_constants.h>
#include <chaos/common/data/DatasetDB.h>

#include <chaos/cu_toolkit/data_manager/KeyDataStorage.h>

#include <common/misc/scheduler/SchedTimeElem.h>
#include <chaos/common/data/CDataWrapper.h>
#include <chaos/common/io/IODataDriver.h>

#include <chaos/common/batch_command/BatchCommandTypes.h>
#define CTRLAPP_ LAPP_ << "[ " << __FUNCTION__ << "]"
#define CTRLDBG_ LDBG_ << "[ " << __FUNCTION__ << "]"
#define CTRLERR_ LERR_ << "[ " << __PRETTY_FUNCTION__ << "]"
#define DEFAULT_TIMEOUT_FOR_CONTROLLER 10000000
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
#define MAX_QUERY_ELEMENTS 1000
#define QUERY_PAGE_MAX_TIME 1000 * 60 * 1 // 1 min
#define CHECK_HB 10 * 1000 * 1000         //10 s
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
class ChaosController : public ::common::misc::scheduler::SchedTimeElem
{

  private:
    chaos::common::message::MDSMessageChannel *mdsChannel;
    chaos::metadata_service_client::ChaosMetadataServiceClient *mds_client;
    chaos::common::io::IODataDriverShrdPtr live_driver;
    std::vector<std::string> mds_server_l;
    std::string path;
    chaos::common::data::DatasetDB datasetDB;

    chaos::CUStateKey::ControlUnitState state, last_state, next_state;
    uint64_t timeo, schedule;
    std::map<std::string, int> binaryToTranslate;
    uint64_t last_access, heart, reqtime, tot_us, naccess, refresh;
    int wostate;
    //  ::common::misc::data::DBbase* db;
    // NetworkBroker *broker;
    //chaos::common::message::MDSMessageChannel *mdsChannel;
    //! Device MEssage channel to control via chaos rpc the device
    //chaos::common::message::DeviceMessageChannel *deviceChannel;
    //! The io driver for accessing live data of the device

    std::string json_dataset;
    chaos::common::data::CDataWrapper data_out;
    std::map<int, std::string> cachedJsonChannels;

    uint32_t queryuid;
    ChaosSharedMutex iomutex;
    ChaosSharedMutex ioctrl;

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

    std::vector<std::string> cachedChannel_v;

    void parseClassZone(ChaosStringVector &v);
    std::string vector2Json(ChaosStringVector &v);
    std::string map2Json(std::map<uint64_t, std::string> &v);
    std::string dataset2Var(chaos::common::data::CDataWrapper *c, std::string &name);
    void cleanUpQuery();
    void initializeClient();
    void deinitializeClient();
    uint64_t last_ts[DPCK_LAST_DATASET_INDEX + 1], delta_update;
    uint64_t last_pckid[DPCK_LAST_DATASET_INDEX + 1];
    std::map<std::string, chaos::common::data::RangeValueInfo> attributeValueMap;
    std::vector<chaos::common::data::RangeValueInfo> getDeviceValuesInfo();
    void initializeAttributeIndexMap();

    void update();

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
        char dev_status[256];
        char error_status[256];
        char log_status[256];
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
    ChaosController(std::string path, uint32_t timeo = DEFAULT_TIMEOUT_FOR_CONTROLLER);

    virtual uint64_t sched(uint64_t ts);
    virtual ~ChaosController();

    int initDevice(const std::string &dev = "");
    int stopDevice(const std::string &dev = "");
    int startDevice(const std::string &dev = "");
    int deinitDevice(const std::string &dev = "");
    int loadDevice(const std::string &dev = "");
    int unloadDevice(const std::string &dev = "");
    int setAttributeToValue(const char *attributeName, const char *attributeValue, const std::string &cuname = "");

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

    void executeTimeIntervallQuery(DatasetDomain domain,
                                   uint64_t start_ts,
                                   uint64_t end_ts,
                                   chaos::common::io::QueryCursor **query_cursor,
                                   const std::string&name="",
                                   uint32_t page= DEFAULT_PAGE_LEN);

    void executeTimeIntervallQuery(DatasetDomain domain,
                                   uint64_t start_ts,
                                   uint64_t end_ts,
                                   const ChaosStringSet &meta_tags,
                                   chaos::common::io::QueryCursor **query_cursor,
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
                                  const std::string&name="",
                                  const uint32_t page_len= DEFAULT_PAGE_LEN);
    //! restore from a tag a dataset associated to a key
    int createNewSnapshot(const std::string &snapshot_tag,
                          const std::vector<std::string> &other_snapped_device);
    //!delete the snapshot
    int deleteSnapshot(const std::string &snapshot_tag);
    //!return the snapshot list for device controlled by this isntance
    int getSnapshotList(ChaosStringVector &snapshot_list);

    //!return the dataset of the specified cu associated to a given snapshot
    // 
    chaos::common::data::CDataWrapper getSnapshotDataset(const std::string&snapname,const std::string& cuname);

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
    boost::shared_ptr<chaos::common::data::CDataWrapper> fetch(int channel);
    const std::string fetchJson(int channel);
    /*
     * perform a history query from start to end, return a vector of result
     * @param[in] start epoch timestamp in ms or string offset start search
     *  @param[in] end epoch timestamp in ms or string offset end search
     *  @param[out] res result
     *  @param[in]  page page len =o if full search
     *  @return 0 if success and end search, >0 is an uid to be use with next to get remaining results, <0 an error occurred
     * */
    int32_t queryHistory(const std::string &start, const std::string &end, int channel, std::vector<boost::shared_ptr<chaos::common::data::CDataWrapper>> &res, int page = 0);
    int32_t queryHistory(const std::string &start, const std::string &end, const std::vector<std::string> &tags, int channel, std::vector<boost::shared_ptr<chaos::common::data::CDataWrapper>> &res, int page = 0);

    int32_t queryNext(int32_t id, std::vector<boost::shared_ptr<chaos::common::data::CDataWrapper>> &res);
    bool queryHasNext(int32_t id);
    int getSnapshotsofCU(const std::string &cuname, std::map<uint64_t, std::string> &res);
    /*void dumpHistoryToTgz(const std::string& fname,const std::string& start,const std::string& end,int channel,std::string tagname);*/
    uint64_t lastAccess() { return reqtime; }
    /**
      * Return one live channel corresponding to a CUname and a domain
     */
    ChaosSharedPtr<chaos::common::data::CDataWrapper> getLiveChannel(const std::string &CUNAME = "", int domain = chaos::DataPackCommonKey::DPCK_DATASET_TYPE_HEALTH);
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

    int recoverDeviceFromError(const std::string& cu="");

  protected:
    int sendCmd(command_t &cmd, bool wait, uint64_t perform_at = 0, uint64_t wait_for = 0);
    int sendMDSCmd(command_t &cmd);
    boost::shared_ptr<chaos::common::data::CDataWrapper> normalizeToJson(chaos::common::data::CDataWrapper *src, std::map<std::string, int> &list);

    boost::shared_ptr<chaos::common::data::CDataWrapper> combineDataSets(std::map<int, chaos::common::data::CDataWrapper *>);
};
} // namespace misc
} // namespace driver
#endif /* ChaosController_H */
