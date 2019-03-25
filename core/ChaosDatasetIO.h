#ifndef CHAOSDATASETIO_H
#define CHAOSDATASETIO_H

#include <chaos/common/io/ManagedDirectIODataDriver.h>

namespace chaos{
    namespace common{
        namespace message{
            class MDSMessageChannel;
        }
        namespace io{
            class QueryCursor;
        };
        namespace network{
            class NetworkBroker;
        }
        namespace metadata_logging{
            class StandardLoggingChannel;
            class AlarmLoggingChannel;
        }
    }
};
#define GET_TIMESTAMP(x) (x->getInt64Value("dpck_ats"))
#define GET_HWTIMESTAMP(x) (x->getInt64Value("dpck_hr_ats"))
#define GET_PCKID(x) (x->getInt64Value("dpck_seq_id"))
#define GET_RUNID(x) (x->getInt64Value("cudk_run_id"))

namespace driver{
    namespace misc{
        typedef ChaosSharedPtr<chaos::common::data::CDataWrapper> ChaosDataSet;
        class ChaosDatasetIO             :protected chaos::common::async_central::TimerHandler{
             chaos::common::io::IODataDriverShrdPtr ioLiveDataDriver;

            ChaosSharedPtr<chaos::common::io::ManagedDirectIODataDriver> ioLiveShDataDriver;
            chaos::common::network::NetworkBroker        *network_broker;
            chaos::common::message::MDSMessageChannel    *mds_message_channel;
             //!logging channel
            chaos::common::metadata_logging::StandardLoggingChannel *standard_logging_channel;
                
                //!control unit alarm group
            chaos::common::metadata_logging::AlarmLoggingChannel    *alarm_logging_channel;
            typedef struct {uint64_t qt;
                uint32_t page_len;
                chaos::common::io::QueryCursor * qc;
            } qc_t;
            typedef std::map<uint64_t,qc_t> query_cursor_map_t;
            query_cursor_map_t query_cursor_map;
            uint64_t query_index;
            chaos::common::data::CDWUniquePtr wrapper2dataset(chaos::common::data::CDataWrapper& in,int dir=chaos::DataPackCommonKey::DPCK_DATASET_TYPE_OUTPUT);
        protected:
            
            uint64_t runid;
            std::string datasetName; // cu name
            std::string groupName; // US name
            uint64_t ageing;
            uint64_t timeo;
            uint64_t last_seq;
            int storageType;
            std::map<int,ChaosDataSet > datasets;
            uint64_t pkids[16];
            void createMDSEntry();
            bool entry_created;
            uint32_t defaultPage;
            std::string uid;
            double last_push_rate_grap_ts;
            void updateHealth();
            void timeout();
            bool deinitialized;
            std::string implementation;
            uint64_t packet_size;
            uint8_t cu_alarm_lvl;
            uint8_t dev_alarm_lvl;
            int32_t findMax(ChaosDataSet&ds, std::vector<std::string>&);
            std::vector<std::string> cu_alarms,dev_alarms;
        public:
            
            ChaosDatasetIO(const std::string& dataset_name,const std::string &group_name="DATASETIO");
            ~ChaosDatasetIO();
            int setAgeing(uint64_t secs);
            int setStorage(int st);
            int setTimeo(uint64_t t);
            ChaosDataSet allocateDataset(int type=chaos::DataPackCommonKey::DPCK_DATASET_TYPE_OUTPUT);
            /**
             
             */
            int registerDataset ();
            int pushDataset(ChaosDataSet&ds, int type=chaos::DataPackCommonKey::DPCK_DATASET_TYPE_OUTPUT);

            int pushDataset( int type=chaos::DataPackCommonKey::DPCK_DATASET_TYPE_OUTPUT);
            /**
             Retrieve its own datasets from live
             */
            ChaosDataSet getLiveDataset(int type=chaos::DataPackCommonKey::DPCK_DATASET_TYPE_OUTPUT);

            /**
             Retrieve  own datasets from live
             */
           ChaosDataSet getDataset(int type=chaos::DataPackCommonKey::DPCK_DATASET_TYPE_OUTPUT);
            /**
             Retrieve whatever dataset from live
             */
            ChaosDataSet getLiveDataset(const std::string &dsname,int type=chaos::DataPackCommonKey::DPCK_DATASET_TYPE_OUTPUT);
            /**
             Perform the query and return uid for pages
             \param dsname dataset name
             \param type type (output, input)
             \param start epoch in ms start of search
             \param end epoch in ms stop of search
             \param page number of item to return
             \return uid to be used to get page of datas, 0 if nothing found
             */
            uint64_t queryHistoryDatasets(const std::string &dsname, uint64_t ms_start,uint64_t ms_end,uint32_t page,int type=chaos::DataPackCommonKey::DPCK_DATASET_TYPE_OUTPUT);
            
            /**
             Same query over owned datasets
             */
            uint64_t queryHistoryDatasets(uint64_t ms_start,uint64_t ms_end,uint32_t page,int type=chaos::DataPackCommonKey::DPCK_DATASET_TYPE_OUTPUT);
            /**
             Not paged query, returns whole query
             */
            std::vector<ChaosDataSet> queryHistoryDatasets(const std::string &dsname, uint64_t ms_start,uint64_t ms_end,int type=chaos::DataPackCommonKey::DPCK_DATASET_TYPE_OUTPUT);
            std::vector<ChaosDataSet> queryHistoryDatasets(uint64_t ms_start,uint64_t ms_end,int type=chaos::DataPackCommonKey::DPCK_DATASET_TYPE_OUTPUT);
            
            /**
             Perform a full query on the current device specifing a starting runid and a sequid and a tag.
             */
            uint64_t queryHistoryDatasets(uint64_t ms_start,uint64_t ms_end,const uint64_t runid,const uint64_t sequid,const ChaosStringSet& meta_tags,uint32_t page,int type=chaos::DataPackCommonKey::DPCK_DATASET_TYPE_OUTPUT);
            /**
             Perform a full query on the specified device specifing a starting runid and a sequid and a tag.
             */
            uint64_t queryHistoryDatasets(const std::string &dsname, uint64_t ms_start,uint64_t ms_end,const uint64_t runid,const uint64_t sequid,const ChaosStringSet& meta_tags,uint32_t page,int type=chaos::DataPackCommonKey::DPCK_DATASET_TYPE_OUTPUT);
            /**
             * Allocate a CU alarm
             * return 0 if success
            */
            int allocateCUAlarm(const std::string& name);
            /**
             * Allocate a DEV alarm
             * return 0 if success
            */
            int allocateDEVAlarm(const std::string& name);

            /**
             * Set a preallocated CUalarm to a given level (0= no alarm,1=warning,2>=failure)
             * return 0 if No CU alarm set, 1 if some warning,2 if some failure
            */
            int setCUAlarmLevel(const std::string& name,uint8_t value,const std::string msg="");
            void log(const std::string& subject, int log_level,const std::string&message);
            /**
             * Set a preallocated DEValarm to a given level (0= no alarm,1=warning,2>=failure)
             * return 0 if No DEV alarm set, 1 if some warning,2 if some failure
            */
            int setDeviceAlarmLevel(const std::string& name,uint8_t value,const std::string msg="");
            bool queryHasNext(uint64_t uid);
            std::string getUid(){return uid;}
            uint64_t getRunID(){return runid;}
            void setImplementation(const std::string &impl);
            void setRunID(uint64_t ru){runid=ru;}
            std::vector<ChaosDataSet> getNextPage(uint64_t uid);
            void deinit();
            
        };
    }}

#endif // CHAOSCULIGHT_H
