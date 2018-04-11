#include "ChaosDatasetIO.h"
#include <chaos/common/healt_system/HealtManager.h>
#include <chaos/common/direct_io/DirectIOClient.h>
#include <chaos/common/direct_io/DirectIOClientConnection.h>
#include <chaos/common/io/SharedManagedDirecIoDataDriver.h>

#define DPD_LOG_HEAD "[ChaosDatasetIO] - "

#define DPD_LAPP LAPP_ << DPD_LOG_HEAD
#define DPD_LDBG LDBG_ << DPD_LOG_HEAD << __PRETTY_FUNCTION__
#define DPD_LERR LERR_ << DPD_LOG_HEAD << __PRETTY_FUNCTION__ << "(" << __LINE__ << ") "

#include <chaos_metadata_service_client/ChaosMetadataServiceClient.h>

using namespace chaos::common::data;
using namespace chaos::common::utility;
using namespace chaos::common::network;
using namespace chaos::common::direct_io;
using namespace chaos::common::direct_io::channel;
using namespace chaos::common::healt_system;

namespace driver{
namespace misc{
ChaosDatasetIO::ChaosDatasetIO(const std::string &name):datasetName(name),pcktid(0){
    InizializableService::initImplementation(chaos::common::io::SharedManagedDirecIoDataDriver::getInstance(), NULL, "SharedManagedDirecIoDataDriver", __PRETTY_FUNCTION__);

    //ioLiveDataDriver =  chaos::metadata_service_client::ChaosMetadataServiceClient::getInstance()->getDataProxyChannelNewInstance();
    ioLiveDataDriver =chaos::common::io::SharedManagedDirecIoDataDriver::getInstance()->getSharedDriver();
    network_broker=NetworkBroker::getInstance();
    mds_message_channel = network_broker->getMetadataserverMessageChannel();
    if(!mds_message_channel)
        throw chaos::CException(-1, "No mds channel found", __PRETTY_FUNCTION__);

    //checkif someone has passed us the device indetification
 /*   DPD_LAPP << "Scan the direction address";

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
                    //connection_feeder.addURL(serverDesc);
                }
            }
        }
    }
*/
    StartableService::initImplementation(HealtManager::getInstance(), NULL, "HealtManager", __PRETTY_FUNCTION__);
    runid=time(NULL);

}
ChaosDatasetIO::~ChaosDatasetIO(){
    DEBUG_CODE(DPD_LDBG << "Destroy all resources");

    //CHAOS_NOT_THROW(StartableService::deinitImplementation(HealtManager::getInstance(), "HealtManager", __PRETTY_FUNCTION__););
//sleep(1);
//InizializableService::deinitImplementation(chaos::common::io::SharedManagedDirecIoDataDriver::getInstance(), "SharedManagedDirecIoDataDriver", __PRETTY_FUNCTION__);
//sleep(1);
    DEBUG_CODE(DPD_LDBG << "End");


   // connection_feeder.clear();
   /* if(direct_io_client) {
        CHAOS_NOT_THROW(InizializableService::deinitImplementation(direct_io_client,
                                                                   direct_io_client->getName(),
                                                                   __PRETTY_FUNCTION__);)
        delete(direct_io_client);
    }

   if(mds_message_channel) network_broker->disposeMessageChannel(mds_message_channel);
   */

}


void ChaosDatasetIO::deinit() throw (chaos::CException){


}

/*
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
    DEBUG_CODE(DPD_LDBG << "Dispose service:"<<std::hex<<(void*)service_ptr);

    DirectIOChannelsInfo	*info = static_cast<DirectIOChannelsInfo*>(service_ptr);

    if(info->device_client_channel) {
        info->connection->releaseChannelInstance(info->device_client_channel);
        info->device_client_channel=NULL;
    }
    if(info->connection){
        direct_io_client->releaseConnection(info->connection);
        info->connection=NULL;
    }
    delete(info);

}


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

*/

// push a dataset
int ChaosDatasetIO::pushDataset( CDataWrapper* new_dataset,int type,int store_hint) {
    int err = 0;
    //ad producer key
    if(!new_dataset->hasKey((chaos::DataPackCommonKey::DPCK_TIMESTAMP))){
    // add timestamp of the datapack
        uint64_t ts = chaos::common::utility::TimingUtil::getTimeStamp();
        new_dataset->addInt64Value(chaos::DataPackCommonKey::DPCK_TIMESTAMP, ts);
    }

    if(!new_dataset->hasKey(chaos::DataPackCommonKey::DPCK_SEQ_ID)){
        new_dataset->addInt64Value(chaos::DataPackCommonKey::DPCK_SEQ_ID,pcktid++ );
    }
    if(!new_dataset->hasKey(chaos::ControlUnitNodeDefinitionKey::CONTROL_UNIT_RUN_ID)){
        new_dataset->addInt64Value(chaos::ControlUnitNodeDefinitionKey::CONTROL_UNIT_RUN_ID,(int64_t)runid );
    }
    if(!new_dataset->hasKey(chaos::DataPackCommonKey::DPCK_DEVICE_ID)){
        new_dataset->addStringValue(chaos::DataPackCommonKey::DPCK_DEVICE_ID, datasetName);
    }
    if(!new_dataset->hasKey(chaos::DataPackCommonKey::DPCK_DATASET_TYPE)){
        new_dataset->addInt32Value(chaos::DataPackCommonKey::DPCK_DATASET_TYPE, type);
    }
    //ChaosUniquePtr<SerializationBuffer> serialization(new_dataset->getBSONData());
//	DPD_LDBG <<" PUSHING:"<<new_dataset->getJSONString();
   // DirectIOChannelsInfo	*next_client = static_cast<DirectIOChannelsInfo*>(connection_feeder.getService());
   // serialization->disposeOnDelete = !next_client;
    ioLiveDataDriver->storeData(datasetName+chaos::datasetTypeToPostfix(type),new_dataset,(chaos::DataServiceNodeDefinitionType::DSStorageType)store_hint,false);


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

chaos::common::data::CDataWrapper ChaosDatasetIO::wrapper2dataset(chaos::common::data::CDataWrapper& dataset_pack,int dir){
    CDataWrapper cu_dataset,mds_registration_pack;
    ChaosStringVector all_template_key;
    dataset_pack.getAllKey(all_template_key);

    for(ChaosStringVectorIterator it = all_template_key.begin();
           it != all_template_key.end();
           it++) {
           if(dataset_pack.isNullValue(*it)) {
               DPD_LERR << "Removed from template null value key:" << *it;
               continue;
           }
           CDataWrapper ds;
           int32_t dstype=0,subtype=0;
           int32_t size=0;
           ds.addStringValue(chaos::ControlUnitNodeDefinitionKey::CONTROL_UNIT_DATASET_ATTRIBUTE_NAME,*it);

           size =dataset_pack.getValueSize(*it);
           if(dataset_pack.isDoubleValue(*it)){
               dstype |= chaos::DataType::TYPE_DOUBLE;
               subtype= chaos::DataType::SUB_TYPE_DOUBLE;
           } else if(dataset_pack.isInt64Value(*it)){
               dstype |=  chaos::DataType::TYPE_INT64;
               subtype= chaos::DataType::SUB_TYPE_INT64;

           } else if(dataset_pack.isInt32Value(*it)){
               dstype |=  chaos::DataType::TYPE_INT32;
               subtype= chaos::DataType::SUB_TYPE_INT32;

           } else if(dataset_pack.isStringValue(*it)){
               dstype |=  chaos::DataType::TYPE_STRING;
               subtype= chaos::DataType::SUB_TYPE_STRING;

           } else if(dataset_pack.isBinaryValue(*it)){
               dstype |=  chaos::DataType::TYPE_BYTEARRAY;
           } else {
               dstype |= chaos::DataType::TYPE_DOUBLE;
               subtype= chaos::DataType::SUB_TYPE_DOUBLE;

           }
           if(dataset_pack.isVector(*it)){
               //dstype = chaos::DataType::TYPE_ACCESS_ARRAY;
               dstype = chaos::DataType::TYPE_BYTEARRAY;
           }
           ds.addInt32Value(chaos::ControlUnitNodeDefinitionKey::CONTROL_UNIT_DATASET_BINARY_SUBTYPE,subtype);
           ds.addInt32Value(chaos::ControlUnitNodeDefinitionKey::CONTROL_UNIT_DATASET_ATTRIBUTE_TYPE,dstype);
           ds.addInt32Value(chaos::ControlUnitNodeDefinitionKey::CONTROL_UNIT_DATASET_ATTRIBUTE_DIRECTION,dir);
           ds.addInt32Value(chaos::ControlUnitNodeDefinitionKey::CONTROL_UNIT_DATASET_VALUE_MAX_SIZE,size);

           DPD_LDBG<<"- ATTRIBUTE \""<<*it<<"\" SIZE:"<<size<<" TYPE:"<<dstype<<" SUBTYPE:"<<subtype;
           cu_dataset.appendCDataWrapperToArray(ds);

       }
    cu_dataset.addInt64Value(chaos::ControlUnitNodeDefinitionKey::CONTROL_UNIT_DATASET_TIMESTAMP,chaos::common::utility::TimingUtil::getTimeStamp());

    //close array for all device description
    cu_dataset.finalizeArrayForKey(chaos::ControlUnitNodeDefinitionKey::CONTROL_UNIT_DATASET_DESCRIPTION);
    cu_dataset.addInt64Value(chaos::DataPackCommonKey::DPCK_SEQ_ID,(int64_t)0);

    mds_registration_pack.addCSDataValue(chaos::ControlUnitNodeDefinitionKey::CONTROL_UNIT_DATASET_DESCRIPTION, cu_dataset);
    mds_registration_pack.addStringValue(chaos::NodeDefinitionKey::NODE_TYPE, chaos::NodeType::NODE_TYPE_CONTROL_UNIT);
    return mds_registration_pack;
}

//! register the dataset of ap roducer
int ChaosDatasetIO::registerDataset(chaos::common::data::CDataWrapper dataset_pack,int type) {
    CHAOS_ASSERT(mds_message_channel);
    CDataWrapper mds_registration_pack=wrapper2dataset(dataset_pack,type);

    int ret;

    DEBUG_CODE(DPD_LDBG << mds_registration_pack.getJSONString());



    mds_registration_pack.addStringValue(chaos::NodeDefinitionKey::NODE_UNIQUE_ID, datasetName);
    mds_registration_pack.addStringValue(chaos::NodeDefinitionKey::NODE_RPC_DOMAIN, chaos::common::utility::UUIDUtil::generateUUIDLite());
    mds_registration_pack.addStringValue(chaos::NodeDefinitionKey::NODE_RPC_ADDR, network_broker->getRPCUrl());
    mds_registration_pack.addStringValue("mds_control_key","none");
    mds_registration_pack.addStringValue(chaos::NodeDefinitionKey::NODE_TYPE, chaos::NodeType::NODE_TYPE_CONTROL_UNIT);
    if((ret=mds_message_channel->sendNodeRegistration(mds_registration_pack, true, 10000)) ==0){
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
