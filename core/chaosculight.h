#ifndef CHAOSCULIGHT_H
#define CHAOSCULIGHT_H


#include <chaos/common/data/CDataWrapper.h>
#include <chaos/common/network/URLServiceFeeder.h>
#include <chaos/common/direct_io/DirectIOClient.h>
#include <chaos/common/message/MDSMessageChannel.h>
#include <chaos/common/utility/InizializableService.h>
#include <chaos/common/direct_io/DirectIOClientConnection.h>
#include <chaos/common/network/NetworkBroker.h>
namespace driver{
namespace misc{

class ChaosCULight{


    chaos::common::network::NetworkBroker		*network_broker;
    chaos::common::direct_io::DirectIOClient	*direct_io_client;
    chaos::common::message::MDSMessageChannel	*mds_message_channel;

    chaos::common::network::URLServiceFeeder	connection_feeder;

    public:
        ChaosCULight();
        ~ChaosCULight();
        /**

        */
        int registerCU (const std::string&name,const chaos::common::data::CDataWrapper& dataset);
        int pushDataset(const chaos::common::data::CDataWrapper& dataset);
        int deinit();

};
}}

#endif // CHAOSCULIGHT_H
