#include "ChaosDatasetIO.h"
#include <chaos/common/healt_system/HealtManager.h>
#include <chaos/common/direct_io/DirectIOClient.h>
#include <chaos/common/direct_io/DirectIOClientConnection.h>
#include <chaos/common/io/SharedManagedDirecIoDataDriver.h>
#include <chaos_metadata_service_client/api_proxy/unit_server/NewUS.h>
#include <chaos_metadata_service_client/api_proxy/unit_server/ManageCUType.h>
#include <chaos_metadata_service_client/api_proxy/control_unit/SetInstanceDescription.h>
#include <chaos/common/network/NetworkBroker.h>

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
using namespace chaos::metadata_service_client;

#define EXECUTE_CHAOS_API(api_name,time_out,...) \
    DPD_LDBG<<" " <<" Executing Api:\""<< # api_name<<"\"" ;\
    chaos::metadata_service_client::api_proxy::ApiProxyResult apires=  GET_CHAOS_API_PTR(api_name)->execute( __VA_ARGS__ );\
    apires->setTimeout(time_out);\
    apires->wait();\
    if(apires->getError()){\
    std::stringstream ss;\
    ss<<" error in :"<<__FUNCTION__<<"|"<<__LINE__<<"|"<< # api_name <<" " <<apires->getErrorMessage();\
    DPD_LERR<<ss.str();\
}
namespace driver{
namespace misc{
ChaosDatasetIO::ChaosDatasetIO(const std::string &name,const std::string &group_name):datasetName(name),groupName(group_name),pcktid(0), ageing(3600),storageType((int)chaos::DataServiceNodeDefinitionType::DSStorageType::DSStorageTypeLiveHistory),timeo(5000),entry_created(false)
 {
    InizializableService::initImplementation(chaos::common::io::SharedManagedDirecIoDataDriver::getInstance(), NULL, "SharedManagedDirecIoDataDriver", __PRETTY_FUNCTION__);

    //ioLiveDataDriver =  chaos::metadata_service_client::ChaosMetadataServiceClient::getInstance()->getDataProxyChannelNewInstance();
    ioLiveDataDriver =chaos::common::io::SharedManagedDirecIoDataDriver::getInstance()->getSharedDriver();
    network_broker=NetworkBroker::getInstance();
    mds_message_channel = network_broker->getMetadataserverMessageChannel();
    if(!mds_message_channel)
        throw chaos::CException(-1, "No mds channel found", __PRETTY_FUNCTION__);


    StartableService::initImplementation(HealtManager::getInstance(), NULL, "HealtManager", __PRETTY_FUNCTION__);
    runid=time(NULL);

}

int ChaosDatasetIO::setAgeing(uint64_t secs){ageing=secs;}
int ChaosDatasetIO::setStorage(int st){storageType=st;}
int ChaosDatasetIO::setTimeo(uint64_t t){timeo=t;}

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
    std::map<int,ChaosSharedPtr<chaos::common::data::CDataWrapper> >::iterator i;
    for(i=datasets.begin();i!=datasets.end();i++){
        DPD_LDBG<<" removing dataset:"<<i->first;
        (i->second).reset();
    }


}



ChaosDataSet ChaosDatasetIO::allocateDataset(int type){
    std::map<int,ChaosDataSet >::iterator i =datasets.find(type);
    if(i == datasets.end()){
        ChaosDataSet tmp(new chaos::common::data::CDataWrapper );
        datasets[type]=tmp;
        DPD_LDBG<<" allocated dataset:"<<type;

        return tmp;
    }
    DPD_LDBG<<" returning already allocated dataset:"<<type;

    return i->second;
}

// push a dataset
int ChaosDatasetIO::pushDataset(int type) {
    int err = 0;
    //ad producer key
    CDataWrapper*new_dataset=datasets[type].get();
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
    ioLiveDataDriver->storeData(datasetName+chaos::datasetTypeToPostfix(type),new_dataset,(chaos::DataServiceNodeDefinitionType::DSStorageType)storageType,false);


    return err;
}

ChaosDataSet ChaosDatasetIO::getDataset(const std::string &dsname,int type){
    size_t dim;
    ChaosDataSet tmp;
    char*ptr=ioLiveDataDriver->retriveRawData(dsname+chaos::datasetTypeToPostfix(type),&dim);
    if(ptr){
        tmp.reset(new chaos::common::data::CDataWrapper(ptr));

    }

    return tmp;
}

ChaosDataSet ChaosDatasetIO::getDataset(int type){
    size_t dim;
    ChaosDataSet tmp;
    char*ptr=ioLiveDataDriver->retriveRawData(datasetName+chaos::datasetTypeToPostfix(type),&dim);
    if(ptr){
        tmp.reset(new chaos::common::data::CDataWrapper(ptr));

    }

    return tmp;
}

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
void ChaosDatasetIO::createMDSEntry(){
    api_proxy::control_unit::SetInstanceDescriptionHelper cud;

    {
        EXECUTE_CHAOS_API(api_proxy::unit_server::NewUS,timeo,groupName);
    }

    {
        EXECUTE_CHAOS_API(api_proxy::unit_server::ManageCUType,timeo,groupName,"datasetIO",0);
    }

    cud.auto_load=1;
    cud.auto_init=1;
    cud.auto_start=1;
    cud.load_parameter = "";
    cud.control_unit_uid=datasetName;
    cud.default_schedule_delay=1;
    cud.unit_server_uid=groupName;
    cud.control_unit_implementation="datasetIO";
    cud.history_ageing=ageing;
    cud.storage_type=(chaos::DataServiceNodeDefinitionType::DSStorageType)storageType;
    {
        EXECUTE_CHAOS_API(api_proxy::control_unit::SetInstanceDescription,timeo,cud);
    }
    HealtManager::getInstance()->addNewNode(datasetName);

    HealtManager::getInstance()->addNodeMetricValue(datasetName,
                        chaos::NodeHealtDefinitionKey::NODE_HEALT_STATUS,
                        chaos::NodeHealtDefinitionValue::NODE_HEALT_STATUS_START,
                        true);

}

//! register the dataset of ap roducer
int ChaosDatasetIO::registerDataset() {
    CHAOS_ASSERT(mds_message_channel);
    if(datasets.empty()){
        DPD_LERR<<" NO DATASET ALLOCATED";

        return -3;
    }
    if(entry_created==false){
        createMDSEntry();
        entry_created=true;
    }
    std::map<int,ChaosSharedPtr<chaos::common::data::CDataWrapper> >::iterator i;

    for(i=datasets.begin();i!=datasets.end();i++){


    CDataWrapper mds_registration_pack=wrapper2dataset(*((i->second).get()),i->first);
    DEBUG_CODE(DPD_LDBG << mds_registration_pack.getJSONString());

    int ret;

    mds_registration_pack.addStringValue(chaos::NodeDefinitionKey::NODE_UNIQUE_ID, datasetName);
    mds_registration_pack.addStringValue(chaos::NodeDefinitionKey::NODE_RPC_DOMAIN, chaos::common::utility::UUIDUtil::generateUUIDLite());
    mds_registration_pack.addStringValue(chaos::NodeDefinitionKey::NODE_RPC_ADDR, network_broker->getRPCUrl());
    mds_registration_pack.addStringValue("mds_control_key","none");
    mds_registration_pack.addStringValue(chaos::NodeDefinitionKey::NODE_TYPE, chaos::NodeType::NODE_TYPE_CONTROL_UNIT);
    DPD_LDBG<<"registering "<<i->first<<" registration pack:"<<mds_registration_pack.getCompliantJSONString();

    if((ret=mds_message_channel->sendNodeRegistration(mds_registration_pack, true, 10000)) ==0){
        CDataWrapper mdsPack;
        mdsPack.addStringValue(chaos::NodeDefinitionKey::NODE_UNIQUE_ID, datasetName);
        mdsPack.addStringValue(chaos::NodeDefinitionKey::NODE_TYPE, chaos::NodeType::NODE_TYPE_CONTROL_UNIT);
        ret = mds_message_channel->sendNodeLoadCompletion(mdsPack, true, 10000);

    } else {
        DPD_LERR<<" cannot register dataset "<<i->first<<" registration pack:"<<mds_registration_pack.getCompliantJSONString();
        return -1;
    }
    }
    return 0;
}

uint64_t ChaosDatasetIO::queryHistoryDatasets(const std::string &dsname,int type, uint64_t ms_start,uint64_t ms_end,int page){
    return 0;
}
bool ChaosDatasetIO::hasNextPage(uint64_t uid){
    return false;
}
std::vector<ChaosDataSet> ChaosDatasetIO::getNextPage(uint64_t uid){
    std::vector<ChaosDataSet> ret;
    return ret;
}

}
}
