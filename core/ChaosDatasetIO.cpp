#include "ChaosDatasetIO.h"
#include <chaos/common/direct_io/DirectIOClient.h>
#include <chaos/common/direct_io/DirectIOClientConnection.h>
#include <chaos/common/healt_system/HealtManager.h>
#include <chaos/common/io/SharedManagedDirecIoDataDriver.h>
#include <chaos/common/metadata_logging/metadata_logging.h>
#include <chaos/cu_toolkit/data_manager/DataManager.h>
#include <chaos_metadata_service_client/api_proxy/control_unit/DeleteInstance.h>
#include <chaos_metadata_service_client/api_proxy/control_unit/SetInstanceDescription.h>
#include <chaos_metadata_service_client/api_proxy/unit_server/ManageCUType.h>
#include <chaos_metadata_service_client/api_proxy/unit_server/NewUS.h>

#include <chaos/common/network/NetworkBroker.h>

#define DPD_LOG_HEAD "[ChaosDatasetIO-" << uid << "] - "
#define DPD_LAPP LAPP_ << DPD_LOG_HEAD
#define DPD_LDBG LDBG_ << DPD_LOG_HEAD << __PRETTY_FUNCTION__
#define DPD_LERR \
  LERR_ << DPD_LOG_HEAD << __PRETTY_FUNCTION__ << "(" << __LINE__ << ") "

#include <chaos_metadata_service_client/ChaosMetadataServiceClient.h>
using namespace chaos;
using namespace chaos::common::data;
using namespace chaos::common::utility;
using namespace chaos::common::property;
using namespace chaos::cu::data_manager;

using namespace chaos::common::network;
using namespace chaos::common::direct_io;
using namespace chaos::common::direct_io::channel;
using namespace chaos::common::healt_system;
using namespace chaos::metadata_service_client;
using namespace chaos::common::metadata_logging;
#define EXECUTE_CHAOS_API(api_name, time_out, ...)                                      \
  DPD_LDBG << " "                                                                       \
           << " Executing Api:\"" << #api_name << "\" ptr:0x" << std::hex               \
           << GET_CHAOS_API_PTR(api_name).get();                                        \
  if (GET_CHAOS_API_PTR(api_name).get() == NULL) {                                      \
    throw chaos::CException(-1, "Cannot retrieve API:" #api_name, __PRETTY_FUNCTION__); \
  }                                                                                     \
  chaos::metadata_service_client::api_proxy::ApiProxyResult apires =                    \
      GET_CHAOS_API_PTR(api_name)->execute(__VA_ARGS__);                                \
  apires->setTimeout(time_out);                                                         \
  apires->wait();                                                                       \
  if (apires->getError()) {                                                             \
    std::stringstream ss;                                                               \
    ss << " error in :" << __FUNCTION__ << "|" << __LINE__ << "|" << #api_name          \
       << " " << apires->getErrorMessage();                                             \
    DPD_LERR << ss.str();                                                               \
  }

template <typename t>
bool testRange(t v, std::string &minRange, std::string &maxRange) {
  t           max, min;
  std::size_t mini = minRange.find("0x");
  std::size_t maxi = maxRange.find("0x");
  std::string maxs, mins;
  if (maxi != std::string::npos) {
    maxs = maxRange.substr(maxi);
  } else {
    maxs = maxRange;
  }
  if (mini != std::string::npos) {
    mins = minRange.substr(mini);
  } else {
    mins = minRange;
  }

  try {
    min = boost::lexical_cast<t>(mins);
  } catch (std::exception &e) {
    LERR_ << boost::str(
        boost::format("Invalid casting MIN value (%1%) value %2%") % mins % v);
    return false;
  }
  try {
    max = boost::lexical_cast<t>(maxs);
  } catch (std::exception &e) {
    LERR_ << boost::str(
        boost::format("Invalid casting MAX value (%1%) value %2%") % maxs % v);
    return false;
  }

  if (((mins.size() > 0) && (v < min)) || ((maxs.size() > 0) && (v > max))) {
    LERR_ << boost::str(
        boost::format("Invalid value (%1%) [Min:%2%-%3% Max:%4%-%5%]") % v %
        min % mins % max % maxs);
    return false;
  }

  return true;
}
namespace driver {
namespace misc {
ChaosSharedMutex ChaosDatasetIO::iomutex;

std::string ChaosDatasetIO::ownerApp;
ChaosDatasetIO::ChaosDatasetIO(const std::string &dataset_name, bool check)
    : check_presence(check), datasetName(dataset_name), groupName(""), ageing(3600), storageType((int)chaos::DataServiceNodeDefinitionType::DSStorageTypeLiveHistory), timeo(5000), entry_created(false), query_index(0), defaultPage(30), last_seq(0), packet_size(0), cu_alarm_lvl(0), dev_alarm_lvl(0), alarm_logging_channel(NULL), standard_logging_channel(NULL), last_push_rate_grap_ts(0), deinitialized(false), implementation("datasetIO"), sched_time(0), last_push_ts(0), push_errors(0), packet_lost(0), packet_tot_size(0), burst_cycles(0), burst_time_ts(0), state(chaos::CUStateKey::DEINIT) {
  if (ownerApp.size()) {
    uid = ownerApp;
  } else {
    if (groupName == "") {
      uid = datasetName;
    } else {
      uid = groupName + "/" + datasetName;
    }
  }
  _initDataset();
}

ChaosDatasetIO::ChaosDatasetIO(const std::string &name,
                               const std::string &group_name)
    : datasetName(name), groupName(group_name), ageing(3600), storageType((int)chaos::DataServiceNodeDefinitionType::DSStorageTypeLiveHistory), timeo(5000), entry_created(false), query_index(0), defaultPage(30), last_seq(0), packet_size(0), cu_alarm_lvl(0), dev_alarm_lvl(0), alarm_logging_channel(NULL), standard_logging_channel(NULL), last_push_rate_grap_ts(0), deinitialized(false), implementation("datasetIO"), sched_time(0), last_push_ts(0), push_errors(0), packet_lost(0), packet_tot_size(0), burst_cycles(0), burst_time_ts(0), state(chaos::CUStateKey::DEINIT), check_presence(true) {
  runid = time(NULL);
  if (ownerApp.size()) {
    uid = ownerApp;
  } else {
    if (groupName == "") {
      uid = datasetName;
    } else {
      uid = groupName + "/" + datasetName;
    }
  }
  _initDataset();
}

void ChaosDatasetIO::_initDataset() {
  try {
    ChaosWriteLock l(iomutex);

    if (chaos::common::io::SharedManagedDirecIoDataDriver::getInstance()
            ->getServiceState() == chaos::CUStateKey::DEINIT) {
      InizializableService::initImplementation(
          chaos::common::io::SharedManagedDirecIoDataDriver::getInstance(),
          NULL,
          "SharedManagedDirecIoDataDriver",
          __PRETTY_FUNCTION__);
    } else {
      DPD_LDBG
          << " SharedManagedDirecIoDataDriver already initialized state:"
          << chaos::common::io::SharedManagedDirecIoDataDriver::getInstance()
                 ->getServiceState();
    }
    /*ioLiveShDataDriver =
        chaos::common::io::SharedManagedDirecIoDataDriver::getInstance()
            ->getSharedDriver();*/
  } catch (...) {
    DPD_LERR << "cannot initialize SharedManagedDirecIoDataDriver, already "
                "initialized?";
  }
  {
    ChaosWriteLock l(iomutex);

    // pool
    ioLiveDataDriver = chaos::metadata_service_client::
                           ChaosMetadataServiceClient::getInstance()
                               ->getDataProxyChannelNewInstance();
    DPD_LDBG << "retrived iodata driver";
  }

  if (ioLiveDataDriver.get() == NULL) {
    throw chaos::CException(-1, "cannot access ioLive driver" + datasetName, __PRETTY_FUNCTION__);
  }
  network_broker = NetworkBroker::getInstance();

  if (network_broker == NULL) {
    throw chaos::CException(-1, "cannot access network broker " + datasetName, __PRETTY_FUNCTION__);
  }
  mds_message_channel = network_broker->getMetadataserverMessageChannel();

  if (mds_message_channel == NULL) {
    throw chaos::CException(-1, "cannot access MDS channel " + datasetName, __PRETTY_FUNCTION__);
  }
  CDWUniquePtr tmp_data_handler;

  if (!mds_message_channel->getDataDriverBestConfiguration(tmp_data_handler,
                                                           5000)) {
    DPD_LDBG << "best config:" << tmp_data_handler->getJSONString();
    ioLiveDataDriver->updateConfiguration(tmp_data_handler.get());
  } else {
    throw chaos::CException(-1, "cannot retrieve server information ", __PRETTY_FUNCTION__);
  }

  int err;
  // check if there is some other CU with the same id.
  if (check_presence) {
    int retry = 12;
    do {
      ChaosStringVector node_found;

      if ((err = mds_message_channel->searchNode(
               uid, chaos::NodeType::NodeSearchType::node_type_ceu, true, 0, 100, node_found, timeo)) == 0) {
        if (node_found.size() && (uid == node_found[0])) {
          if (retry == 0) {
            throw chaos::CException(
                -1, "another Node with the same uid:\"" + uid + "\" is living", __PRETTY_FUNCTION__);
          } else {
            DPD_LDBG << "nodes:" << node_found.size()
                     << " found with the same uid:\"" << node_found[0]
                     << "\" retry check:" << retry << " in 5s";
            std::cout << "nodes:" << node_found.size()
                      << " found with the same uid:\"" << node_found[0]
                      << "\" retry check:" << retry << " in 5s" << std::endl;
            sleep(5);
          }
        }

      } else {
        throw chaos::CException(-10, "cannot check for node liveness:" + uid, __PRETTY_FUNCTION__);
      }
    } while (retry--);
  }

  for (int cnt = 0; cnt < sizeof(pkids) / sizeof(uint64_t); cnt++) {
    pkids[cnt] = 0;
  }

  alarm_logging_channel =
      (AlarmLoggingChannel *)MetadataLoggingManager::getInstance()->getChannel(
          "AlarmLoggingChannel");
  if (alarm_logging_channel == NULL) {
    DPD_LERR << "Alarm logging channel not found";
    // LOG_AND_TROW(DPD_LERR, -1, "Alarm logging channel not found");
  }

  standard_logging_channel =
      (StandardLoggingChannel *)MetadataLoggingManager::getInstance()
          ->getChannel("StandardLoggingChannel");
  if (standard_logging_channel == NULL) {
    DPD_LERR << "Standard logging channel not found";

    // LOG_AND_TROW(DPD_LERR, -2, "Standard logging channel not found");
  } else {
    standard_logging_channel->setLogLevel(
        chaos::common::metadata_logging::StandardLoggingChannel::LogLevelInfo);
  }

  ChaosDataSet sys =
      allocateDataset(chaos::DataPackCommonKey::DPCK_DATASET_TYPE_SYSTEM);
  sys->addStringValue(chaos::DataPackSystemKey::DP_SYS_UNIT_TYPE,
                      chaos::NodeType::NODE_SUBTYPE_SCRIPTABLE_EXECUTION_UNIT);
  sys->addInt32Value(chaos::ControlUnitDatapackSystemKey::DEV_ALRM_LEVEL, 0);
  sys->addInt32Value(chaos::ControlUnitDatapackSystemKey::CU_ALRM_LEVEL, 0);
  sys->addBoolValue(chaos::ControlUnitDatapackSystemKey::BYPASS_STATE, false);
  sys->addInt32Value(chaos::ControlUnitDatapackSystemKey::THREAD_SCHEDULE_DELAY,
                     0);
  sys->addInt32Value(
      chaos::DataServiceNodeDefinitionKey::DS_STORAGE_HISTORY_TIME, ageing);
  sys->addInt32Value(chaos::DataServiceNodeDefinitionKey::DS_STORAGE_LIVE_TIME,
                     0);
  sys->addBoolValue(ControlUnitDatapackSystemKey::BURST_STATE, false);
  sys->addStringValue(ControlUnitDatapackSystemKey::BURST_TAG, "", 80);

  allocateDataset(chaos::DataPackCommonKey::DPCK_DATASET_TYPE_DEV_ALARM);
  allocateDataset(chaos::DataPackCommonKey::DPCK_DATASET_TYPE_CU_ALARM);
  allocateCUAlarm("packet_lost");
  allocateCUAlarm("packet_send_error");
  /// register actions
  DeclareAction::addActionDescritionInstance<ChaosDatasetIO>(
      this, &ChaosDatasetIO::_registrationAck, UnitServerNodeDomainAndActionRPC::RPC_DOMAIN, UnitServerNodeDomainAndActionRPC::ACTION_REGISTRATION_CONTROL_UNIT_ACK, "Update Command Manager Configuration");

  addActionDescritionInstance<ChaosDatasetIO>(
      this, &ChaosDatasetIO::_setDatasetAttribute, uid, ControlUnitNodeDomainAndActionRPC::CONTROL_UNIT_APPLY_INPUT_DATASET_ATTRIBUTE_CHANGE_SET, "method for setting the input element for the dataset");

  addActionDescritionInstance<ChaosDatasetIO>(
      this, &ChaosDatasetIO::_init, uid, NodeDomainAndActionRPC::ACTION_NODE_INIT, "Perform the control unit initialization");

  addActionDescritionInstance<ChaosDatasetIO>(
      this, &ChaosDatasetIO::_load, UnitServerNodeDomainAndActionRPC::RPC_DOMAIN, UnitServerNodeDomainAndActionRPC::ACTION_UNIT_SERVER_LOAD_CONTROL_UNIT, "Attempt to load");
  addActionDescritionInstance<ChaosDatasetIO>(
      this, &ChaosDatasetIO::setProperty, UnitServerNodeDomainAndActionRPC::RPC_DOMAIN, NodeDomainAndActionRPC::ACTION_SET_PROPERTIES, "method for set properties");
  addActionDescritionInstance<ChaosDatasetIO>(
      this, &ChaosDatasetIO::getProperty, UnitServerNodeDomainAndActionRPC::RPC_DOMAIN, NodeDomainAndActionRPC::ACTION_GET_PROPERTIES, "method for get properties");

  addActionDescritionInstance<ChaosDatasetIO>(
      this, &ChaosDatasetIO::_unload, UnitServerNodeDomainAndActionRPC::RPC_DOMAIN, UnitServerNodeDomainAndActionRPC::ACTION_UNIT_SERVER_UNLOAD_CONTROL_UNIT, "Unload");

  chaos::DeclareAction::addActionDescritionInstance<ChaosDatasetIO>(
      this, &ChaosDatasetIO::_updateConfiguration, uid, chaos::NodeDomainAndActionRPC::ACTION_UPDATE_PROPERTY, "Update Dataset property");

  addActionDescritionInstance<ChaosDatasetIO>(
      this, &ChaosDatasetIO::_deinit, uid, NodeDomainAndActionRPC::ACTION_NODE_DEINIT, "Perform the control unit deinitialization");
  addActionDescritionInstance<ChaosDatasetIO>(
      this, &ChaosDatasetIO::_start, uid, NodeDomainAndActionRPC::ACTION_NODE_START, "Start the control unit scheduling");

  addActionDescritionInstance<ChaosDatasetIO>(
      this, &ChaosDatasetIO::_stop, uid, NodeDomainAndActionRPC::ACTION_NODE_STOP, "Stop the control unit scheduling");

  addActionDescritionInstance<ChaosDatasetIO>(
      this, &ChaosDatasetIO::_getInfo, uid, NodeDomainAndActionRPC::ACTION_CU_GET_INFO, "Get the information about running control unit");
  addActionDescritionInstance<ChaosDatasetIO>(
      this, &ChaosDatasetIO::_submitStorageBurst, uid, ControlUnitNodeDomainAndActionRPC::ACTION_STORAGE_BURST, "Execute a storage burst on control unit");

  NetworkBroker::getInstance()->registerAction(this);

  _initPropertyGroup();
}

int ChaosDatasetIO::setAgeing(uint64_t secs) {
  ageing = secs;
  datasets[chaos::DataPackCommonKey::DPCK_DATASET_TYPE_SYSTEM]->setValue(
      chaos::DataServiceNodeDefinitionKey::DS_STORAGE_HISTORY_TIME,
      (int32_t)ageing);
  DPD_LDBG << "Set Ageing:" << ageing;

  return 0;
}
int ChaosDatasetIO::setStorage(int st) {
  storageType      = st;
  ChaosDataSet sys = datasets[chaos::DataPackCommonKey::DPCK_DATASET_TYPE_SYSTEM];
  sys->setValue(chaos::DataServiceNodeDefinitionKey::DS_STORAGE_TYPE, (int32_t)storageType);
  DPD_LDBG << "Set Storage:" << storageType << " json:" << sys->getJSONString();

  return 0;
}

int ChaosDatasetIO::setSchedule(int st) {
  sched_time = st;
  datasets[chaos::DataPackCommonKey::DPCK_DATASET_TYPE_SYSTEM]->setValue(
      ControlUnitDatapackSystemKey::THREAD_SCHEDULE_DELAY, (int32_t)sched_time);
  DPD_LDBG << "Set Schedule:" << st;

  return 0;
}
int ChaosDatasetIO::setTimeo(uint64_t t) {
  timeo = t;
  return 0;
}

ChaosDatasetIO::~ChaosDatasetIO() {
  state = chaos::CUStateKey::DEINIT;
  DPD_LDBG << "destroying";

  deinit();
  waitEU.notifyAll();
}

void ChaosDatasetIO::updateHealth() {
  uint64_t rate_acq_ts = TimingUtil::getTimeStamp();
  double   time_offset = (double(rate_acq_ts - last_push_rate_grap_ts)) / 1000.0;
  uint32_t npushes =
      (pkids[chaos::DataPackCommonKey::DPCK_DATASET_TYPE_OUTPUT] -
       last_seq);  // time in seconds
  double output_ds_rate =
      (time_offset > 0) ? npushes / time_offset : 0;  // rate in seconds
  int32_t output_ds_size =
      (npushes > 0) ? packet_size / npushes : 0;  // rate in seconds

  HealtManager::getInstance()->addNodeMetricValue(
      uid,
      chaos::ControlUnitHealtDefinitionValue::CU_HEALT_OUTPUT_DATASET_PUSH_RATE,
      output_ds_rate,
      true);
  HealtManager::getInstance()->addNodeMetricValue(
      uid,
      chaos::ControlUnitHealtDefinitionValue::CU_HEALT_OUTPUT_ALARM_LEVEL,
      std::max(cu_alarm_lvl, dev_alarm_lvl),
      true);

  HealtManager::getInstance()->addNodeMetricValue(
      uid,
      chaos::ControlUnitHealtDefinitionValue::CU_HEALT_OUTPUT_DATASET_PUSH_SIZE,
      output_ds_size,
      true);
  HealtManager::getInstance()->addNodeMetricValue(uid,
                                                  ControlUnitHealtDefinitionValue::CU_HEALT_OUTPUT_DATASET_PUSH_ERROR,
                                                  (int32_t)push_errors);

  HealtManager::getInstance()->addNodeMetricValue(uid,
                                                  ControlUnitHealtDefinitionValue::CU_HEALT_OUTPUT_DATASET_PUSH_LOST,
                                                  (int32_t)packet_lost);

  HealtManager::getInstance()->addNodeMetricValue(uid,
                                                  ControlUnitHealtDefinitionValue::CU_HEALT_OUTPUT_TOT_PUSH_KSIZE,
                                                  (int32_t)(packet_tot_size / 1024.0));

  // keep track of acquire timestamp
  last_push_rate_grap_ts = rate_acq_ts;
  packet_size            = 0;
  // reset pushe count
  last_seq = pkids[chaos::DataPackCommonKey::DPCK_DATASET_TYPE_OUTPUT];
}
ChaosDataSet ChaosDatasetIO::allocateDataset(int type) {
  std::map<int, ChaosDataSet>::iterator i = datasets.find(type);
  if (i == datasets.end()) {
    ChaosDataSet new_dataset(new chaos::common::data::CDataWrapper);
    datasets[type] = new_dataset;
    DPD_LDBG << " allocated dataset:" << type;
    new_dataset->addInt64Value(chaos::DataPackCommonKey::DPCK_TIMESTAMP, (int64_t)0);
    new_dataset->addInt64Value(chaos::DataPackCommonKey::DPCK_HIGH_RESOLUTION_TIMESTAMP, (int64_t)0);
    new_dataset->addInt64Value(chaos::DataPackCommonKey::DPCK_SEQ_ID, (int64_t)0);
    new_dataset->addInt64Value(chaos::ControlUnitNodeDefinitionKey::CONTROL_UNIT_RUN_ID, (int64_t)runid);
    new_dataset->addStringValue(chaos::DataPackCommonKey::DPCK_DEVICE_ID, uid);
    new_dataset->addInt32Value(chaos::DataPackCommonKey::DPCK_DATASET_TYPE, type);
    new_dataset->addInt32Value(DataServiceNodeDefinitionKey::DS_STORAGE_TYPE, storageType);

    return new_dataset;
  }
  DPD_LDBG << " returning already allocated dataset:" << type;

  return i->second;
}
int ChaosDatasetIO::updateSystem() {
  return pushDataset(chaos::DataPackCommonKey::DPCK_DATASET_TYPE_SYSTEM);
}

int ChaosDatasetIO::pushDataset(ChaosDataSet &new_dataset, int type) {
  int err = 0;

  uint64_t ts = chaos::common::utility::TimingUtil::getTimeCorStamp();
  uint64_t tsh =
      chaos::common::utility::TimingUtil::getTimeStampInMicroseconds();
  if (new_dataset.get() == NULL) {
    DPD_LERR << "attempting to psuh a non valid dataset";
    return -1;
  }
  if (!new_dataset->hasKey((chaos::DataPackCommonKey::DPCK_TIMESTAMP))) {
    // add timestamp of the datapack
    new_dataset->addInt64Value(chaos::DataPackCommonKey::DPCK_TIMESTAMP, ts);
  } else {
    new_dataset->setValue(chaos::DataPackCommonKey::DPCK_TIMESTAMP,
                          (int64_t)ts);
  }
  if (!new_dataset->hasKey(
          (chaos::DataPackCommonKey::DPCK_HIGH_RESOLUTION_TIMESTAMP))) {
    // add timestamp of the datapack
    new_dataset->addInt64Value(
        chaos::DataPackCommonKey::DPCK_HIGH_RESOLUTION_TIMESTAMP, tsh);
  } else {
    new_dataset->setValue(
        chaos::DataPackCommonKey::DPCK_HIGH_RESOLUTION_TIMESTAMP, (int64_t)tsh);
  }
  if (!new_dataset->hasKey(chaos::DataPackCommonKey::DPCK_SEQ_ID)) {
    new_dataset->addInt64Value(chaos::DataPackCommonKey::DPCK_SEQ_ID,
                               pkids[type]++);
  } else {
    new_dataset->setValue(chaos::DataPackCommonKey::DPCK_SEQ_ID,
                          (int64_t)pkids[type]++);
  }
  if (!new_dataset->hasKey(
          chaos::ControlUnitNodeDefinitionKey::CONTROL_UNIT_RUN_ID)) {
    new_dataset->addInt64Value(
        chaos::ControlUnitNodeDefinitionKey::CONTROL_UNIT_RUN_ID, runid);
  }
  if (!new_dataset->hasKey(chaos::DataPackCommonKey::DPCK_DEVICE_ID)) {
    new_dataset->addStringValue(chaos::DataPackCommonKey::DPCK_DEVICE_ID, uid);
  }
  if (!new_dataset->hasKey(chaos::DataPackCommonKey::DPCK_DATASET_TYPE)) {
    new_dataset->addInt32Value(chaos::DataPackCommonKey::DPCK_DATASET_TYPE,
                               type);
  }
  int sttype = ((type == (int)chaos::DataPackCommonKey::DPCK_DATASET_TYPE_SYSTEM)
                    ? ((int)(0x2 | storageType))
                    : storageType);

  if (!new_dataset->hasKey(chaos::DataServiceNodeDefinitionKey::DS_STORAGE_TYPE)) {
    new_dataset->addInt32Value(DataServiceNodeDefinitionKey::DS_STORAGE_TYPE, sttype);

  } else {
    new_dataset->setValue(DataServiceNodeDefinitionKey::DS_STORAGE_TYPE, sttype);
  }
  int psize = new_dataset->getBSONRawSize();
  packet_size += psize;

  // ChaosUniquePtr<SerializationBuffer>
  // serialization(new_dataset->getBSONData());
  //    DPD_LDBG <<" PUSHING:"<<new_dataset->getJSONString();
  // DirectIOChannelsInfo    *next_client =
  // static_cast<DirectIOChannelsInfo*>(connection_feeder.getService());
  // serialization->disposeOnDelete = !next_client;

  if ((type == (int)chaos::DataPackCommonKey::DPCK_DATASET_TYPE_OUTPUT) &&
      (sched_time > 0)) {
    int64_t diff = ts - last_push_ts;
    if (diff < sched_time) {
      usleep((sched_time - diff));
    }
  }
  if (((burst_cycles > 0) && (--burst_cycles == 0)) ||
      ((burst_time_ts > 0) && (burst_time_ts >= ts))) {
    datasets[chaos::DataPackCommonKey::DPCK_DATASET_TYPE_SYSTEM]->setValue(
        ControlUnitDatapackSystemKey::BURST_STATE, false);
    datasets[chaos::DataPackCommonKey::DPCK_DATASET_TYPE_SYSTEM]->setValue(
        ControlUnitDatapackSystemKey::BURST_TAG, std::string(""));
    burst_time_ts = 0;

    updateSystem();
    DPD_LDBG << " exit BURST";
  }
  if ((state == chaos::CUStateKey::START) ||
      (type != chaos::DataPackCommonKey::DPCK_DATASET_TYPE_OUTPUT)) {
    int retry = 10;
    packet_tot_size += psize;
    do {
      err = ioLiveDataDriver->storeData(
          uid + chaos::datasetTypeToPostfix(type), new_dataset, (chaos::DataServiceNodeDefinitionType::DSStorageType)(sttype | ((type != chaos::DataPackCommonKey::DPCK_DATASET_TYPE_OUTPUT) ? 0x2 : 0x0)));
      if (err != 0) {
        push_errors++;
        DPD_LERR << push_errors << "] ERROR pushing runid:" << runid << " seq:" << new_dataset->getInt64Value(DataPackCommonKey::DPCK_SEQ_ID);
        usleep(10000);
        setCUAlarmLevel("packet_send_error", 1);
      }
    } while ((err != 0) && (retry--));
    if (retry < 0) {
      packet_lost++;

      setCUAlarmLevel("packet_lost", packet_lost);
    }
    last_push_ts = ts;
  } else if (type == (int)chaos::DataPackCommonKey::DPCK_DATASET_TYPE_OUTPUT) {
    DPD_LDBG << "STOPPED ";
    cout << "==== STOP====" << std::endl;
    waitEU.wait();
    DPD_LDBG << "RESTARTED ";
    cout << "==== START ====" << std::endl;
  }
  return err;
}
int ChaosDatasetIO::notifyAllClients(const std::string &msg, int errorLevel) {
  chaos::common::data::CDWShrdPtr ptr(new chaos::common::data::CDataWrapper());
  ptr->addStringValue("msg", msg);
  ptr->addStringValue("date", chaos::common::utility::TimingUtil::toString(chaos::common::utility::TimingUtil::getTimeCorStamp()));
  ptr->addStringValue("type", ((errorLevel == 2) ? "alarm" : "system"));
  ptr->addStringValue("dst", "broadcast");
  ptr->addStringValue("src", network_broker->getRPCUrl());
  ptr->addStringValue("username", uid);

  int err = ioLiveDataDriver->storeData("chaos_web_log", ptr, (chaos::DataServiceNodeDefinitionType::DSStorageType)0);
  return err;
}

// push a dataset
int ChaosDatasetIO::pushDataset(int type) {
  // ad producer key
  return pushDataset(datasets[type], type);
}
ChaosDataSet ChaosDatasetIO::getDataset(int type) { return datasets[type]; }

ChaosDataSet ChaosDatasetIO::getLiveDataset(const std::string &dsname,
                                            int                type) {
  size_t       dim;
  ChaosDataSet tmp;
  char *       ptr = ioLiveDataDriver->retriveRawData(
      dsname + chaos::datasetTypeToPostfix(type), &dim);
  if (ptr) {
    tmp.reset(new chaos::common::data::CDataWrapper(ptr));
    free(ptr);
  }

  return tmp;
}

ChaosDataSet ChaosDatasetIO::getLiveDataset(int type) {
  size_t       dim;
  ChaosDataSet tmp;
  char *       ptr = ioLiveDataDriver->retriveRawData(
      uid + chaos::datasetTypeToPostfix(type), &dim);
  if (ptr) {
    tmp.reset(new chaos::common::data::CDataWrapper(ptr));
    free(ptr);
  }

  return tmp;
}

ChaosDataSet ChaosDatasetIO::allocateDataset(const std::string &json,
                                             int                type) {
  ChaosDataSet ret = allocateDataset(type);
  ret->setSerializedJsonData(json.c_str());
  return ret;
}

void ChaosDatasetIO::wrapper2dataset(
    chaos::common::data::CDataWrapper &      cu_dataset,
    const chaos::common::data::CDataWrapper &dataset_pack,
    int                                      dir) {
  ChaosStringVector all_template_key;
  dataset_pack.getAllKey(all_template_key);

  for (ChaosStringVectorIterator it = all_template_key.begin();
       it != all_template_key.end();
       it++) {
    if (dataset_pack.isNullValue(*it)) {
      DPD_LERR << "Removed from template null value key:" << *it;
      continue;
    }
    CDataWrapper      ds;
    int32_t           dstype = 0, subtype = 0;
    int32_t           size = 0;
    std::stringstream ss;
    ds.addStringValue(chaos::ControlUnitNodeDefinitionKey::
                          CONTROL_UNIT_DATASET_ATTRIBUTE_NAME,
                      *it);
    ss << *it;

    size = dataset_pack.getValueSize(*it);
    if (dataset_pack.isDoubleValue(*it)) {
      dstype |= chaos::DataType::TYPE_DOUBLE;
      subtype = chaos::DataType::SUB_TYPE_DOUBLE;
      ss << " double";
    } else if (dataset_pack.isInt64Value(*it)) {
      dstype |= chaos::DataType::TYPE_INT64;
      subtype = chaos::DataType::SUB_TYPE_INT64;
      ss << " int65";

    } else if (dataset_pack.isInt32Value(*it)) {
      dstype |= chaos::DataType::TYPE_INT32;
      subtype = chaos::DataType::SUB_TYPE_INT32;
      ss << " int32";

    } else if (dataset_pack.isStringValue(*it)) {
      dstype |= chaos::DataType::TYPE_STRING;
      subtype = chaos::DataType::SUB_TYPE_STRING;
      ss << " string[" << size << "]";

    } else if (dataset_pack.isBinaryValue(*it)) {
      dstype |= chaos::DataType::TYPE_BYTEARRAY;
      ss << " bin[" << size << "]";

    } else {
      dstype |= chaos::DataType::TYPE_DOUBLE;
      subtype = chaos::DataType::SUB_TYPE_DOUBLE;
    }
    if (dataset_pack.isVector(*it)) {
      // dstype = chaos::DataType::TYPE_ACCESS_ARRAY;
      dstype = chaos::DataType::TYPE_BYTEARRAY;
      ss << "[]";
    }
    ds.addInt32Value(chaos::ControlUnitNodeDefinitionKey::
                         CONTROL_UNIT_DATASET_BINARY_SUBTYPE,
                     subtype);
    ds.addInt32Value(chaos::ControlUnitNodeDefinitionKey::
                         CONTROL_UNIT_DATASET_ATTRIBUTE_TYPE,
                     dstype);
    int real_dir = 0;
    switch (dir) {
      case chaos::DataPackCommonKey::DPCK_DATASET_TYPE_INPUT:
        real_dir = chaos::DataType::Input;
        break;
      case chaos::DataPackCommonKey::DPCK_DATASET_TYPE_OUTPUT:
        real_dir = chaos::DataType::Output;
        break;
      default:
        real_dir = chaos::DataType::Bidirectional;

        break;
    }

    ds.addInt32Value(chaos::ControlUnitNodeDefinitionKey::
                         CONTROL_UNIT_DATASET_ATTRIBUTE_DIRECTION,
                     real_dir);
    ds.addInt32Value(chaos::ControlUnitNodeDefinitionKey::
                         CONTROL_UNIT_DATASET_VALUE_MAX_SIZE,
                     size);
    ds.addStringValue(chaos::ControlUnitNodeDefinitionKey::
                          CONTROL_UNIT_DATASET_ATTRIBUTE_DESCRIPTION,
                      ss.str());

    DPD_LDBG << "- ATTRIBUTE \"" << *it << "\" SIZE:" << size
             << " TYPE:" << dstype << " SUBTYPE:" << subtype;
    cu_dataset.appendCDataWrapperToArray(ds);
  }
}
void ChaosDatasetIO::setImplementation(const std::string &impl) {
  implementation = impl;
}

int ChaosDatasetIO::createMDSEntry() {
  api_proxy::control_unit::SetInstanceDescriptionHelper cud;
  int                                                   error = 0;

  ChaosStringVector node_found;
  error = mds_message_channel->searchNode(uid, chaos::NodeType::NodeSearchType::node_type_ceu, false, 0, 100, node_found, timeo);

  if (error == 0) {
    if (node_found.size()) {
      for (ChaosStringVector::iterator i = node_found.begin();
           i != node_found.end();
           i++) {
        DPD_LDBG << "The container " << *i << " exists";
      }
    } else {
      /* {
         EXECUTE_CHAOS_API(api_proxy::unit_server::NewUS, timeo, groupName);
         error = apires->getError();
       }
       if (error == 0) {
         EXECUTE_CHAOS_API(api_proxy::unit_server::ManageCUType, timeo,
                           groupName, "datasetIO", 0);
         error = apires->getError();
       }*/

      /* if (error == 0) {

        cud.auto_load = 1;
        cud.auto_init = 1;
        cud.auto_start = 1;
        cud.load_parameter = "";
        cud.control_unit_uid = uid;
        cud.default_schedule_delay = 1;
        cud.unit_server_uid = groupName;
        cud.control_unit_implementation = implementation;
        cud.history_ageing = ageing;
        cud.storage_type =
            (chaos::DataServiceNodeDefinitionType::DSStorageType)storageType;
        DPD_LDBG << "Setting CU:" << uid;

        EXECUTE_CHAOS_API(api_proxy::control_unit::SetInstanceDescription,
                          timeo, cud);
      }*/
    }

  } else {
    /* throw chaos::CException(-10, "cannot check for node liveness:" + uid,
                            __PRETTY_FUNCTION__);*/

    return -1;
  }

  return error;
}

//! register the dataset of ap roducer
int ChaosDatasetIO::registerDataset() {
  int ret;
  CHAOS_ASSERT(mds_message_channel);
  if (datasets.empty()) {
    DPD_LERR << " NO DATASET ALLOCATED";

    return -3;
  }
  if (entry_created == false) {
    if ((ret = createMDSEntry()) < 0) {
      return ret;
    }
    entry_created = true;
  }
  CDWUniquePtr mds_registration_pack = CDWUniquePtr(new CDataWrapper());
  mds_registration_pack->addStringValue(
      chaos::NodeDefinitionKey::NODE_UNIQUE_ID, uid);
  mds_registration_pack->addStringValue(
      chaos::NodeDefinitionKey::NODE_RPC_DOMAIN, uid);
  mds_registration_pack->addStringValue(chaos::NodeDefinitionKey::NODE_RPC_ADDR,
                                        network_broker->getRPCUrl());
  mds_registration_pack->addStringValue("mds_control_key", "none");
  // mds_registration_pack->addStringValue(UnitServerNodeDomainAndActionRPC::PARAM_CONTROL_UNIT_TYPE,CUType::SEXUT);

  mds_registration_pack->addStringValue(
      chaos::NodeDefinitionKey::NODE_TYPE,
      chaos::NodeType::NODE_TYPE_ROOT);
  mds_registration_pack->addStringValue(
      NodeDefinitionKey::NODE_SUB_TYPE,
      chaos::NodeType::NODE_SUBTYPE_SCRIPTABLE_EXECUTION_UNIT);
  mds_registration_pack->addStringValue(chaos::ControlUnitNodeDefinitionKey::CONTROL_UNIT_VIEW, implementation);
  std::map<int, ChaosSharedPtr<chaos::common::data::CDataWrapper>>::iterator i;
  CDataWrapper                                                               ds;
  ds.addInt64Value(ControlUnitNodeDefinitionKey::CONTROL_UNIT_DATASET_TIMESTAMP,
                   chaos::common::utility::TimingUtil::getTimeStamp());

  for (i = datasets.begin(); i != datasets.end(); i++) {
    if (i->first == chaos::DataPackCommonKey::DPCK_DATASET_TYPE_INPUT ||
        i->first == chaos::DataPackCommonKey::DPCK_DATASET_TYPE_OUTPUT)
      wrapper2dataset(ds, *((i->second).get()), i->first);
  }

  ds.finalizeArrayForKey(
      chaos::ControlUnitNodeDefinitionKey::CONTROL_UNIT_DATASET_DESCRIPTION);

  mds_registration_pack->addCSDataValue(
      chaos::ControlUnitNodeDefinitionKey::CONTROL_UNIT_DATASET_DESCRIPTION,
      ds);
  CDWUniquePtr tmp(new CDataWrapper());
  this->getActionDescrionsInDataWrapper(*tmp, true);
  mds_registration_pack->appendCDataWrapperToArray(*tmp);
  mds_registration_pack->finalizeArrayForKey(
      RpcActionDefinitionKey::CS_CMDM_ACTION_DESC);
  PropertyCollector::fillDescription("property", *mds_registration_pack.get());

  DPD_LDBG << " REGISTRATION PACK: " << mds_registration_pack->getJSONString();

  if ((ret = mds_message_channel->sendNodeRegistration(
           MOVE(mds_registration_pack), true, 10000)) == 0) {
    CDWUniquePtr mdsPack(new CDataWrapper());
    mdsPack->addStringValue(chaos::NodeDefinitionKey::NODE_UNIQUE_ID, uid);
    mdsPack->addStringValue(chaos::NodeDefinitionKey::NODE_TYPE,
                            chaos::NodeType::NODE_TYPE_ROOT);

    ret =
        mds_message_channel->sendNodeLoadCompletion(MOVE(mdsPack), true, 10000);

    chaos::common::async_central::AsyncCentralManager::getInstance()->addTimer(
        this, 0, chaos::common::constants::CUTimersTimeoutinMSec);
  } else {
    DPD_LERR << " cannot register dataset " << i->first;

    return -1;
  }

  try {
    if (HealtManager::getInstance()->getServiceState() ==
        chaos::CUStateKey::DEINIT) {
      StartableService::initImplementation(HealtManager::getInstance(), NULL, "HealtManager", __PRETTY_FUNCTION__);
    }

    InizializableService::initImplementation(
        chaos::common::metadata_logging::MetadataLoggingManager::getInstance(),
        NULL,
        "MetadataLoggingManager",
        __PRETTY_FUNCTION__);
  } catch (...) {
    DPD_LERR
        << "cannot initialize HealtManager/logmanager, already initialized?";
  }
  CHAOS_NOT_THROW(HealtManager::getInstance()->addNewNode(uid););
  // add push rate metric
  CHAOS_NOT_THROW(HealtManager::getInstance()->addNodeMetric(
      uid,
      chaos::ControlUnitHealtDefinitionValue::CU_HEALT_OUTPUT_DATASET_PUSH_RATE,
      chaos::DataType::TYPE_DOUBLE););
  CHAOS_NOT_THROW(HealtManager::getInstance()->addNodeMetric(
      uid,
      chaos::ControlUnitHealtDefinitionValue::CU_HEALT_OUTPUT_ALARM_LEVEL,
      chaos::DataType::TYPE_INT32););
  CHAOS_NOT_THROW(HealtManager::getInstance()->addNodeMetric(
      uid,
      chaos::ControlUnitHealtDefinitionValue::CU_HEALT_OUTPUT_DATASET_PUSH_SIZE,
      chaos::DataType::TYPE_INT32););
  CHAOS_NOT_THROW(HealtManager::getInstance()->addNodeMetric(
      uid,
      chaos::ControlUnitHealtDefinitionValue::CU_HEALT_OUTPUT_DATASET_PUSH_ERROR,
      chaos::DataType::TYPE_INT32););
  CHAOS_NOT_THROW(HealtManager::getInstance()->addNodeMetric(
      uid,
      chaos::ControlUnitHealtDefinitionValue::CU_HEALT_OUTPUT_DATASET_PUSH_LOST,
      chaos::DataType::TYPE_INT32););

  CHAOS_NOT_THROW(HealtManager::getInstance()->addNodeMetric(
      uid,
      chaos::ControlUnitHealtDefinitionValue::CU_HEALT_OUTPUT_TOT_PUSH_KSIZE,
      chaos::DataType::TYPE_INT32););

  CHAOS_NOT_THROW(HealtManager::getInstance()->addNodeMetricValue(
      uid, chaos::NodeHealtDefinitionKey::NODE_HEALT_STATUS, chaos::NodeHealtDefinitionValue::NODE_HEALT_STATUS_START, true);)
  CHAOS_NOT_THROW(
      StartableService::startImplementation(
          HealtManager::getInstance(), "HealtManager", __PRETTY_FUNCTION__););

  pushDataset(chaos::DataPackCommonKey::DPCK_DATASET_TYPE_SYSTEM);

  {
    EXECUTE_CHAOS_API(api_proxy::control_unit::InitDeinit, timeo, uid, true);
    if (apires->getError()) {
      return apires->getError();
    }
  }
  DPD_LAPP << "Waiting init ";

  waitEU.wait();

  {
    EXECUTE_CHAOS_API(api_proxy::control_unit::StartStop, timeo, uid, true);
    if (apires->getError()) {
      return apires->getError();
    }
  }
  DPD_LAPP << "Waiting start ";

  waitEU.wait();

  return 0;
}

// RPC HANDLERS
chaos::common::data::CDWUniquePtr ChaosDatasetIO::_load(
    chaos::common::data::CDWUniquePtr dataset_attribute_values) {
  CDWUniquePtr result;

  HealtManager::getInstance()->addNodeMetricValue(
      uid, chaos::NodeHealtDefinitionKey::NODE_HEALT_STATUS, chaos::NodeHealtDefinitionValue::NODE_HEALT_STATUS_LOAD, true);

  DPD_LDBG << "LOAD: " << dataset_attribute_values->getJSONString();
  if (dataset_attribute_values->hasKey("cudk_load_param")) {
    std::string str = dataset_attribute_values->getStringValue("cudk_load_param");
    try {
      chaos::common::data::CDataWrapper wp;

      wp.setSerializedJsonData(str.c_str());
      result = wp.clone();
      return execute(ACT_LOAD, result);

    } catch (...) {
    }
  }
  return execute(ACT_LOAD, dataset_attribute_values);
}
chaos::common::data::CDWUniquePtr ChaosDatasetIO::_unload(
    chaos::common::data::CDWUniquePtr dataset_attribute_values) {
  CDWUniquePtr result;
  HealtManager::getInstance()->addNodeMetricValue(
      uid, chaos::NodeHealtDefinitionKey::NODE_HEALT_STATUS, chaos::NodeHealtDefinitionValue::NODE_HEALT_STATUS_UNLOAD, true);

  DPD_LDBG << "UNLOAD: " << dataset_attribute_values->getJSONString();
  waitEU.notifyAll();
  return execute(ACT_UNLOAD, dataset_attribute_values);
  ;
}

CDWUniquePtr
ChaosDatasetIO::_setDatasetAttribute(CDWUniquePtr dataset_attribute_values) {
  CDWUniquePtr   result;
  ChaosStringSet keys;
  DPD_LDBG << "INPUT: " << dataset_attribute_values->getJSONString();
  bool changed = false;
  datasets[chaos::DataPackCommonKey::DPCK_DATASET_TYPE_INPUT]->getAllKey(keys);
  for (ChaosStringSet::iterator i = keys.begin(); i != keys.end(); i++) {
    if (*i == chaos::DataPackCommonKey::DPCK_DEVICE_ID)
      continue;
    if (dataset_attribute_values->hasKey(*i)) {
      DPD_LDBG << "Setting " << *i << " = "
               << dataset_attribute_values->getStringValue(*i);
      /* if(attr_desc.find(*i)!=attr_desc.end()){
         // check data. boundary TODO
       }*/
      changed = true;
      datasets[chaos::DataPackCommonKey::DPCK_DATASET_TYPE_INPUT]->setAsString(
          *i, dataset_attribute_values->getStringValue(*i));
    }
  }
  if (changed) {
    pushDataset(chaos::DataPackCommonKey::DPCK_DATASET_TYPE_INPUT);
  }
  return execute(ACT_SET, dataset_attribute_values);
}

chaos::common::data::CDWUniquePtr ChaosDatasetIO::_registrationAck(
    chaos::common::data::CDWUniquePtr dataset_attribute_values) {
  CDWUniquePtr result;

  DPD_LDBG << "REGISTRATION ACK: " << dataset_attribute_values->getJSONString();

  return result;
}
chaos::common::data::CDWUniquePtr ChaosDatasetIO::_start(
    chaos::common::data::CDWUniquePtr dataset_attribute_values) {
  CDWUniquePtr result;
  state = chaos::CUStateKey::START;
  DPD_LDBG << "Start";

  HealtManager::getInstance()->addNodeMetricValue(
      uid, chaos::NodeHealtDefinitionKey::NODE_HEALT_STATUS, chaos::NodeHealtDefinitionValue::NODE_HEALT_STATUS_START, true);
  waitEU.notifyAll();
  return execute(ACT_START, dataset_attribute_values);
}
chaos::common::data::CDWUniquePtr ChaosDatasetIO::_stop(
    chaos::common::data::CDWUniquePtr dataset_attribute_values) {
  CDWUniquePtr result;
  state = chaos::CUStateKey::STOP;
  DPD_LDBG << "Stop";

  HealtManager::getInstance()->addNodeMetricValue(
      uid, chaos::NodeHealtDefinitionKey::NODE_HEALT_STATUS, chaos::NodeHealtDefinitionValue::NODE_HEALT_STATUS_STOP, true);
  HealtManager::getInstance()->addNodeMetricValue(uid,
                                                  ControlUnitHealtDefinitionValue::CU_HEALT_OUTPUT_DATASET_PUSH_RATE,
                                                  0.0,
                                                  true);
  HealtManager::getInstance()->addNodeMetricValue(uid,
                                                  ControlUnitHealtDefinitionValue::CU_HEALT_OUTPUT_DATASET_PUSH_SIZE,
                                                  0,
                                                  true);
  return execute(ACT_STOP, dataset_attribute_values);
}
chaos::common::data::CDWUniquePtr ChaosDatasetIO::_deinit(
    chaos::common::data::CDWUniquePtr dataset_attribute_values) {
  CDWUniquePtr result;
  state = chaos::CUStateKey::DEINIT;
  DPD_LDBG << "Deinit";

  HealtManager::getInstance()->addNodeMetricValue(
      uid, chaos::NodeHealtDefinitionKey::NODE_HEALT_STATUS, chaos::NodeHealtDefinitionValue::NODE_HEALT_STATUS_DEINIT, true);
  HealtManager::getInstance()->addNodeMetricValue(uid,
                                                  ControlUnitHealtDefinitionValue::CU_HEALT_OUTPUT_DATASET_PUSH_RATE,
                                                  0.0,
                                                  true);
  HealtManager::getInstance()->addNodeMetricValue(uid,
                                                  ControlUnitHealtDefinitionValue::CU_HEALT_OUTPUT_DATASET_PUSH_SIZE,
                                                  0,
                                                  true);
  return execute(ACT_DEINIT, dataset_attribute_values);
}
chaos::common::data::CDWUniquePtr ChaosDatasetIO::_getInfo(
    chaos::common::data::CDWUniquePtr dataset_attribute_values) {
  CDWUniquePtr result;

  return result;
}

chaos::common::data::CDWUniquePtr
ChaosDatasetIO::_submitStorageBurst(chaos::common::data::CDWUniquePtr data) {
  CDWUniquePtr                                    result;
  common::data::structured::DatasetBurstSDWrapper db_sdw;
  db_sdw.deserialize(data.get());
  burst_cycles = burst_time_ts = 0;

  common::data::structured::DatasetBurstShrdPtr burst =
      ChaosMakeSharedPtr<common::data::structured::DatasetBurst>(db_sdw());

  if (burst->type ==
      chaos::ControlUnitNodeDefinitionType::DSStorageBurstTypeUndefined) {
    DPD_LERR << CHAOS_FORMAT("The type is mandatory for burst %1%",
                             % data->getJSONString());
    return CDWUniquePtr();
  }
  if (!burst->value.isValid()) {
    DPD_LERR << CHAOS_FORMAT("The value is mandatory for burst %1%",
                             % data->getJSONString());
    return CDWUniquePtr();
  }

  if (burst->type ==
      chaos::ControlUnitNodeDefinitionType::DSStorageBurstTypeMSec) {
    burst_time_ts = TimingUtil::getTimeStamp() + burst->value.asInt32();
  }
  if (burst->type ==
      chaos::ControlUnitNodeDefinitionType::DSStorageBurstTypeNPush) {
    burst_cycles = burst->value.asUInt32();
  }
  DPD_LDBG << "Enabling burst:" << data->getCompliantJSONString();
  datasets[chaos::DataPackCommonKey::DPCK_DATASET_TYPE_SYSTEM]->setValue(
      ControlUnitDatapackSystemKey::BURST_STATE, true);

  datasets[chaos::DataPackCommonKey::DPCK_DATASET_TYPE_SYSTEM]->setValue(
      ControlUnitDatapackSystemKey::BURST_TAG, burst->tag);

  updateSystem();
  return execute(ACT_BURST, data);
}

CDWUniquePtr ChaosDatasetIO::_init(CDWUniquePtr dataset_attribute_values) {
  CDWUniquePtr result;
  burst_cycles = burst_time_ts = 0;
  push_errors                  = 0;
  packet_lost                  = 0;
  packet_tot_size              = 0;
  DPD_LDBG << "INIT INPUT: " << dataset_attribute_values->getJSONString();
  if (!dataset_attribute_values->hasKey(
          ControlUnitNodeDefinitionKey::CONTROL_UNIT_DATASET_DESCRIPTION)) {
    DPD_LERR << "NO DATASET PRESENT INPUT: ";
    result = execute(ACT_INIT, dataset_attribute_values);
    waitEU.notifyAll();

    return result;
  }
  CDWUniquePtr desc = dataset_attribute_values->getCSDataValue(
      ControlUnitNodeDefinitionKey::CONTROL_UNIT_DATASET_DESCRIPTION);
  if ((datasets[chaos::DataPackCommonKey::DPCK_DATASET_TYPE_INPUT].get()) && desc->isVector(
                                                                                 ControlUnitNodeDefinitionKey::CONTROL_UNIT_DATASET_DESCRIPTION)) {
    CMultiTypeDataArrayWrapperSPtr elementsDescriptions = desc->getVectorValue(
        ControlUnitNodeDefinitionKey::CONTROL_UNIT_DATASET_DESCRIPTION);

    for (int idx = 0; idx < elementsDescriptions->size(); idx++) {
      CDWUniquePtr elementDescription =
          elementsDescriptions->getCDataWrapperElementAtIndex(idx);
      DPD_LDBG << "LOOKING " << elementDescription->getJSONString();
      // attribute name
      if (!elementDescription->hasKey(
              ControlUnitNodeDefinitionKey::
                  CONTROL_UNIT_DATASET_ATTRIBUTE_NAME) ||
          !elementDescription->hasKey(ControlUnitNodeDefinitionKey::
                                          CONTROL_UNIT_DATASET_DEFAULT_VALUE)) {
        continue;
      }

      string attrName = elementDescription->getStringValue(
          ControlUnitNodeDefinitionKey::CONTROL_UNIT_DATASET_ATTRIBUTE_NAME);
      string attrValue = elementDescription->getStringValue(
          ControlUnitNodeDefinitionKey::CONTROL_UNIT_DATASET_DEFAULT_VALUE);
      DPD_LDBG << "CHECKING " << attrName << " is in the dataset:"
               << datasets[chaos::DataPackCommonKey::DPCK_DATASET_TYPE_INPUT]
                      ->hasKey(attrName);

      if (datasets[chaos::DataPackCommonKey::DPCK_DATASET_TYPE_INPUT]->hasKey(
              attrName)) {
        /*  TODO pay attention this is not supported in C98
        attr_desc[attrName] = MOVE(elementDescription);
        */
        int ret = datasets[chaos::DataPackCommonKey::DPCK_DATASET_TYPE_INPUT]
                      ->setAsString(attrName, attrValue);
        DPD_LDBG << "SETTING " << attrName << "=" << attrValue << " ret:" << ret;  //<<" reread:"<<datasets[chaos::DataPackCommonKey::DPCK_DATASET_TYPE_INPUT]->getStringValue(attrName);
        execute(ACT_SET, elementDescription);

      }
    }
  }
  state = chaos::CUStateKey::INIT;
  HealtManager::getInstance()->addNodeMetricValue(
      uid, chaos::NodeHealtDefinitionKey::NODE_HEALT_STATUS, chaos::NodeHealtDefinitionValue::NODE_HEALT_STATUS_INIT, true);

  updateConfiguration(dataset_attribute_values);
  setCUAlarmLevel("packet_send_error", 0);
  setCUAlarmLevel("packet_lost", 0);
  if (datasets[chaos::DataPackCommonKey::DPCK_DATASET_TYPE_INPUT].get()) {
    pushDataset(chaos::DataPackCommonKey::DPCK_DATASET_TYPE_INPUT);
  }
  result = execute(ACT_INIT, dataset_attribute_values);

  waitEU.notifyAll();

  return result;
}
CDWUniquePtr ChaosDatasetIO::_updateConfiguration(CDWUniquePtr update_pack) {
  return updateConfiguration(update_pack);
}
CDWUniquePtr ChaosDatasetIO::updateConfiguration(CDWUniquePtr &update_pack) {
  // check to see if the device can ben initialized

  // PropertyGroupVectorSDWrapper pg_sdw;
  // pg_sdw.serialization_key = "property";
  // pg_sdw.deserialize(update_pack.get());
  PropertyGroupVectorSDWrapper pg_sdw;
  pg_sdw.serialization_key = "property";
  pg_sdw.deserialize(update_pack.get());
  DPD_LDBG << "properties " << pg_sdw.serialize()->getJSONString();

  PropertyCollector::applyValue(pg_sdw());

  // update the property
  // PropertyCollector::applyValue(pg_sdw());

  return execute(ACT_UPDATE, update_pack);
}
//////

uint64_t ChaosDatasetIO::queryHistoryDatasets(uint64_t ms_start,
                                              uint64_t ms_end,
                                              uint32_t page,
                                              int      type) {
  return queryHistoryDatasets(uid, ms_start, ms_end, page, type);
}

uint64_t ChaosDatasetIO::queryHistoryDatasets(const std::string &dsname,
                                              uint64_t           ms_start,
                                              uint64_t           ms_end,
                                              uint32_t           page,
                                              int                type) {
  std::string dst = dsname + chaos::datasetTypeToPostfix(type);

  DPD_LDBG << "Query To:" << dst << " start:" << ms_start
           << " end_ms:" << ms_end << " page:" << page;

  chaos::common::io::QueryCursor *pnt =
      ioLiveDataDriver->performQuery(dst, ms_start, ms_end, page);
  if (pnt == NULL) {
    DPD_LERR << "NO CURSOR";

    return 0;
  }
  query_index++;
  qc_t q;
  q.page_len                    = page;
  q.qc                          = pnt;
  q.qt                          = query_index;
  query_cursor_map[query_index] = q;
  return query_index;
}
/**
             Perform a full query on the current device specifing a starting
   runid and a sequid and a tag.
             */
uint64_t ChaosDatasetIO::queryHistoryDatasets(uint64_t              ms_start,
                                              uint64_t              ms_end,
                                              const uint64_t        runid,
                                              const uint64_t        sequid,
                                              const ChaosStringSet &meta_tags,
                                              uint32_t              page,
                                              int                   type) {
  return queryHistoryDatasets(uid, ms_start, ms_end, runid, sequid, meta_tags, page, type);
}

uint64_t ChaosDatasetIO::queryHistoryDatasets(
    const std::string &   dsname,
    uint64_t              ms_start,
    uint64_t              ms_end,
    const uint64_t        runid,
    const uint64_t        sequid,
    const ChaosStringSet &meta_tags,
    uint32_t              page,
    int                   type) {
  std::string dst = dsname + chaos::datasetTypeToPostfix(type);
  DPD_LDBG << "Query To:" << dst << " start:" << ms_start
           << " end_ms:" << ms_end << "runid:" << runid << " sequid:" << sequid
           << "ntags:" << meta_tags.size() << " page:" << page;

  chaos::common::io::QueryCursor *pnt = ioLiveDataDriver->performQuery(
      dst, ms_start, ms_end, sequid, runid, meta_tags, ChaosStringSet(), page);
  if (pnt == NULL) {
    DPD_LERR << "NO CURSOR";

    return 0;
  }
  query_index++;
  qc_t q;
  q.page_len                    = page;
  q.qc                          = pnt;
  q.qt                          = query_index;
  query_cursor_map[query_index] = q;
  return query_index;
}
bool ChaosDatasetIO::queryHasNext(uint64_t uid) {
  bool                         ret;
  query_cursor_map_t::iterator i = query_cursor_map.find(uid);
  if (i == query_cursor_map.end()) {
    DPD_LERR << "query ID:" << uid << " not exists";
    return false;
  }
  chaos::common::io::QueryCursor *pnt = (i->second).qc;
  ret                                 = pnt->hasNext();
  DPD_LDBG << "query ID:" << uid << " (0xx" << std::hex << pnt
           << ") has next: " << std::dec << ret;

  return ret;
}
std::vector<ChaosDataSet> ChaosDatasetIO::getNextPage(uint64_t uid) {
  std::vector<ChaosDataSet>    ret;
  query_cursor_map_t::iterator i = query_cursor_map.find(uid);
  if (i == query_cursor_map.end()) {
    DPD_LERR << "query ID:" << uid << " not exists";
    return ret;
  }
  chaos::common::io::QueryCursor *pnt = (i->second).qc;
  uint32_t                        len = (i->second).page_len;
  int                             cnt = 0;
  DPD_LDBG << "query ID:" << uid << " page len " << pnt->getPageLen()
           << " application page:" << len;

  while (pnt->hasNext() && (cnt < len)) {
    ChaosDataSet q_result(pnt->next());

    ret.push_back(q_result);
    cnt++;
  }

  if (pnt->hasNext() == false) {
    DPD_LDBG << "query ID:" << uid << " ENDED freeing resource";
    ioLiveDataDriver->releaseQuery(pnt);
    query_cursor_map.erase(i);
  }
  return ret;
}

std::vector<ChaosDataSet> ChaosDatasetIO::queryHistoryDatasets(
    const std::string &dsname,
    uint64_t           ms_start,
    uint64_t           ms_end,
    int                type) {
  std::vector<ChaosDataSet> ret;
  std::string               dst = dsname + chaos::datasetTypeToPostfix(type);
  DPD_LDBG << "Query To:" << dst << " start:" << ms_start
           << " end_ms:" << ms_end << "default page:" << defaultPage;

  chaos::common::io::QueryCursor *pnt =
      ioLiveDataDriver->performQuery(dst, ms_start, ms_end, defaultPage);
  if (pnt == NULL) {
    DPD_LERR << "NO CURSOR";
    return ret;
  }
  while (pnt->hasNext()) {
    ChaosDataSet q_result(pnt->next());
    //   std::cout<<"Retrieve: "<<q_result->getCompliantJSONString()<<std::endl;
    ret.push_back(q_result);
  }
  ioLiveDataDriver->releaseQuery(pnt);
  return ret;
}
std::vector<ChaosDataSet>
ChaosDatasetIO::queryHistoryDatasets(uint64_t ms_start, uint64_t ms_end, int type) {
  return queryHistoryDatasets(uid, ms_start, ms_end, type);
}
void ChaosDatasetIO::timeout() {
  // update push metric
  updateHealth();
}

int ChaosDatasetIO::allocateCUAlarm(const std::string &name) {
  if (datasets[chaos::DataPackCommonKey::DPCK_DATASET_TYPE_CU_ALARM]->hasKey(
          name)) {
    return -1;
  }
  cu_alarms.push_back(name);

  datasets[chaos::DataPackCommonKey::DPCK_DATASET_TYPE_CU_ALARM]->addInt32Value(
      name, 0);
  return 0;
}

int32_t ChaosDatasetIO::findMax(ChaosDataSet &            ds,
                                std::vector<std::string> &dataset_key) {
  int32_t ret = 0;

  for (std::vector<std::string>::iterator it = dataset_key.begin();
       it != dataset_key.end();
       it++) {
    int32_t val = ds->getInt32Value(*it);
    if (val > ret) {
      ret = val;
    }
  }
  return ret;
}

int ChaosDatasetIO::allocateDEVAlarm(const std::string &name) {
  if (datasets[chaos::DataPackCommonKey::DPCK_DATASET_TYPE_DEV_ALARM]->hasKey(
          name)) {
    return -1;
  }
  dev_alarms.push_back(name);

  datasets[chaos::DataPackCommonKey::DPCK_DATASET_TYPE_DEV_ALARM]
      ->addInt32Value(name, 0);
  return 0;
}
int ChaosDatasetIO::setAlarmLevel(const std::string &name, uint8_t value, const unsigned channel, const std::string msg) {
  if (!(datasets[channel]->hasKey(
          name))) {
    throw chaos::CException(-1, "No Alarm \"" + name + "\" found", __PRETTY_FUNCTION__);

    return -1;
  }
  int oldVal = datasets[channel]->getInt32Value(name);
  int lvl    = 0;
  if (oldVal != value) {
    datasets[channel]->setValue(name, value);
    if (channel == chaos::DataPackCommonKey::DPCK_DATASET_TYPE_CU_ALARM) {
      lvl = cu_alarm_lvl =
          findMax(datasets[chaos::DataPackCommonKey::DPCK_DATASET_TYPE_CU_ALARM],
                  cu_alarms);
      datasets[chaos::DataPackCommonKey::DPCK_DATASET_TYPE_SYSTEM]->setValue(
          chaos::ControlUnitDatapackSystemKey::CU_ALRM_LEVEL, cu_alarm_lvl);
      if (msg.size()) {
        if (value == 0) {
          log("CU Alarm OFF", value + 1, (msg.size()) ? msg : name);

        } else {
          log("CU Alarm", value + 1, (msg.size()) ? msg : name);
        }
      }
    } else if (channel == chaos::DataPackCommonKey::DPCK_DATASET_TYPE_DEV_ALARM) {
      lvl = dev_alarm_lvl =
          findMax(datasets[chaos::DataPackCommonKey::DPCK_DATASET_TYPE_DEV_ALARM],
                  dev_alarms);
      datasets[chaos::DataPackCommonKey::DPCK_DATASET_TYPE_SYSTEM]->setValue(
          chaos::ControlUnitDatapackSystemKey::DEV_ALRM_LEVEL, dev_alarm_lvl);
      if (msg.size()) {
        if (value) {
          log("DEV Alarm", value + 1, msg);

        } else {
          log("DEV Alarm OFF", value + 1, msg);
        }
      }
    }
    pushDataset(chaos::DataPackCommonKey::DPCK_DATASET_TYPE_SYSTEM);
    pushDataset(channel);
  }
  return lvl;
}

int ChaosDatasetIO::setCUAlarmLevel(const std::string &name, uint8_t value, const std::string msg) {
  return setAlarmLevel(name, value, chaos::DataPackCommonKey::DPCK_DATASET_TYPE_CU_ALARM, msg);
}
int ChaosDatasetIO::setDeviceAlarmLevel(const std::string &name, uint8_t value, const std::string msg) {
  return setAlarmLevel(name, value, chaos::DataPackCommonKey::DPCK_DATASET_TYPE_DEV_ALARM, msg);
}

void ChaosDatasetIO::log(const std::string &subject, int log_leve, const std::string &message) {
  if (standard_logging_channel == NULL)
    return;
  chaos::common::metadata_logging::StandardLoggingChannel::LogLevel log_level =
      (chaos::common::metadata_logging::StandardLoggingChannel::LogLevel)
          log_leve;
  standard_logging_channel->logMessage(uid, subject, log_level, message);
  switch (log_level) {
    case StandardLoggingChannel::LogLevelInfo:
      DPD_LAPP << "[" << log_leve << "]" << message;
      break;
    case StandardLoggingChannel::LogLevelDebug:
      DPD_LDBG << "[" << log_leve << "]" << message;
      break;
    case StandardLoggingChannel::LogLevelWarning:
      DPD_LDBG << "[" << log_leve << "]" << message;
      break;
    case StandardLoggingChannel::LogLevelError:
      DPD_LERR << "[" << log_leve << "]" << message;
      break;
    case StandardLoggingChannel::LogLevelFatal:
      DPD_LERR << "[" << log_leve << "]" << message;
      break;
  }
}
bool ChaosDatasetIO::propertyChangeHandler(
    const std::string &                      group_name,
    const std::string &                      property_name,
    const chaos::common::data::CDataVariant &property_value) {
  DPD_LDBG << CHAOS_FORMAT(
      "Update property request for %1%[%2%] with value %3%",
      % property_name % group_name % property_value.asString());
  return true;
}

//! callback ofr updated property value
void ChaosDatasetIO::propertyUpdatedHandler(
    const std::string &                      group_name,
    const std::string &                      property_name,
    const chaos::common::data::CDataVariant &old_value,
    const chaos::common::data::CDataVariant &new_value) {
  if (group_name.compare("property_abstract_control_unit") == 0) {
    // update property on driver
    // reflect modification on dataset
    if (property_name.compare(ControlUnitDatapackSystemKey::BYPASS_STATE) ==
        0) {
    } else if (property_name.compare(
                   DataServiceNodeDefinitionKey::DS_STORAGE_TYPE) == 0) {
      setStorage(new_value.asInt32());
    } else if (property_name.compare(
                   ControlUnitDatapackSystemKey::THREAD_SCHEDULE_DELAY) == 0) {
      setSchedule(new_value.asInt32());
    } else if (property_name.compare(
                   DataServiceNodeDefinitionKey::DS_STORAGE_LIVE_TIME) == 0) {
    } else if (property_name.compare(
                   DataServiceNodeDefinitionKey::DS_STORAGE_HISTORY_TIME) ==
               0) {
      setAgeing(new_value.asInt32());
    }
  }
  pushDataset(chaos::DataPackCommonKey::DPCK_DATASET_TYPE_SYSTEM);
}

void ChaosDatasetIO::_initPropertyGroup() {
  PropertyGroup &pg_abstract_cu =
      addGroup(chaos::ControlUnitPropertyKey::P_GROUP_NAME);
  pg_abstract_cu.addProperty(ControlUnitDatapackSystemKey::BYPASS_STATE,
                             "Put in bypass state",
                             DataType::TYPE_BOOLEAN,
                             0,
                             CDataVariant((bool)false));
  pg_abstract_cu.addProperty(DataServiceNodeDefinitionKey::DS_STORAGE_TYPE,
                             "Set the storage type",
                             DataType::TYPE_INT32,
                             0,
                             CDataVariant((int32_t)0));
  pg_abstract_cu.addProperty(DataServiceNodeDefinitionKey::DS_STORAGE_LIVE_TIME,
                             "Set live time",
                             DataType::TYPE_INT64,
                             0,
                             CDataVariant((int64_t)0));
  pg_abstract_cu.addProperty(
      DataServiceNodeDefinitionKey::DS_STORAGE_HISTORY_TIME, "Set histo time", DataType::TYPE_INT64, 0, CDataVariant((int64_t)0));
  //    CDWUniquePtr burst_type_desc(new CDataWrapper());
  //    burst_type_desc->addInt32Value(DataServiceNodeDefinitionKey::DS_HISTORY_BURST_TYPE,
  //    DataServiceNodeDefinitionType::DSStorageBurstTypeUndefined);
  //    pg_abstract_cu.addProperty(DataServiceNodeDefinitionKey::DS_HISTORY_BURST,
  //    "Specify if the restore operation need to be done as real operation or
  //    not", DataType::TYPE_CLUSTER,0,
  //    CDataVariant(burst_type_desc.release()));

  pg_abstract_cu.addProperty(
      ControlUnitDatapackSystemKey::THREAD_SCHEDULE_DELAY,
      "Set the control unit step repeat time in microseconds",
      DataType::TYPE_INT64,
      0,
      CDataVariant((int64_t)1000000));  // set to one seconds

  PropertyCollector::setPropertyValueChangeFunction(ChaosBind(
      &ChaosDatasetIO::propertyChangeHandler, this, ChaosBindPlaceholder(_1), ChaosBindPlaceholder(_2), ChaosBindPlaceholder(_3)));
  PropertyCollector::setPropertyValueUpdatedFunction(
      ChaosBind(&ChaosDatasetIO::propertyUpdatedHandler, this, ChaosBindPlaceholder(_1), ChaosBindPlaceholder(_2), ChaosBindPlaceholder(_3), ChaosBindPlaceholder(_4)));
}
chaos::common::data::CDWUniquePtr ChaosDatasetIO::getProperty(chaos::common::data::CDWUniquePtr data) {
  chaos::common::data::CDWUniquePtr ret = execute(ACT_GETPROP, data);
  DPD_LDBG << "get properties:" << ((ret.get()) ? ret->getJSONString() : "");

  return ret;
}
chaos::common::data::CDWUniquePtr ChaosDatasetIO::setProperty(chaos::common::data::CDWUniquePtr data) {
  chaos::common::data::CDWUniquePtr ret = execute(ACT_SETPROP, data);
  DPD_LDBG << "set properties:" << ((data.get()) ? data->getJSONString() : "");

  return ret;
}
chaos::common::data::CDWUniquePtr
ChaosDatasetIO::execute(ActionID r, chaos::common::data::CDWUniquePtr &p) {
  handler_t::iterator i = handlermap.find(r);
  DPD_LDBG << "finding Action:" << r;

  if (i != handlermap.end()) {
    actionFunc_t handler = i->second;
    return handler(p, this);
  }
  DPD_LDBG << "No registered Action:" << r;

  return chaos::common::data::CDWUniquePtr();
}

int ChaosDatasetIO::subscribe(const std::string &key) {
  ioLiveDataDriver->subscribe(key);
  return 0;
}

int ChaosDatasetIO::addHandler(const std::string &key, chaos::common::message::msgHandler cb) {
  return ioLiveDataDriver->addHandler(key, cb);
}
int ChaosDatasetIO::addHandler(chaos::common::message::msgHandler cb) {
  return ioLiveDataDriver->addHandler(cb);
}

int ChaosDatasetIO::registerAction(actionFunc_t func, ActionID id) {
  if (id < ACT_LOAD || id >= ACT_NONE)
    return -1;
  DPD_LDBG << "register Action:" << id;

  handlermap[id] = func;
  return 0;
}

void ChaosDatasetIO::deinit() {
  waitEU.notifyAll();

  if (deinitialized) {
    DEBUG_CODE(DPD_LDBG << "Already deinitialized");
    return;
  }
  DPD_LDBG << "deinit";
  NetworkBroker::getInstance()->deregisterAction(this);

  /*{
    EXECUTE_CHAOS_API(api_proxy::control_unit::DeleteInstance, timeo, uid,
                      groupName);
  }*/
  DEBUG_CODE(DPD_LDBG << "Timer removed");
  CHAOS_NOT_THROW(InizializableService::deinitImplementation(
                      MetadataLoggingManager::getInstance(),
                      "MetadataLoggingManager",
                      __PRETTY_FUNCTION__););

  if (alarm_logging_channel) {
    MetadataLoggingManager::getInstance()->releaseChannel(
        alarm_logging_channel);
    alarm_logging_channel = NULL;
  }

  if (standard_logging_channel) {
    MetadataLoggingManager::getInstance()->releaseChannel(
        standard_logging_channel);
    standard_logging_channel = NULL;
  }
  chaos::common::async_central::AsyncCentralManager::getInstance()->removeTimer(
      this);

  sleep(2);
  HealtManager::getInstance()->addNodeMetricValue(
      uid, chaos::NodeHealtDefinitionKey::NODE_HEALT_STATUS, chaos::NodeHealtDefinitionValue::NODE_HEALT_STATUS_UNLOAD, true);

  for (std::map<int,
                ChaosSharedPtr<chaos::common::data::CDataWrapper>>::iterator i =
           datasets.begin();
       i != datasets.end();
       i++) {
    DPD_LDBG << " removing dataset:" << i->first;
    (i->second).reset();
  }

  for (query_cursor_map_t::iterator i = query_cursor_map.begin();
       i != query_cursor_map.end();) {
    DPD_LDBG << " removing query ID:" << i->first;

    ioLiveDataDriver->releaseQuery((i->second).qc);

    query_cursor_map.erase(i++);
  }
  // RIMANE APPESO SU UN LOCK
  if (ioLiveShDataDriver.use_count() == 1) {
    CHAOS_NOT_THROW(InizializableService::deinitImplementation(
                        ioLiveShDataDriver.get(),
                        "SharedManagedDirecIoDataDriver",
                        __PRETTY_FUNCTION__););
  }
  DEBUG_CODE(DPD_LDBG << "Shared Manager deinitialized");

  DEBUG_CODE(DPD_LDBG << "Deinitialized");

  deinitialized = true;
  DEBUG_CODE(DPD_LDBG << "Destroy all resources");

  CHAOS_NOT_THROW(
      StartableService::stopImplementation(
          HealtManager::getInstance(), "HealtManager", __PRETTY_FUNCTION__););
  DEBUG_CODE(DPD_LDBG << "Health stopped");

  CHAOS_NOT_THROW(
      StartableService::deinitImplementation(
          HealtManager::getInstance(), "HealtManager", __PRETTY_FUNCTION__););
  DEBUG_CODE(DPD_LDBG << "Health deinitialized");
}
}  // namespace misc
}  // namespace driver
