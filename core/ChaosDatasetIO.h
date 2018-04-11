#ifndef CHAOSDATASETIO_H
#define CHAOSDATASETIO_H


#include <chaos/common/data/CDataWrapper.h>
#include <chaos/common/io/ManagedDirectIODataDriver.h>
#include <chaos/common/direct_io/DirectIOClient.h>
#include <chaos/common/message/MDSMessageChannel.h>
#include <chaos/common/utility/InizializableService.h>
#include <chaos/common/direct_io/DirectIOClientConnection.h>
#include <chaos/common/network/NetworkBroker.h>
namespace driver{
namespace misc{

class ChaosDatasetIO {
   // chaos::common::io::IODataDriverShrdPtr ioLiveDataDriver;
    ChaosSharedPtr<chaos::common::io::ManagedDirectIODataDriver> ioLiveDataDriver;
    chaos::common::network::NetworkBroker		*network_broker;
    chaos::common::message::MDSMessageChannel	*mds_message_channel;

    chaos::common::data::CDataWrapper wrapper2dataset(chaos::common::data::CDataWrapper& in,int dir=chaos::DataPackCommonKey::DPCK_DATASET_TYPE_OUTPUT);
protected:
    //!inherited by @common::network::URLServiceFeederHandler
  //  void  disposeService(void *service_ptr);

    //! inherited by @common::network::URLServiceFeederHandler
   // void* serviceForURL(const URL& url, uint32_t service_index);
    uint64_t pcktid;
    uint64_t runid;

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
        void deinit() throw (chaos::CException);

};
}}

#endif // CHAOSCULIGHT_H
