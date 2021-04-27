/*
 * File:   testDataSetAttribute.cpp
 * Author: michelo
 * test from UI
 * Created on September 10, 2015, 11:24 AM
 */

#include <chaos_metadata_service_client/ChaosMetadataServiceClient.h>
#include <driver/misc/core/ChaosController.h>
#include <regex>
using namespace std;
#include "ChaosRoot.h"
#include "TROOT.h"
#include "TRint.h"
#include <chaos/common/healt_system/HealtManager.h>
//#include <chaos/common/io/SharedManagedDirecIoDataDriver.h>
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

#define EXECUTE_CHAOS_API(api_name, time_out, ...)                             \
  ROOTDBG << " "                                                              \
           << " Executing Api:\"" << #api_name << "\" ptr:0x" << std::hex      \
           << GET_CHAOS_API_PTR(api_name).get();                               \
  if (GET_CHAOS_API_PTR(api_name).get() == NULL) {                             \
    throw chaos::CException(-1, "Cannot retrieve API:" #api_name,              \
                            __PRETTY_FUNCTION__);                              \
  }                                                                            \
  chaos::metadata_service_client::api_proxy::ApiProxyResult apires =           \
      GET_CHAOS_API_PTR(api_name)->execute(__VA_ARGS__);                       \
  apires->setTimeout(time_out);                                                \
  apires->wait();                                                              \
  if (apires->getError()) {                                                    \
    std::stringstream ss;                                                      \
    ss << " error in :" << __FUNCTION__ << "|" << __LINE__ << "|" << #api_name \
       << " " << apires->getErrorMessage();                                    \
    ROOTERR << ss.str();                                                      \
  }

namespace driver {
namespace misc {
namespace root {

ChaosRoot::ChaosRoot(){
 // int id=boost::this_process::get_id();
 // uid=std::to_string(id);
 // ROOTDBG<<" ChaosRoot Process ID:"<<uid;
  uid="";
  rootApp=NULL;
}
ChaosRoot::~ChaosRoot(){
  ROOTDBG<<" ChaosRoot "<<uid<< " remove";
  if(rootApp){
    delete rootApp;
  }
}
void ChaosRoot::init(int argc, const char *argv[]) throw(chaos::CException){
//  chaos::ChaosCommon<ChaosRoot>::init(argc, argv);
  
  chaos::metadata_service_client::ChaosMetadataServiceClient::getInstance()->getGlobalConfigurationInstance()->addOption("rootopt", po::value<std::string>(&rootopts), "Options to give to CERN ROOT interpreter ");
   
  chaos::metadata_service_client::ChaosMetadataServiceClient::getInstance()->init(argc, argv);
if(GlobalConfiguration::getInstance()->hasOption(InitOption::CONTROL_MANAGER_UNIT_SERVER_ALIAS)){
    uid=GlobalConfiguration::getInstance()->getOption<std::string>(InitOption::CONTROL_MANAGER_UNIT_SERVER_ALIAS);
  }
 
  /*InizializableService::initImplementation(SharedManagedDirecIoDataDriver::getInstance(), NULL,"SharedManagedDirecIoDataDriver", __PRETTY_FUNCTION__);
  StartableService::initImplementation(HealtManager::getInstance(), NULL,
                                       "HealthManager", __PRETTY_FUNCTION__);
  */
}
void ChaosRoot::init(istringstream &initStringStream) throw(chaos::CException) {
//  chaos::ChaosCommon<ChaosRoot>::init(initStringStream);
  chaos::metadata_service_client::ChaosMetadataServiceClient::getInstance()->init(initStringStream);
      
}
chaos::common::data::CDWUniquePtr ChaosRoot::_load(chaos::common::data::CDWUniquePtr dataset_attribute_values){
    ROOTDBG<<" CHAOSROOT LOAD:"<<uid<<dataset_attribute_values->getJSONString();

  return chaos::common::data::CDWUniquePtr();
}

void ChaosRoot::start() throw(chaos::CException){
  chaos::common::message::MDSMessageChannel *mds_message_channel;
  mds_message_channel = chaos::common::network::NetworkBroker::getInstance()
                            ->getMetadataserverMessageChannel();

  ChaosUniquePtr<chaos::common::data::CDataWrapper> result(
      new chaos::common::data::CDataWrapper());
  const char *root_opts[120];
   int nroot_opts = 1;
   int ret;
   // root_opts[nroot_opts++] = uid.c_str();
    std::string buf;

    std::stringstream ss(rootopts);
    while (ss >> buf)
    {
        ROOTDBG<<"options["<<root_opts<<"]="<<buf;

        root_opts[nroot_opts++] = strdup(buf.c_str());
    }
  if(uid.size()==0){
    // if uid not given, use the name of function
    for(int cnt=1;cnt<nroot_opts;cnt++){
      if(*root_opts[cnt]!='-'){
        std::string path=root_opts[cnt];
        regex reg("([-\\.\\w]+)\\({0,1}[-,\\.\\w]*\\){0,1}$");
        smatch m;
        uid=path;
        if(regex_search(path,m,reg)){
          uid=m[1];
        }
      }
    }
  }
  root_opts[0]=uid.c_str();
  /*
  ROOTDBG<<" ChaosRoot UID:\""<<uid<<"\"";

  result->addStringValue(NodeDefinitionKey::NODE_UNIQUE_ID, uid);
  result->addStringValue(chaos::NodeDefinitionKey::NODE_TYPE,
                         chaos::NodeType::NODE_TYPE_ROOT);
  result->addStringValue(NodeDefinitionKey::NODE_RPC_ADDR,
                         chaos::GlobalConfiguration::getInstance()
                             ->getLocalServerAddressAnBasePort());
  result->addStringValue(NodeDefinitionKey::NODE_RPC_DOMAIN, UnitServerNodeDomainAndActionRPC::RPC_DOMAIN);
   result->addStringValue(NodeDefinitionKey::NODE_DESC,rootopts);
  result->addInt64Value(NodeDefinitionKey::NODE_TIMESTAMP,
                        TimingUtil::getTimeStamp());
result->addStringValue(NodeDefinitionKey::NODE_BUILD_INFO,
                           chaos::metadata_service_client::ChaosMetadataServiceClient::getInstance()->getBuildInfo(chaos::common::data::CDWUniquePtr ())->getJSONString());
  // lock o monitor for waith the end
  
    // start all wan interface
    StartableService::startImplementation(HealtManager::getInstance(),
                                          "HealtManager", __PRETTY_FUNCTION__);

    ROOTDBG << "Publishing as:" << uid
              << " registration:" << result->getCompliantJSONString();


    addActionDescritionInstance<ChaosRoot>(
      this, &ChaosRoot::_load,
      UnitServerNodeDomainAndActionRPC::RPC_DOMAIN,
      UnitServerNodeDomainAndActionRPC::ACTION_UNIT_SERVER_LOAD_CONTROL_UNIT,
      "Attempt to load");
   chaos::common::network::NetworkBroker::getInstance()->registerAction(this);

    if((ret=mds_message_channel->sendNodeRegistration(MOVE(result),true))!=0){
          ROOTERR << "cannot register:"<<uid;
          throw chaos::CException(ret,"cannot register "+uid,__PRETTY_FUNCTION__);

    }
  try {
    HealtManager::getInstance()->addNewNode(uid);
    HealtManager::getInstance()->addNodeMetricValue(
        uid, NodeHealtDefinitionKey::NODE_HEALT_STATUS,
        NodeHealtDefinitionValue::NODE_HEALT_STATUS_START);
    HealtManager::getInstance()->publishNodeHealt(uid);

  {
    EXECUTE_CHAOS_API(api_proxy::unit_server::LoadUnloadControlUnit, 5000, uid,
                      true);
  }

  } catch (CException &ex) {
    DECODE_CHAOS_EXCEPTION(ex)
  }
    */
     ::driver::misc::ChaosDatasetIO::ownerApp=uid;

    rootApp = new TRint("Rint", &nroot_opts, (char **)root_opts,0,0, kFALSE);
 
  // rootapp->init(NULL);
  // rootapp->start();
//chaos::ChaosCommon<ChaosRoot>::start();
  chaos::metadata_service_client::ChaosMetadataServiceClient::getInstance()->start();

  chaos::common::network::NetworkBroker::getInstance()->disposeMessageChannel(mds_message_channel);

  rootApp->SetPrompt("chaosRoot[%d]>");
  rootApp->Run();

}
void ChaosRoot::setRootOpts( const std::string& opts){
    rootopts=opts;
}

 void ChaosRoot::deinit() throw(chaos::CException){
   /* CHAOS_NOT_THROW(StartableService::deinitImplementation(
                        HealtManager::getInstance(), "HealthManager",
                        __PRETTY_FUNCTION__););
    InizializableService::deinitImplementation(
        SharedManagedDirecIoDataDriver::getInstance(),
        "SharedManagedDirecIoDataDriver", __PRETTY_FUNCTION__);

   // chaos::ChaosCommon<ChaosRoot>::deinit();
   */
    chaos::metadata_service_client::ChaosMetadataServiceClient::getInstance()->deinit();
    

  }
  void ChaosRoot::stop()  throw(chaos::CException){
    CHAOS_NOT_THROW(StartableService::stopImplementation(
                        HealtManager::getInstance(), "HealthManager",
                        __PRETTY_FUNCTION__););
   //chaos::ChaosCommon<ChaosRoot>::stop();

    chaos::metadata_service_client::ChaosMetadataServiceClient::getInstance()->stop();

  }
  void ChaosRoot::init(void *init_data) throw(chaos::CException) { 
 //   chaos::ChaosCommon<ChaosRoot>::init(init_data);
    chaos::metadata_service_client::ChaosMetadataServiceClient::getInstance()->init(init_data);
   }
} // namespace cernroot
} // namespace misc
} // namespace driver
