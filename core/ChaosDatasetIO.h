#ifndef CHAOSDATASETIO_H
#define CHAOSDATASETIO_H


#include <chaos/common/data/CDataWrapper.h>
#include <chaos/common/network/URLServiceFeeder.h>
#include <chaos/common/direct_io/DirectIOClient.h>
#include <chaos/common/message/MDSMessageChannel.h>
#include <chaos/common/utility/InizializableService.h>
#include <chaos/common/direct_io/DirectIOClientConnection.h>
#include <chaos/common/network/NetworkBroker.h>
namespace driver{
namespace misc{

class ChaosDatasetIO:protected chaos::common::network::URLServiceFeederHandler,
chaos::common::direct_io::DirectIOClientConnectionEventHandler{
    typedef struct DirectIOChannelsInfo {
        boost::shared_mutex													connection_mutex;
        chaos::common::direct_io::DirectIOClientConnection					*connection;
        chaos::common::direct_io::channel::DirectIODeviceClientChannel		*device_client_channel;
    } DirectIOChannelsInfo;

    chaos::common::network::NetworkBroker		*network_broker;
    chaos::common::direct_io::DirectIOClient	*direct_io_client;
    chaos::common::message::MDSMessageChannel	*mds_message_channel;

    chaos::common::network::URLServiceFeeder	connection_feeder;

protected:
    //!inherited by @common::network::URLServiceFeederHandler
    void  disposeService(void *service_ptr);

    //! inherited by @common::network::URLServiceFeederHandler
    void* serviceForURL(const URL& url, uint32_t service_index);

    //! inherited by @chaos::common::direct_io::DirectIOClientConnectionEventHandler
    void handleEvent(chaos_direct_io::DirectIOClientConnection *client_connection,chaos_direct_io::DirectIOClientConnectionStateType::DirectIOClientConnectionStateType event);
    public:
    std::string datasetName; // cu name
        ChaosDatasetIO(const std::string& dataset_name);
        ~ChaosDatasetIO();
        /**

        */
        int registerDataset ( chaos::common::data::CDataWrapper dataset,int type=chaos::DataPackCommonKey::DPCK_DATASET_TYPE_OUTPUT);
        int pushDataset( chaos::common::data::CDataWrapper* dataset,int type=chaos::DataPackCommonKey::DPCK_DATASET_TYPE_OUTPUT,int store_hint=2);
        int deinit();

};
}}

#endif // CHAOSCULIGHT_H
