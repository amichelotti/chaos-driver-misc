#include "ChaosDatasetIO.h"
#include <chaos/common/healt_system/HealtManager.h>
#include <chaos/common/direct_io/DirectIOClient.h>
#include <chaos/common/direct_io/DirectIOClientConnection.h>

#define DPD_LOG_HEAD "[ChaosDatasetIO] - "

#define DPD_LAPP LAPP_ << DPD_LOG_HEAD
#define DPD_LDBG LDBG_ << DPD_LOG_HEAD << __PRETTY_FUNCTION__
#define DPD_LERR LERR_ << DPD_LOG_HEAD << __PRETTY_FUNCTION__ << "(" << __LINE__ << ") "


using namespace chaos::common::data;
using namespace chaos::common::utility;
using namespace chaos::common::network;
using namespace chaos::common::direct_io;
using namespace chaos::common::direct_io::channel;
using namespace chaos::common::healt_system;

namespace driver{
namespace misc{
ChaosDatasetIO::ChaosDatasetIO(const std::string &name):connection_feeder("ChaosDatasetIO", this),datasetName(name){
    network_broker=NetworkBroker::getInstance();
    mds_message_channel = network_broker->getMetadataserverMessageChannel();
    if(!mds_message_channel)
        throw chaos::CException(-1, "No mds channel found", __PRETTY_FUNCTION__);

    //! get the direct io client
    direct_io_client = network_broker->getSharedDirectIOClientInstance();
    //checkif someone has passed us the device indetification
    DPD_LAPP << "Scan the direction address";

    chaos::common::data::CDataWrapper *tmp_data_handler = NULL;

    if(!mds_message_channel->getDataDriverBestConfiguration(&tmp_data_handler, 5000)){
        if(tmp_data_handler!=NULL){
            ChaosUniquePtr<chaos::common::data::CDataWrapper> best_available_da_ptr(tmp_data_handler);
            DPD_LDBG <<best_available_da_ptr->getJSONString();
            ChaosUniquePtr<chaos::common::data::CMultiTypeDataArrayWrapper> liveMemAddrConfig(best_available_da_ptr->getVectorValue(chaos::DataServiceNodeDefinitionKey::DS_DIRECT_IO_FULL_ADDRESS_LIST));
            if(liveMemAddrConfig.get()){
                size_t numerbOfserverAddressConfigured = liveMemAddrConfig->size();
                for ( int idx = 0; idx < numerbOfserverAddressConfigured; idx++ ){
                    std::string serverDesc = liveMemAddrConfig->getStringElementAtIndex(idx);
                    connection_feeder.addURL(serverDesc);
                }
            }
        }
    }
}
ChaosDatasetIO::~ChaosDatasetIO(){
    connection_feeder.clear();

    //if(direct_io_client) {
    //CHAOS_NOT_THROW(InizializableService::deinitImplementation(direct_io_client,
    //														   direct_io_client->getName(),
    //														   __PRETTY_FUNCTION__);)
    //delete(direct_io_client);
    //}

    if(mds_message_channel) network_broker->disposeMessageChannel(mds_message_channel);
}


int ChaosDatasetIO::deinit(){

}


void ChaosDatasetIO::handleEvent(DirectIOClientConnection *client_connection,DirectIOClientConnectionStateType::DirectIOClientConnectionStateType event) {
    //if the channel has bee disconnected turn the relative index offline, if onli reput it online
    boost::unique_lock<boost::shared_mutex>(mutext_feeder);
    uint32_t service_index = boost::lexical_cast<uint32_t>(client_connection->getCustomStringIdentification());
    DEBUG_CODE(DPD_LDBG << "Manage event for service with index " << service_index << " and url " << client_connection->getURL();)
    switch(event) {
        case chaos_direct_io::DirectIOClientConnectionStateType::DirectIOClientConnectionEventConnected:
            connection_feeder.setURLOnline(service_index);
            break;

        case chaos_direct_io::DirectIOClientConnectionStateType::DirectIOClientConnectionEventDisconnected:
            connection_feeder.setURLOffline(service_index);
            break;
    }
}
void ChaosDatasetIO::disposeService(void *service_ptr) {
    if(!service_ptr) return;
    DirectIOChannelsInfo	*info = static_cast<DirectIOChannelsInfo*>(service_ptr);

    if(info->device_client_channel) info->connection->releaseChannelInstance(info->device_client_channel);
    direct_io_client->releaseConnection(info->connection);
    delete(info);
}

/*---------------------------------------------------------------------------------

 ---------------------------------------------------------------------------------*/
void* ChaosDatasetIO::serviceForURL(const chaos::common::network::URL& url, uint32_t service_index) {
    DPD_LDBG << "Add connection for " << url.getURL();
    DirectIOChannelsInfo * clients_channel = NULL;
    chaos_direct_io::DirectIOClientConnection *tmp_connection = direct_io_client->getNewConnection(url.getURL());
    if(tmp_connection) {
        clients_channel = new DirectIOChannelsInfo();
        clients_channel->connection = tmp_connection;

        //allocate the client channel
        clients_channel->device_client_channel = (DirectIODeviceClientChannel*)tmp_connection->getNewChannelInstance("DirectIODeviceClientChannel");
        if(!clients_channel->device_client_channel) {
            DPD_LDBG << "Error creating client device channel for " << url.getURL();

            //release conenction
            direct_io_client->releaseConnection(tmp_connection);

            //relase struct
            delete(clients_channel);
            return NULL;
        }

        //set this driver instance as event handler for connection
        clients_channel->connection->setEventHandler(this);
        //!put the index in the conenction so we can found it wen we receive event from it
        clients_channel->connection->setCustomStringIdentification(boost::lexical_cast<std::string>(service_index));
    } else {
        DPD_LERR << "Error creating client connection for " << url.getURL();
    }
    return clients_channel;
}


// push a dataset
int ChaosDatasetIO::pushDataset( CDataWrapper* new_dataset,int type,int store_hint) {
    int err = 0;
    //ad producer key
    new_dataset->addStringValue(chaos::DataPackCommonKey::DPCK_DEVICE_ID, datasetName);

    new_dataset->addInt32Value(chaos::DataPackCommonKey::DPCK_DATASET_TYPE, type);
    ChaosUniquePtr<SerializationBuffer> serialization(new_dataset->getBSONData());
//	DPD_LDBG <<" PUSHING:"<<new_dataset->getJSONString();
    DirectIOChannelsInfo	*next_client = static_cast<DirectIOChannelsInfo*>(connection_feeder.getService());
    serialization->disposeOnDelete = !next_client;
    if(next_client) {
        boost::shared_lock<boost::shared_mutex>(next_client->connection_mutex);

        //free the packet
        serialization->disposeOnDelete = false;
        if((err =(int)next_client->device_client_channel->storeAndCacheDataOutputChannel(datasetName+chaos::datasetTypeToPostfix(type),
                                                                                         (void*)serialization->getBufferPtr(),
                                                                                         (uint32_t)serialization->getBufferLen(),
                                                                                         (chaos::DataServiceNodeDefinitionType::DSStorageType)store_hint))) {
            DPD_LERR << "Error storing dataset with code:" << err;
        }
    } else {
        DEBUG_CODE(DPD_LDBG << "No available socket->loose packet");
        err = -1;
    }

    return err;
}
/*
// get a dataset
int ChaosDatasetIO::getLastDataset(const std::string& producer_key,
                                             chaos::common::data::CDataWrapper **last_dataset) {
    int err = 0;
    uint32_t size = 0;
    char* result = NULL;
    DirectIOChannelsInfo	*next_client = static_cast<DirectIOChannelsInfo*>(connection_feeder.getService());
    if(!next_client) return err;

    boost::shared_lock<boost::shared_mutex>(next_client->connection_mutex);

    next_client->device_client_channel->requestLastOutputData(producer_key, (void**)&result, size);
    *last_dataset = new CDataWrapper(result);
    return err;
}
*/
//! register the dataset of ap roducer
int ChaosDatasetIO::registerDataset(chaos::common::data::CDataWrapper last_dataset,int type) {
    CHAOS_ASSERT(mds_message_channel);
    int ret;
    CDataWrapper mdsPack;
    mdsPack.addCSDataValue(chaos::ControlUnitNodeDefinitionKey::CONTROL_UNIT_DATASET_DESCRIPTION, last_dataset);

    mdsPack.addStringValue(chaos::NodeDefinitionKey::NODE_UNIQUE_ID, datasetName);
    mdsPack.addStringValue(chaos::NodeDefinitionKey::NODE_RPC_DOMAIN, chaos::common::utility::UUIDUtil::generateUUIDLite());
    mdsPack.addStringValue(chaos::NodeDefinitionKey::NODE_RPC_ADDR, network_broker->getRPCUrl());
    mdsPack.addStringValue("mds_control_key","none");
    mdsPack.addStringValue(chaos::NodeDefinitionKey::NODE_TYPE, chaos::NodeType::NODE_TYPE_CONTROL_UNIT);
    if((ret=mds_message_channel->sendNodeRegistration(mdsPack, true, 10000)) ==0){
        CDataWrapper mdsPack;
        mdsPack.addStringValue(chaos::NodeDefinitionKey::NODE_UNIQUE_ID, datasetName);
        mdsPack.addStringValue(chaos::NodeDefinitionKey::NODE_TYPE, chaos::NodeType::NODE_TYPE_CONTROL_UNIT);
        ret = mds_message_channel->sendNodeLoadCompletion(mdsPack, true, 10000);
        HealtManager::getInstance()->addNewNode(datasetName);

        HealtManager::getInstance()->addNodeMetricValue(datasetName,
							chaos::NodeHealtDefinitionKey::NODE_HEALT_STATUS,
							chaos::NodeHealtDefinitionValue::NODE_HEALT_STATUS_START,
							true);
    }

    return ret;
}


}
}
