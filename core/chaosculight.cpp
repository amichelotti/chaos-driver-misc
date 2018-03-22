#include "chaosculight.h"

#define DPD_LOG_HEAD "[ChaosCULight] - "

#define DPD_LAPP LAPP_ << DPD_LOG_HEAD
#define DPD_LDBG LDBG_ << DPD_LOG_HEAD << __PRETTY_FUNCTION__
#define DPD_LERR LERR_ << DPD_LOG_HEAD << __PRETTY_FUNCTION__ << "(" << __LINE__ << ") "


namespace driver{
namespace misc{
ChaosCULight::ChaosCULight(){
    network_broker=NetworkBroker::getInstance();
    mds_message_channel = network_broker->getMetadataserverMessageChannel();
    if(!mds_message_channel)
        throw chaos::CException(-1, "No mds channel found", __PRETTY_FUNCTION__);

    //! get the direct io client
    direct_io_client = network_broker->getSharedDirectIOClientInstance();
    //checkif someone has passed us the device indetification
    DPD_LAPP << "Scan the direction address";

    CDataWrapper *tmp_data_handler = NULL;

    if(!mds_message_channel->getDataDriverBestConfiguration(&tmp_data_handler, 5000)){
        if(tmp_data_handler!=NULL){
            ChaosUniquePtr<chaos::common::data::CDataWrapper> best_available_da_ptr(tmp_data_handler);
            DPD_LDBG <<best_available_da_ptr->getJSONString();
            ChaosUniquePtr<chaos::common::data::CMultiTypeDataArrayWrapper> liveMemAddrConfig(best_available_da_ptr->getVectorValue(DataServiceNodeDefinitionKey::DS_DIRECT_IO_FULL_ADDRESS_LIST));
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
ChaosCULight::~ChaosCULight(){
    connection_feeder.clear();

    //if(direct_io_client) {
    //CHAOS_NOT_THROW(InizializableService::deinitImplementation(direct_io_client,
    //														   direct_io_client->getName(),
    //														   __PRETTY_FUNCTION__);)
    //delete(direct_io_client);
    //}

    if(mds_message_channel) network_broker->disposeMessageChannel(mds_message_channel);
}
int ChaosCULight::registerCU (const std::string&name,const chaos::common::data::CDataWrapper& dataset){
    if(network_broker == NULL){
    }



}
int ChaosCULight::pushDataset(const chaos::common::data::CDataWrapper& dataset){

}
int ChaosCULight::deinit(){

}
}
}
