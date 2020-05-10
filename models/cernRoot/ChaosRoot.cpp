/*
 * File:   testDataSetAttribute.cpp
 * Author: michelo
 * test from UI
 * Created on September 10, 2015, 11:24 AM
 */

#include <chaos_metadata_service_client/ChaosMetadataServiceClient.h>
#include <driver/misc/core/ChaosController.h>
using namespace std;
#include "ChaosRoot.h"
#include "TROOT.h"
#include "TRint.h"
#include <chaos/common/healt_system/HealtManager.h>
#include <chaos/common/io/SharedManagedDirecIoDataDriver.h>
#include <driver/misc/models/cernRoot/rootUtil.h>
#include <boost/process/environment.hpp>
#include <string>
/*
 *
 */
using namespace chaos;
using namespace driver::misc;
using namespace chaos::metadata_service_client;
using namespace chaos::common::io;
using namespace chaos::common::utility;
using namespace chaos::common::healt_system;
using namespace chaos::common::data;
#define ROOTERR ERR_LOG(ChaosRoot) 

#define ROOTDBG DBG_LOG(ChaosRoot)

namespace driver {
namespace misc {
namespace root {

ChaosRoot::ChaosRoot(){
  int id=boost::this_process::get_id();
  uid=std::to_string(id);
  ROOTDBG<<" ChaosRoot Process ID:"<<uid;
  rootApp=NULL;
}
ChaosRoot::~ChaosRoot(){
  ROOTDBG<<" ChaosRoot "<<uid<< " remove";
  if(rootApp){
    delete rootApp;
  }
}
void ChaosRoot::init(int argc, const char *argv[]) throw(chaos::CException){
  chaos::ChaosCommon<ChaosRoot>::init(argc, argv);
  InizializableService::initImplementation(SharedManagedDirecIoDataDriver::getInstance(), NULL,"SharedManagedDirecIoDataDriver", __PRETTY_FUNCTION__);
  StartableService::initImplementation(HealtManager::getInstance(), NULL,
                                       "HealthManager", __PRETTY_FUNCTION__);
}
void ChaosRoot::init(istringstream &initStringStream) throw(chaos::CException) {
  chaos::DeclareAction::addActionDescritionInstance<ChaosRoot>(this,&ChaosRoot::updateConfiguration,"chaosroot",NodeDomainAndActionRPC::ACTION_UPDATE_PROPERTY,"Update Root property");
        
  chaos::ChaosCommon<ChaosRoot>::init(initStringStream);
}

void ChaosRoot::start() throw(chaos::CException){
  chaos::common::message::MDSMessageChannel *mds_message_channel;
  mds_message_channel = chaos::common::network::NetworkBroker::getInstance()
                            ->getMetadataserverMessageChannel();

  ChaosUniquePtr<chaos::common::data::CDataWrapper> result(
      new chaos::common::data::CDataWrapper());
  ROOTDBG<<" ChaosRoot UID:"<<uid;

  result->addStringValue(NodeDefinitionKey::NODE_UNIQUE_ID, uid);
  result->addStringValue(chaos::NodeDefinitionKey::NODE_TYPE,
                         chaos::NodeType::NODE_TYPE_ROOT);
  result->addStringValue(NodeDefinitionKey::NODE_RPC_ADDR,
                         chaos::GlobalConfiguration::getInstance()
                             ->getLocalServerAddressAnBasePort());
  result->addStringValue(NodeDefinitionKey::NODE_RPC_DOMAIN, "chaosroot");
   result->addStringValue(NodeDefinitionKey::NODE_DESC,rootopts);
  result->addInt64Value(NodeDefinitionKey::NODE_TIMESTAMP,
                        TimingUtil::getTimeStamp());

  // lock o monitor for waith the end
  try {
    // start all wan interface
    StartableService::startImplementation(HealtManager::getInstance(),
                                          "HealtManager", __PRETTY_FUNCTION__);

    ROOTDBG << "Publishing as:" << uid
              << " registration:" << result->getCompliantJSONString();
    mds_message_channel->sendNodeRegistration(MOVE(result));
    HealtManager::getInstance()->addNewNode(uid);
    HealtManager::getInstance()->addNodeMetricValue(
        uid, NodeHealtDefinitionKey::NODE_HEALT_STATUS,
        NodeHealtDefinitionValue::NODE_HEALT_STATUS_LOAD);
    HealtManager::getInstance()->publishNodeHealt(uid);


  } catch (CException &ex) {
    DECODE_CHAOS_EXCEPTION(ex)
  }
    const char *root_opts[120];
    int nroot_opts = 0;
    root_opts[nroot_opts++] = uid.c_str();
    std::string buf;

    std::stringstream ss(rootopts);
    while (ss >> buf)
    {
        root_opts[nroot_opts++] = buf.c_str();
    }
    rootApp = new TRint("Rint", &nroot_opts, (char **)root_opts,0,0, kFALSE);

  // rootapp->init(NULL);
  // rootapp->start();
  chaos::ChaosCommon<ChaosRoot>::start();
  chaos::common::network::NetworkBroker::getInstance()->disposeMessageChannel(mds_message_channel);

  rootApp->SetPrompt("chaosRoot[%d]>");
  rootApp->Run();

}
void ChaosRoot::setRootOpts( const std::string& opts){
    rootopts=opts;
}
CDWUniquePtr ChaosRoot::updateConfiguration(CDWUniquePtr update_pack) {
        //check to see if the device can ben initialized
        
       // PropertyGroupVectorSDWrapper pg_sdw;
        //pg_sdw.serialization_key = "property";
        //pg_sdw.deserialize(update_pack.get());
        ROOTDBG<<"properties "<< update_pack->getJSONString();
        //update the property
       // PropertyCollector::applyValue(pg_sdw());
        
        return CDWUniquePtr();
    }
 void ChaosRoot::deinit() throw(chaos::CException){
    CHAOS_NOT_THROW(StartableService::deinitImplementation(
                        HealtManager::getInstance(), "HealthManager",
                        __PRETTY_FUNCTION__););
    InizializableService::deinitImplementation(
        SharedManagedDirecIoDataDriver::getInstance(),
        "SharedManagedDirecIoDataDriver", __PRETTY_FUNCTION__);

    chaos::ChaosCommon<ChaosRoot>::deinit();
  }
  void ChaosRoot::stop()  throw(chaos::CException){
    CHAOS_NOT_THROW(StartableService::stopImplementation(
                        HealtManager::getInstance(), "HealthManager",
                        __PRETTY_FUNCTION__););

    chaos::ChaosCommon<ChaosRoot>::stop();
  }
  void ChaosRoot::init(void *init_data) throw(chaos::CException) 
  { chaos::ChaosCommon<ChaosRoot>::init(init_data);
   }
} // namespace cernroot
} // namespace misc
} // namespace driver
