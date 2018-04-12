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
}
};

namespace driver{
namespace misc{
typedef ChaosSharedPtr<chaos::common::data::CDataWrapper> ChaosDataSet;
class ChaosDatasetIO {
    // chaos::common::io::IODataDriverShrdPtr ioLiveDataDriver;
    ChaosSharedPtr<chaos::common::io::ManagedDirectIODataDriver> ioLiveDataDriver;
    chaos::common::network::NetworkBroker		*network_broker;
    chaos::common::message::MDSMessageChannel	*mds_message_channel;

    typedef struct {uint64_t qt;chaos::common::io::QueryCursor * qc;} qc_t;
    typedef std::map<uint64_t,qc_t> query_cursor_map_t;
    query_cursor_map_t query_cursor_map;

    chaos::common::data::CDataWrapper wrapper2dataset(chaos::common::data::CDataWrapper& in,int dir=chaos::DataPackCommonKey::DPCK_DATASET_TYPE_OUTPUT);
protected:

    uint64_t pcktid;
    uint64_t runid;
    std::string datasetName; // cu name
    std::string groupName; // US name
    uint64_t ageing;
    uint64_t timeo;
    int storageType;
    std::map<int,ChaosDataSet > datasets;
    void createMDSEntry();
    bool entry_created;
public:

    ChaosDatasetIO(const std::string& dataset_name,const std::string &group_name="DASETIO");
    ~ChaosDatasetIO();
    int setAgeing(uint64_t secs);
    int setStorage(int st);
    int setTimeo(uint64_t t);
    ChaosDataSet allocateDataset(int type=chaos::DataPackCommonKey::DPCK_DATASET_TYPE_OUTPUT);
    /**

        */
    int registerDataset ();
    int pushDataset( int type=chaos::DataPackCommonKey::DPCK_DATASET_TYPE_OUTPUT);
    /**
      Retrieve its own datasets from live
    */
    ChaosDataSet getDataset(int type=chaos::DataPackCommonKey::DPCK_DATASET_TYPE_OUTPUT);
    /**
      Retrieve whatever dataset
    */
    ChaosDataSet getDataset(const std::string &dsname,int type=chaos::DataPackCommonKey::DPCK_DATASET_TYPE_OUTPUT);
    /**
      Perform the query and return uid for pages
      \param dsname dataset name
      \param type type (output, input)
      \param start epoch in ms start of search
      \param end epoch in ms stop of search
      \param page number of item to return
      \return uid to be used to get page of datas, 0 if nothing found
    */
    uint64_t queryHistoryDatasets(const std::string &dsname,int type, uint64_t ms_start,uint64_t ms_end,int page=100);
    bool hasNextPage(uint64_t uid);
    std::vector<ChaosDataSet> getNextPage(uint64_t uid);

};
}}

#endif // CHAOSCULIGHT_H
