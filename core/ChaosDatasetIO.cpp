#include "ChaosDatasetIO.h"
#include <chaos/common/direct_io/DirectIOClient.h>
#include <chaos/common/direct_io/DirectIOClientConnection.h>
#include <chaos/common/healt_system/HealtManager.h>
#include <chaos/common/io/SharedManagedDirecIoDataDriver.h>
#include <chaos/common/metadata_logging/metadata_logging.h>
#include <chaos_metadata_service_client/api_proxy/control_unit/DeleteInstance.h>
#include <chaos_metadata_service_client/api_proxy/control_unit/SetInstanceDescription.h>
#include <chaos_metadata_service_client/api_proxy/unit_server/ManageCUType.h>
#include <chaos_metadata_service_client/api_proxy/unit_server/NewUS.h>

#include <chaos/common/network/NetworkBroker.h>

#define DPD_LOG_HEAD "[ChaosDatasetIO] - "
#define DPD_LAPP LAPP_ << DPD_LOG_HEAD
#define DPD_LDBG LDBG_ << DPD_LOG_HEAD << __PRETTY_FUNCTION__
#define DPD_LERR                                                               \
  LERR_ << DPD_LOG_HEAD << __PRETTY_FUNCTION__ << "(" << __LINE__ << ") "

#include <chaos_metadata_service_client/ChaosMetadataServiceClient.h>

using namespace chaos::common::data;
using namespace chaos::common::utility;
using namespace chaos::common::network;
using namespace chaos::common::direct_io;
using namespace chaos::common::direct_io::channel;
using namespace chaos::common::healt_system;
using namespace chaos::metadata_service_client;
using namespace chaos::common::metadata_logging;
#define EXECUTE_CHAOS_API(api_name, time_out, ...)                             \
  DPD_LDBG << " "                                                              \
           << " Executing Api:\"" << #api_name << "\"";                        \
  chaos::metadata_service_client::api_proxy::ApiProxyResult apires =           \
      GET_CHAOS_API_PTR(api_name)->execute(__VA_ARGS__);                       \
  apires->setTimeout(time_out);                                                \
  apires->wait();                                                              \
  if (apires->getError()) {                                                    \
    std::stringstream ss;                                                      \
    ss << " error in :" << __FUNCTION__ << "|" << __LINE__ << "|" << #api_name \
       << " " << apires->getErrorMessage();                                    \
    DPD_LERR << ss.str();                                                      \
  }
namespace driver {
namespace misc {
ChaosSharedMutex ChaosDatasetIO::iomutex;

ChaosDatasetIO::ChaosDatasetIO(const std::string &name,
                               const std::string &group_name)
    : datasetName(name), groupName(group_name), ageing(3600),
      storageType(
          (int)chaos::DataServiceNodeDefinitionType::DSStorageTypeLiveHistory),
      timeo(5000), entry_created(false), query_index(0), defaultPage(30),
      last_seq(0), packet_size(0), cu_alarm_lvl(0), dev_alarm_lvl(0),
      alarm_logging_channel(NULL), standard_logging_channel(NULL),
      last_push_rate_grap_ts(0), deinitialized(false),
      implementation("datasetIO") {
    ChaosWriteLock l(iomutex);

  try {

    InizializableService::initImplementation(
        chaos::common::io::SharedManagedDirecIoDataDriver::getInstance(), NULL,
        "SharedManagedDirecIoDataDriver", __PRETTY_FUNCTION__);
    /*ioLiveShDataDriver =
        chaos::common::io::SharedManagedDirecIoDataDriver::getInstance()
            ->getSharedDriver();*/
  } catch (...) {
  }

  // pool
  ioLiveDataDriver =
      chaos::metadata_service_client::ChaosMetadataServiceClient::getInstance()
          ->getDataProxyChannelNewInstance();
  network_broker = NetworkBroker::getInstance();

  mds_message_channel = network_broker->getMetadataserverMessageChannel();

  //        ioLiveDataDriver->init(NULL);
  CDWUniquePtr tmp_data_handler;
  if (!mds_message_channel->getDataDriverBestConfiguration(tmp_data_handler,
                                                           5000)) {
    ioLiveDataDriver->updateConfiguration(tmp_data_handler.get());
  }
  try {
    StartableService::initImplementation(HealtManager::getInstance(), NULL,
                                         "HealtManager", __PRETTY_FUNCTION__);
    InizializableService::initImplementation(
        chaos::common::metadata_logging::MetadataLoggingManager::getInstance(),
        NULL, "MetadataLoggingManager", __PRETTY_FUNCTION__);
  } catch (...) {
  }
  runid = time(NULL);
  for (int cnt = 0; cnt < sizeof(pkids) / sizeof(uint64_t); cnt++) {
    pkids[cnt] = 0;
  }

  if (!network_broker) {
    throw chaos::CException(-1, "No network broker found for:" + name,
                            __PRETTY_FUNCTION__);
  }
  if (!mds_message_channel) {
    throw chaos::CException(-1, "No mds channel found for:" + name,
                            __PRETTY_FUNCTION__);
  }
  if (groupName == "") {
    uid = datasetName;
  } else {
    uid = groupName + "/" + datasetName;
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

  allocateDataset(chaos::DataPackCommonKey::DPCK_DATASET_TYPE_DEV_ALARM);
  allocateDataset(chaos::DataPackCommonKey::DPCK_DATASET_TYPE_CU_ALARM);
}

int ChaosDatasetIO::setAgeing(uint64_t secs) {
  ageing = secs;
  return 0;
}
int ChaosDatasetIO::setStorage(int st) {
  storageType = st;
  return 0;
}
int ChaosDatasetIO::setTimeo(uint64_t t) {
  timeo = t;
  return 0;
}

ChaosDatasetIO::~ChaosDatasetIO() { deinit(); }

void ChaosDatasetIO::updateHealth() {
  uint64_t rate_acq_ts = TimingUtil::getTimeStamp();
  double time_offset = (double(rate_acq_ts - last_push_rate_grap_ts)) / 1000.0;
  uint32_t npushes =
      (pkids[chaos::DataPackCommonKey::DPCK_DATASET_TYPE_OUTPUT] -
       last_seq); // time in seconds
  double output_ds_rate =
      (time_offset > 0) ? npushes / time_offset : 0; // rate in seconds
  int32_t output_ds_size =
      (npushes > 0) ? packet_size / npushes : 0; // rate in seconds

  HealtManager::getInstance()->addNodeMetricValue(
      uid,
      chaos::ControlUnitHealtDefinitionValue::CU_HEALT_OUTPUT_DATASET_PUSH_RATE,
      output_ds_rate, true);
  HealtManager::getInstance()->addNodeMetricValue(
      uid,
      chaos::ControlUnitHealtDefinitionValue::CU_HEALT_OUTPUT_DATASET_PUSH_SIZE,
      output_ds_size, true);
  // keep track of acquire timestamp
  last_push_rate_grap_ts = rate_acq_ts;
  packet_size = 0;
  // reset pushe count
  last_seq = pkids[chaos::DataPackCommonKey::DPCK_DATASET_TYPE_OUTPUT];
}
ChaosDataSet ChaosDatasetIO::allocateDataset(int type) {
  std::map<int, ChaosDataSet>::iterator i = datasets.find(type);
  if (i == datasets.end()) {
    ChaosDataSet tmp(new chaos::common::data::CDataWrapper);
    datasets[type] = tmp;
    DPD_LDBG << " allocated dataset:" << type;

    return tmp;
  }
  DPD_LDBG << " returning already allocated dataset:" << type;

  return i->second;
}
int ChaosDatasetIO::pushDataset(ChaosDataSet &new_dataset, int type) {
  int err = 0;

  uint64_t ts = chaos::common::utility::TimingUtil::getTimeStamp();
  uint64_t tsh =
      chaos::common::utility::TimingUtil::getTimeStampInMicroseconds();

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
  packet_size += new_dataset->getBSONRawSize();
  // ChaosUniquePtr<SerializationBuffer>
  // serialization(new_dataset->getBSONData());
  //    DPD_LDBG <<" PUSHING:"<<new_dataset->getJSONString();
  // DirectIOChannelsInfo    *next_client =
  // static_cast<DirectIOChannelsInfo*>(connection_feeder.getService());
  // serialization->disposeOnDelete = !next_client;
  ioLiveDataDriver->storeData(
      uid + chaos::datasetTypeToPostfix(type), new_dataset,
      (chaos::DataServiceNodeDefinitionType::DSStorageType)storageType);

  return err;
}

// push a dataset
int ChaosDatasetIO::pushDataset(int type) {
  // ad producer key
  return pushDataset(datasets[type], type);
}
ChaosDataSet ChaosDatasetIO::getDataset(int type) { return datasets[type]; }

ChaosDataSet ChaosDatasetIO::getLiveDataset(const std::string &dsname,
                                            int type) {
  size_t dim;
  ChaosDataSet tmp;
  char *ptr = ioLiveDataDriver->retriveRawData(
      dsname + chaos::datasetTypeToPostfix(type), &dim);
  if (ptr) {
    tmp.reset(new chaos::common::data::CDataWrapper(ptr));
    free(ptr);
  }

  return tmp;
}

ChaosDataSet ChaosDatasetIO::getLiveDataset(int type) {
  size_t dim;
  ChaosDataSet tmp;
  char *ptr = ioLiveDataDriver->retriveRawData(
      uid + chaos::datasetTypeToPostfix(type), &dim);
  if (ptr) {
    tmp.reset(new chaos::common::data::CDataWrapper(ptr));
    free(ptr);
  }

  return tmp;
}

chaos::common::data::CDWUniquePtr
ChaosDatasetIO::wrapper2dataset(chaos::common::data::CDataWrapper &dataset_pack,
                                int dir) {
  chaos::common::data::CDWUniquePtr mds_registration_pack(new CDataWrapper);
  CDataWrapper cu_dataset;
  ChaosStringVector all_template_key;
  dataset_pack.getAllKey(all_template_key);

  for (ChaosStringVectorIterator it = all_template_key.begin();
       it != all_template_key.end(); it++) {
    if (dataset_pack.isNullValue(*it)) {
      DPD_LERR << "Removed from template null value key:" << *it;
      continue;
    }
    CDataWrapper ds;
    int32_t dstype = 0, subtype = 0;
    int32_t size = 0;
    ds.addStringValue(chaos::ControlUnitNodeDefinitionKey::
                          CONTROL_UNIT_DATASET_ATTRIBUTE_NAME,
                      *it);

    size = dataset_pack.getValueSize(*it);
    if (dataset_pack.isDoubleValue(*it)) {
      dstype |= chaos::DataType::TYPE_DOUBLE;
      subtype = chaos::DataType::SUB_TYPE_DOUBLE;
    } else if (dataset_pack.isInt64Value(*it)) {
      dstype |= chaos::DataType::TYPE_INT64;
      subtype = chaos::DataType::SUB_TYPE_INT64;
    } else if (dataset_pack.isInt32Value(*it)) {
      dstype |= chaos::DataType::TYPE_INT32;
      subtype = chaos::DataType::SUB_TYPE_INT32;
    } else if (dataset_pack.isStringValue(*it)) {
      dstype |= chaos::DataType::TYPE_STRING;
      subtype = chaos::DataType::SUB_TYPE_STRING;
    } else if (dataset_pack.isBinaryValue(*it)) {
      dstype |= chaos::DataType::TYPE_BYTEARRAY;
    } else {
      dstype |= chaos::DataType::TYPE_DOUBLE;
      subtype = chaos::DataType::SUB_TYPE_DOUBLE;
    }
    if (dataset_pack.isVector(*it)) {
      // dstype = chaos::DataType::TYPE_ACCESS_ARRAY;
      dstype = chaos::DataType::TYPE_BYTEARRAY;
    }
    ds.addInt32Value(chaos::ControlUnitNodeDefinitionKey::
                         CONTROL_UNIT_DATASET_BINARY_SUBTYPE,
                     subtype);
    ds.addInt32Value(chaos::ControlUnitNodeDefinitionKey::
                         CONTROL_UNIT_DATASET_ATTRIBUTE_TYPE,
                     dstype);
    ds.addInt32Value(chaos::ControlUnitNodeDefinitionKey::
                         CONTROL_UNIT_DATASET_ATTRIBUTE_DIRECTION,
                     dir);
    ds.addInt32Value(chaos::ControlUnitNodeDefinitionKey::
                         CONTROL_UNIT_DATASET_VALUE_MAX_SIZE,
                     size);

    DPD_LDBG << "- ATTRIBUTE \"" << *it << "\" SIZE:" << size
             << " TYPE:" << dstype << " SUBTYPE:" << subtype;
    cu_dataset.appendCDataWrapperToArray(ds);
  }
  cu_dataset.addInt64Value(
      chaos::ControlUnitNodeDefinitionKey::CONTROL_UNIT_DATASET_TIMESTAMP,
      chaos::common::utility::TimingUtil::getTimeStamp());

  // close array for all device description
  cu_dataset.finalizeArrayForKey(
      chaos::ControlUnitNodeDefinitionKey::CONTROL_UNIT_DATASET_DESCRIPTION);
  cu_dataset.addInt64Value(chaos::DataPackCommonKey::DPCK_SEQ_ID, (int64_t)0);

  mds_registration_pack->addCSDataValue(
      chaos::ControlUnitNodeDefinitionKey::CONTROL_UNIT_DATASET_DESCRIPTION,
      cu_dataset);
  mds_registration_pack->addStringValue(
      chaos::NodeDefinitionKey::NODE_TYPE,
      chaos::NodeType::NODE_TYPE_CONTROL_UNIT);
  return mds_registration_pack;
}
void ChaosDatasetIO::createMDSEntry() {
  api_proxy::control_unit::SetInstanceDescriptionHelper cud;

  { EXECUTE_CHAOS_API(api_proxy::unit_server::NewUS, timeo, groupName); }

  {
    EXECUTE_CHAOS_API(api_proxy::unit_server::ManageCUType, timeo, groupName,
                      "datasetIO", 0);
  }
  if (groupName == "") {
    uid = datasetName;
  } else {
    uid = groupName + "/" + datasetName;
  }

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
  {
    EXECUTE_CHAOS_API(api_proxy::control_unit::SetInstanceDescription, timeo,
                      cud);
  }
  CHAOS_NOT_THROW(HealtManager::getInstance()->addNewNode(uid););
  // add push rate metric
  CHAOS_NOT_THROW(HealtManager::getInstance()->addNodeMetric(
      uid,
      chaos::ControlUnitHealtDefinitionValue::CU_HEALT_OUTPUT_DATASET_PUSH_RATE,
      chaos::DataType::TYPE_DOUBLE););
  CHAOS_NOT_THROW(HealtManager::getInstance()->addNodeMetric(
      uid,
      chaos::ControlUnitHealtDefinitionValue::CU_HEALT_OUTPUT_DATASET_PUSH_SIZE,
      chaos::DataType::TYPE_INT32););

  CHAOS_NOT_THROW(HealtManager::getInstance()->addNodeMetricValue(
      uid, chaos::NodeHealtDefinitionKey::NODE_HEALT_STATUS,
      chaos::NodeHealtDefinitionValue::NODE_HEALT_STATUS_START, true);)
  CHAOS_NOT_THROW(
      StartableService::startImplementation(
          HealtManager::getInstance(), "HealtManager", __PRETTY_FUNCTION__););
}

//! register the dataset of ap roducer
int ChaosDatasetIO::registerDataset() {
  CHAOS_ASSERT(mds_message_channel);
  if (datasets.empty()) {
    DPD_LERR << " NO DATASET ALLOCATED";

    return -3;
  }
  if (entry_created == false) {
    createMDSEntry();
    entry_created = true;
  }
  std::map<int, ChaosSharedPtr<chaos::common::data::CDataWrapper>>::iterator i;
    for (i = datasets.begin(); i != datasets.end(); i++) {
    if ((i->second).get()) {
      CDWUniquePtr mds_registration_pack =
          wrapper2dataset(*((i->second).get()), i->first);
      if (mds_registration_pack.get()) {
        DEBUG_CODE(DPD_LDBG << mds_registration_pack->getJSONString());

        int ret;

        mds_registration_pack->addStringValue(
            chaos::NodeDefinitionKey::NODE_UNIQUE_ID, uid);
        mds_registration_pack->addStringValue(
            chaos::NodeDefinitionKey::NODE_RPC_DOMAIN,
            chaos::common::utility::UUIDUtil::generateUUIDLite());
        mds_registration_pack->addStringValue(
            chaos::NodeDefinitionKey::NODE_RPC_ADDR,
            network_broker->getRPCUrl());
        mds_registration_pack->addStringValue("mds_control_key", "none");
        mds_registration_pack->addStringValue(
            chaos::NodeDefinitionKey::NODE_TYPE,
            chaos::NodeType::NODE_TYPE_CONTROL_UNIT);
        DPD_LDBG << "registering " << i->first << " registration pack:"
                 << mds_registration_pack->getCompliantJSONString();

        if ((ret = mds_message_channel->sendNodeRegistration(
                 MOVE(mds_registration_pack), true, 10000)) == 0) {
          CDWUniquePtr mdsPack(new CDataWrapper());
          mdsPack->addStringValue(chaos::NodeDefinitionKey::NODE_UNIQUE_ID,
                                  uid);
          mdsPack->addStringValue(chaos::NodeDefinitionKey::NODE_TYPE,
                                  chaos::NodeType::NODE_TYPE_CONTROL_UNIT);
          ret = mds_message_channel->sendNodeLoadCompletion(MOVE(mdsPack), true,
                                                            10000);

          chaos::common::async_central::AsyncCentralManager::getInstance()
              ->addTimer(this, chaos::common::constants::CUTimersTimeoutinMSec,
                         chaos::common::constants::CUTimersTimeoutinMSec);
        } else {
          DPD_LERR << " cannot register dataset " << i->first;
        
        
          return -1;
        }
      }
    }
  }
  pushDataset(chaos::DataPackCommonKey::DPCK_DATASET_TYPE_SYSTEM);
  return 0;
}
uint64_t ChaosDatasetIO::queryHistoryDatasets(uint64_t ms_start,
                                              uint64_t ms_end, uint32_t page,
                                              int type) {
  return queryHistoryDatasets(uid, ms_start, ms_end, page, type);
}

uint64_t ChaosDatasetIO::queryHistoryDatasets(const std::string &dsname,
                                              uint64_t ms_start,
                                              uint64_t ms_end, uint32_t page,
                                              int type) {
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
  q.page_len = page;
  q.qc = pnt;
  q.qt = query_index;
  query_cursor_map[query_index] = q;
  return query_index;
}
/**
             Perform a full query on the current device specifing a starting
   runid and a sequid and a tag.
             */
uint64_t ChaosDatasetIO::queryHistoryDatasets(uint64_t ms_start,
                                              uint64_t ms_end,
                                              const uint64_t runid,
                                              const uint64_t sequid,
                                              const ChaosStringSet &meta_tags,
                                              uint32_t page, int type) {
  return queryHistoryDatasets(uid, ms_start, ms_end, runid, sequid, meta_tags,
                              page, type);
}

uint64_t ChaosDatasetIO::queryHistoryDatasets(
    const std::string &dsname, uint64_t ms_start, uint64_t ms_end,
    const uint64_t runid, const uint64_t sequid,
    const ChaosStringSet &meta_tags, uint32_t page, int type) {
  std::string dst = dsname + chaos::datasetTypeToPostfix(type);
  DPD_LDBG << "Query To:" << dst << " start:" << ms_start
           << " end_ms:" << ms_end << "runid:" << runid << " sequid:" << sequid
           << "ntags:" << meta_tags.size() << " page:" << page;

  chaos::common::io::QueryCursor *pnt = ioLiveDataDriver->performQuery(
      dst, ms_start, ms_end, sequid, runid, meta_tags, ChaosStringSet(),page);
  if (pnt == NULL) {
    DPD_LERR << "NO CURSOR";

    return 0;
  }
  query_index++;
  qc_t q;
  q.page_len = page;
  q.qc = pnt;
  q.qt = query_index;
  query_cursor_map[query_index] = q;
  return query_index;
}
bool ChaosDatasetIO::queryHasNext(uint64_t uid) {
  bool ret;
  query_cursor_map_t::iterator i = query_cursor_map.find(uid);
  if (i == query_cursor_map.end()) {
    DPD_LERR << "query ID:" << uid << " not exists";
    return false;
  }
  chaos::common::io::QueryCursor *pnt = (i->second).qc;
  ret = pnt->hasNext();
  DPD_LDBG << "query ID:" << uid << " (0xx" << std::hex << pnt
           << ") has next: " << std::dec << ret;

  return ret;
}
std::vector<ChaosDataSet> ChaosDatasetIO::getNextPage(uint64_t uid) {
  std::vector<ChaosDataSet> ret;
  query_cursor_map_t::iterator i = query_cursor_map.find(uid);
  if (i == query_cursor_map.end()) {
    DPD_LERR << "query ID:" << uid << " not exists";
    return ret;
  }
  chaos::common::io::QueryCursor *pnt = (i->second).qc;
  uint32_t len = (i->second).page_len;
  int cnt = 0;
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
void ChaosDatasetIO::setImplementation(const std::string &impl) {
  implementation = impl;
}

std::vector<ChaosDataSet> ChaosDatasetIO::queryHistoryDatasets(
    const std::string &dsname, uint64_t ms_start, uint64_t ms_end, int type) {
  std::vector<ChaosDataSet> ret;
  std::string dst = dsname + chaos::datasetTypeToPostfix(type);
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
ChaosDatasetIO::queryHistoryDatasets(uint64_t ms_start, uint64_t ms_end,
                                     int type) {
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

int32_t ChaosDatasetIO::findMax(ChaosDataSet &ds,
                                std::vector<std::string> &dataset_key) {
  int32_t ret = 0;

  for (std::vector<std::string>::iterator it = dataset_key.begin();
       it != dataset_key.end(); it++) {
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
int ChaosDatasetIO::setCUAlarmLevel(const std::string &name, uint8_t value,
                                    const std::string msg) {
  if (!(datasets[chaos::DataPackCommonKey::DPCK_DATASET_TYPE_CU_ALARM]->hasKey(
          name))) {
    throw chaos::CException(-1, "No CU Alarm \"" + name + "\" found",
                            __PRETTY_FUNCTION__);

    return -1;
  }

  datasets[chaos::DataPackCommonKey::DPCK_DATASET_TYPE_CU_ALARM]->setValue(
      name, value);
  cu_alarm_lvl =
      findMax(datasets[chaos::DataPackCommonKey::DPCK_DATASET_TYPE_CU_ALARM],
              cu_alarms);
  datasets[chaos::DataPackCommonKey::DPCK_DATASET_TYPE_SYSTEM]->setValue(
      chaos::ControlUnitDatapackSystemKey::CU_ALRM_LEVEL, cu_alarm_lvl);
  pushDataset(chaos::DataPackCommonKey::DPCK_DATASET_TYPE_SYSTEM);
  pushDataset(chaos::DataPackCommonKey::DPCK_DATASET_TYPE_CU_ALARM);
  if (msg.size() > 0) {
    log("CU Alarm", cu_alarm_lvl + 1, msg);
  }
  return cu_alarm_lvl;
}
int ChaosDatasetIO::setDeviceAlarmLevel(const std::string &name, uint8_t value,
                                        const std::string msg) {
  if (!(datasets[chaos::DataPackCommonKey::DPCK_DATASET_TYPE_DEV_ALARM]->hasKey(
          name))) {
    throw chaos::CException(-1, "No DEV Alarm \"" + name + "\" found",
                            __PRETTY_FUNCTION__);

    return -1;
  }

  datasets[chaos::DataPackCommonKey::DPCK_DATASET_TYPE_DEV_ALARM]->setValue(
      name, value);
  dev_alarm_lvl =
      findMax(datasets[chaos::DataPackCommonKey::DPCK_DATASET_TYPE_DEV_ALARM],
              dev_alarms);

  datasets[chaos::DataPackCommonKey::DPCK_DATASET_TYPE_SYSTEM]->setValue(
      chaos::ControlUnitDatapackSystemKey::DEV_ALRM_LEVEL, dev_alarm_lvl);
  pushDataset(chaos::DataPackCommonKey::DPCK_DATASET_TYPE_SYSTEM);
  pushDataset(chaos::DataPackCommonKey::DPCK_DATASET_TYPE_DEV_ALARM);
  if (msg.size() > 0) {
    log("Device Alarm", dev_alarm_lvl + 1, msg);
  }

  return dev_alarm_lvl;
}

void ChaosDatasetIO::log(const std::string &subject, int log_leve,
                         const std::string &message) {
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
void ChaosDatasetIO::deinit() {

  if (deinitialized) {
    DEBUG_CODE(DPD_LDBG << "Already deinitialized");
    return;
  }
  {
    EXECUTE_CHAOS_API(api_proxy::control_unit::DeleteInstance, timeo, uid,
                      groupName);
  }
  DEBUG_CODE(DPD_LDBG << "Timer removed");
  CHAOS_NOT_THROW(InizializableService::deinitImplementation(
                      MetadataLoggingManager::getInstance(),
                      "MetadataLoggingManager", __PRETTY_FUNCTION__););

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
      uid, chaos::NodeHealtDefinitionKey::NODE_HEALT_STATUS,
      chaos::NodeHealtDefinitionValue::NODE_HEALT_STATUS_DEINIT, true);

  for (std::map<int,
                ChaosSharedPtr<chaos::common::data::CDataWrapper>>::iterator i =
           datasets.begin();
       i != datasets.end(); i++) {
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
} // namespace misc
} // namespace driver
