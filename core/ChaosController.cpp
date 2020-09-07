/*
 * File:   ChaosController.cpp
 * Author: michelo
 *
 * Created on September 2, 2015, 5:29 PM
 */
#define CMD_BY_MDS
#include "ChaosController.h"
#include <chaos_metadata_service_client/ChaosMetadataServiceClient.h>
#include <chaos_metadata_service_client/api_proxy/unit_server/NewUS.h>
#include <chaos_metadata_service_client/api_proxy/unit_server/DeleteUS.h>
#include <chaos_metadata_service_client/api_proxy/unit_server/GetSetFullUnitServer.h>

#include <chaos_metadata_service_client/api_proxy/unit_server/ManageCUType.h>
#include <chaos_metadata_service_client/api_proxy/service/SetSnapshotDatasetsForNode.h>

#include <chaos_metadata_service_client/api_proxy/control_unit/SetInstanceDescription.h>
#include <chaos_metadata_service_client/api_proxy/control_unit/Delete.h>
#include <chaos_metadata_service_client/api_proxy/control_unit/DeleteInstance.h>
#include <chaos_metadata_service_client/api_proxy/agent/agent.h>
#include <chaos_metadata_service_client/api_proxy/node/CommandTemplateSubmit.h>

#include <chaos_metadata_service_client/api_proxy/logging/logging.h>
#include <chaos/common/utility/TimingUtil.h>

#include <chaos/common/exception/CException.h>

#include <common/debug/core/debug.h>
#include <ctype.h>
#include <json/json.h>
#include <json/reader.h>
#include <json/writer.h>
#include <json/value.h>
using namespace chaos::common::message;
using namespace chaos::cu::data_manager;
using namespace chaos::common::data;
using namespace chaos::metadata_service_client;
using namespace chaos::common::io;
using namespace chaos::common::network;

using namespace ::driver::misc;

#define DBGET DBG_LOG(ChaosController) << "[" << getPath() << "]"
#define DBGETERR ERR_LOG(ChaosController) << "[" << getPath() << "]"
#define CTRLDBG_ DBG_LOG(ChaosController) 
#define CTRLERR_ ERR_LOG(ChaosController)

#define CALC_EXEC_TIME                                                                             \
    tot_us += (reqtime - chaos::common::utility::TimingUtil::getTimeStampInMicroseconds());        \
    if (naccess % 500 == 0)                                                                        \
    {                                                                                              \
        refresh = tot_us / 500;                                                                    \
        tot_us = 0;                                                                                \
        CTRLDBG_ << " Profiling: N accesses:" << naccess << " response time:" << refresh << " us"; \
    }

void ChaosController::setTimeout(uint64_t timeo_us)
{
    timeo = timeo_us;
}


int ChaosController::forceState(int dstState)
{
    chaos::CUStateKey::ControlUnitState currState, oldstate;
    boost::posix_time::ptime start;
    int retry = 10;

    do
    {
        oldstate = currState;
        getState(currState);

        DBGET << "Current state :" << currState << " destination state:" << dstState;
        if (currState == dstState)
        {
            return 0;
        }
        if (currState != oldstate)
        {
            start = boost::posix_time::microsec_clock::local_time();
        }

        if (currState < 0)
            return currState;

        switch (currState)
        {
        case chaos::CUStateKey::DEINIT:
            DBGET << "[deinit] apply \"init\"";
            initDevice();
            break;

        case chaos::CUStateKey::INIT:
            switch (dstState)
            {
            case chaos::CUStateKey::DEINIT:
                DBGET << "[init] apply \"deinit\" ";
                deinitDevice();
                break;
            case chaos::CUStateKey::START:
            case chaos::CUStateKey::STOP:
                DBGET << "[init] apply \"start\"";
                ;
                startDevice();
                break;
            }

            break;

        case chaos::CUStateKey::START:
            DBGET << "[start] apply \"stop\"";
            stopDevice();
            break;

        case chaos::CUStateKey::STOP:
            switch (dstState)
            {
            case chaos::CUStateKey::DEINIT:
            case chaos::CUStateKey::INIT:
                DBGET << "[stop] apply \"deinit\"";
                deinitDevice();
                break;
            case chaos::CUStateKey::START:
                DBGET << "[stop] apply \"start\"";
                startDevice();
                break;
            }

            break;
        default:
            return 0;
            
        }
        if ((boost::posix_time::microsec_clock::local_time() - start).total_microseconds() > timeo)
        {
            retry--;
            CTRLERR_ << "[" << getPath() << "] Timeout of " << timeo << " us elapsed:" << (boost::posix_time::microsec_clock::local_time() - start).total_microseconds() << "  Retry:" << retry;
            /*if (init(path, timeo) != 0) {
                CTRLERR_ << "cannot retrive controller for:" << path;
                return -1;

                }*/
            start = boost::posix_time::microsec_clock::local_time();
        }
    } while ((currState != dstState) && (retry > 0));

    if (retry == 0)
    {
        CTRLERR_ << "[" << getPath() << "]"
                 << " Not Responding";
        return -100;
    }

    return 0;
}

int ChaosController::init(int force)
{

    if (force)
    {

        return forceState(chaos::CUStateKey::INIT);
    }
    return initDevice();
}

int ChaosController::stop(int force)
{
    if (force)
    {
        return forceState(chaos::CUStateKey::STOP);
    }
    return stopDevice();
}

int ChaosController::start(int force)
{
    if (force)
    {
        return forceState(chaos::CUStateKey::START);
    }
    return startDevice();
}

int ChaosController::deinit(int force)
{
    if (force)
    {
        return forceState(chaos::CUStateKey::DEINIT);
    }
    return deinitDevice();
}

uint64_t ChaosController::getState(chaos::CUStateKey::ControlUnitState &stat,const std::string& dev)
{
    uint64_t ret = 0;
    std::string name=(dev=="")?path:dev;
    chaos::common::data::CDWShrdPtr tmp=getLiveChannel(name,KeyDataStorageDomainHealth);
    stat = chaos::CUStateKey::UNDEFINED;
    if (tmp.get() && tmp->hasKey(chaos::NodeHealtDefinitionKey::NODE_HEALT_STATUS))
    {
        cached_channels[chaos::DataPackCommonKey::DPCK_DATASET_TYPE_HEALTH]=tmp;
        std::string state = tmp->getCStringValue(chaos::NodeHealtDefinitionKey::NODE_HEALT_STATUS);
        if ((state == chaos::NodeHealtDefinitionValue::NODE_HEALT_STATUS_START) || (state == chaos::NodeHealtDefinitionValue::NODE_HEALT_STATUS_STARTING))
            stat = chaos::CUStateKey::START;
        else if ((state == chaos::NodeHealtDefinitionValue::NODE_HEALT_STATUS_STOP) || (state == chaos::NodeHealtDefinitionValue::NODE_HEALT_STATUS_STOPING))
            stat = chaos::CUStateKey::STOP;
        else if ((state == chaos::NodeHealtDefinitionValue::NODE_HEALT_STATUS_INIT) || (state == chaos::NodeHealtDefinitionValue::NODE_HEALT_STATUS_INITING))
            stat = chaos::CUStateKey::INIT;
        else if ((state == chaos::NodeHealtDefinitionValue::NODE_HEALT_STATUS_DEINIT) || (state == chaos::NodeHealtDefinitionValue::NODE_HEALT_STATUS_DEINITING) || (state == chaos::NodeHealtDefinitionValue::NODE_HEALT_STATUS_LOAD))
            stat = chaos::CUStateKey::DEINIT;
        else if ((state == chaos::NodeHealtDefinitionValue::NODE_HEALT_STATUS_RERROR))
            stat = chaos::CUStateKey::RECOVERABLE_ERROR;
        else if ((state == chaos::NodeHealtDefinitionValue::NODE_HEALT_STATUS_FERROR))
            stat = chaos::CUStateKey::FATAL_ERROR;

        if (tmp->hasKey(chaos::NodeHealtDefinitionKey::NODE_HEALT_TIMESTAMP))
        {
            ret = tmp->getInt64Value(chaos::NodeHealtDefinitionKey::NODE_HEALT_TIMESTAMP);
        }
        return ret;
    }

    return 0;
}

uint64_t ChaosController::getTimeStamp(const std::string& dev,int domain)
{
    uint64_t ret=0;
    ChaosSharedPtr<chaos::common::data::CDataWrapper> obj=getLiveChannel(dev,domain);
    if(obj.get()&&obj->hasKey(chaos::DataPackCommonKey::DPCK_TIMESTAMP)){
      ret= obj->getInt64Value(chaos::DataPackCommonKey::DPCK_TIMESTAMP);
    }
    return ret;
}

ChaosController::command_t ChaosController::prepareCommand(std::string alias)
{
    ChaosController::command_t cmd = boost::shared_ptr<command>(new command());
    cmd->alias = alias;
    return cmd;
}

std::string ChaosController::getJsonState()
{
    std::string ret;
    ret = bundle_state.getData()->getCompliantJSONString();
    return ret;
}

void ChaosController::initializeAttributeIndexMap() {
    //	boost::mutex::scoped_lock lock(trackMutext);
    vector<string> attributeNames;
    RangeValueInfo attributerangeInfo;
    
    attributeValueMap.clear();
    
    
    //get all attribute name from db
    datasetDB.getDatasetAttributesName(chaos::DataType::Input, attributeNames);
    for (vector<string>::iterator iter = attributeNames.begin();
         iter != attributeNames.end();
         iter++) {
        
        if(datasetDB.getAttributeRangeValueInfo(*iter, attributerangeInfo)!=0){
            LERR_<<"CANNOT RETRIVE attr range info of:"<<*iter;
        }
        //     LDBG_<<"IN attr:"<<attributerangeInfo.name<<" type:"<<attributerangeInfo.valueType<<" bin type:"<<attributerangeInfo.binType;
        attributeValueMap.insert(make_pair(*iter, attributerangeInfo));
        
    }
    
    attributeNames.clear();
    datasetDB.getDatasetAttributesName(chaos::DataType::Output, attributeNames);
    for (vector<string>::iterator iter = attributeNames.begin();
         iter != attributeNames.end();
         iter++) {
        
        
        if(datasetDB.getAttributeRangeValueInfo(*iter, attributerangeInfo)!=0){
            LERR_<<"CANNOT RETRIVE attr range info of:"<<*iter;
            
        }
        //    LDBG_<<"OUT attr:"<<attributerangeInfo.name<<" type:"<<attributerangeInfo.valueType<<" bin type:"<<attributerangeInfo.binType;
        attributeValueMap.insert(make_pair(*iter, attributerangeInfo));
        
    }
    
}

std::vector<chaos::common::data::RangeValueInfo> ChaosController::getDeviceValuesInfo(){
    std::vector<chaos::common::data::RangeValueInfo> ret;
    for(std::map<std::string,chaos::common::data::RangeValueInfo>::iterator i= attributeValueMap.begin();i!=attributeValueMap.end();i++)
        ret.push_back(i->second);
    
    return ret;
}
int ChaosController::init(const std::string& p, uint64_t timeo_)
{

    path = p;
    state = chaos::CUStateKey::UNDEFINED;
    schedule = 0;

    bundle_state.reset();
    bundle_state.status(state);
    DBGET << "init CU NAME:\"" << path << "\""
          << " timeo:" << timeo_;
    last_access = reqtime = tot_us = naccess = refresh = 0;

    ChaosWriteLock ll(ioctrl);

   
    timeo = timeo_;
    wostate = 0;

    setUid(path);

  /*  for (int cnt = 0; cnt <= DPCK_LAST_DATASET_INDEX; cnt++)
    {
        last_ts[cnt] = 0;
        last_pckid[cnt] = 0;
    }*/
    last_health_ts=0;
    delta_update = 0;
    if (getState(state) == 0)
    {
        DBGET << "Uknown state for device assuming wostate";
        wostate = 1;
    }
    std::vector<chaos::common::data::RangeValueInfo> vi = getDeviceValuesInfo();
    for (std::vector<chaos::common::data::RangeValueInfo>::iterator i = vi.begin(); i != vi.end(); i++)
    {
        DBGET << "attr_name:" << i->name << "type:" << i->valueType;
        if ((i->valueType == chaos::DataType::TYPE_BYTEARRAY) && i->binType.size() && (i->binType[0] != chaos::DataType::SUB_TYPE_NONE))
        {
            binaryToTranslate.insert(std::make_pair(i->name, i->binType[0]));
            DBGET << i->name << " is binary of type:" << i->binType[0];
        } /*else if((i->valueType==chaos::DataType::TYPE_INT64)){
    binaryToTranslate.insert(std::make_pair(i->name,i->valueType));
    DBGET << i->name<<" is of type64 :"<<i->valueType;
    }*/
    }
    fetch(-1);
  //  fetch(255);
   
    DBGET << "initalization ok";

    last_access = boost::posix_time::microsec_clock::local_time().time_of_day().total_microseconds();

    return 0;
}

/*
int ChaosController::waitCmd()
{
    return waitCmd(last_cmd);
}

int ChaosController::waitCmd(command_t &cmd)
{
    int ret;
    boost::posix_time::ptime start = boost::posix_time::microsec_clock::local_time();
    chaos::common::batch_command::CommandState command_state;
    if (cmd == NULL)
        return -200;
    command_state.command_id = cmd->command_id;
    do
    {
        if ((ret = controller->getCommandState(command_state)) != 0)
        {
            return ret;
        }

    } while ((command_state.last_event != chaos::common::batch_command::BatchCommandEventType::EVT_COMPLETED) && (command_state.last_event != chaos::common::batch_command::BatchCommandEventType::EVT_RUNNING) && ((boost::posix_time::microsec_clock::local_time() - start).total_microseconds() < timeo));

    DBGET << " Command state last event:" << command_state.last_event;
    if ((command_state.last_event == chaos::common::batch_command::BatchCommandEventType::EVT_COMPLETED) || (command_state.last_event == chaos::common::batch_command::BatchCommandEventType::EVT_RUNNING))
    {
        return 0;
    }

    return -100;
}
*/
int ChaosController::sendMDSCmd(command_t& cmd){
    CDWUniquePtr local_command_pack(new CDataWrapper());
    CDWUniquePtr result_data;
    
    local_command_pack->appendAllElement(cmd->param);
    
    /*
    // set the default slow command information
    local_command_pack->addStringValue(chaos::common::batch_command::BatchCommandAndParameterDescriptionkey::BC_ALIAS, cmd->alias);
    local_command_pack->addInt32Value(chaos::common::batch_command::BatchCommandSubmissionKey::SUBMISSION_RULE_UI32, (uint32_t) cmd->sub_rule);
    local_command_pack->addInt32Value(chaos::common::batch_command::BatchCommandSubmissionKey::SUBMISSION_PRIORITY_UI32, (uint32_t) cmd->priority);
    
    local_command_pack->addInt64Value(chaos::common::batch_command::BatchCommandSubmissionKey::SCHEDULER_STEP_TIME_INTERVALL_UI64, cmd->scheduler_steps_delay);
    local_command_pack->addInt32Value(chaos::common::batch_command::BatchCommandSubmissionKey::SUBMISSION_RETRY_DELAY_UI32,  cmd->submission_checker_steps_delay);
    */    
    //err = deviceChannel->setAttributeValue(local_command_pack, false, millisecToWait);
    local_command_pack->addStringValue(chaos::NodeDefinitionKey::NODE_UNIQUE_ID, path);
    CTRLDBG_ << "sending command through MDS \"" << cmd->alias << "\" params:" << cmd->param.getJSONString()<< " full msg:"<<local_command_pack->getJSONString();

    //chaos::metadata_service_client::api_proxy::ApiProxyResult apires = GET_CHAOS_API_PTR(chaos::metadata_service_client::api_proxy::node::CommandTemplateSubmit)->execute(local_command_pack); 
    chaos::metadata_service_client::api_proxy::ApiProxyResult apires = GET_CHAOS_API_PTR(chaos::metadata_service_client::api_proxy::node::CommandTemplateSubmit)->execute(
    path,cmd->alias,cmd->sub_rule,cmd->priority,cmd->scheduler_steps_delay,cmd->submission_checker_steps_delay,local_command_pack);
    apires->setTimeout(5000);                                                                                         
    apires->wait();                                                                                                   
    if (apires->getError() ){  
        return -1;                            
    }
    return 0;
}
#if 0
int ChaosController::sendCmd(command_t &cmd, bool wait, uint64_t perform_at, uint64_t wait_for)
{
    int err = 0;
    if (cmd == NULL)
        return -2;
    if (perform_at)
    {
        cmd->param.addInt64Value("perform_at", perform_at);
        DBGET << "command will be performed at " << perform_at;
    }
    else if (wait_for)
    {
        cmd->param.addInt64Value("wait_for", wait_for);
        DBGET << "command will be performed in " << wait_for << " us";
    }
    CTRLAPP_ << "sending command \"" << cmd->alias << "\" params:" << cmd->param.getJSONString();
    if ((err = controller->submitSlowControlCommand(cmd->alias, cmd->sub_rule, cmd->priority, cmd->command_id, 0, cmd->scheduler_steps_delay, cmd->submission_checker_steps_delay, &cmd->param)) != 0)
    {
        CTRLERR_ << "[" << getPath() << "] error submitting: err:" << err << " timeout set to:" << controller->getRequestTimeWaith();
        return err;
    }

    chaos::common::batch_command::CommandState command_state;
    command_state.command_id = cmd->command_id;

    err += controller->getCommandState(command_state);

    //    LAPP_ << "command after:" << ss.str().c_str();
    return err;
}
#endif
int ChaosController::executeCmd(command_t &cmd, bool wait, uint64_t perform_at, uint64_t wait_for)
{
    int ret = sendMDSCmd(cmd);
    if (ret != 0)
    {
        // retry to update channel
        CTRLERR_ << "error sending command to:" << path << " update controller";

        if (init(path, timeo) == 0)
        {
            ret = sendMDSCmd(cmd);
        }
        else
        {
            CTRLERR_ << "cannot reinitialize controller:" << path;
        }
        return ret;
    }
    last_cmd = cmd;
    #if 0
    if (wait)
    {
        DBGET << "waiting command id:" << cmd->command_id;
        if ((ret = waitCmd(cmd)) != 0)
        {
            CTRLERR_ << "error waiting ret:" << ret;

            return ret;
        }
        DBGET << "command performed";
    }
    #endif
    return ret;
}

void ChaosController::releaseQuery(QueryCursor *query_cursor) {
    live_driver->releaseQuery(query_cursor);
}

void ChaosController::cleanUpQuery()
{
    uint64_t now = boost::posix_time::microsec_clock::local_time().time_of_day().total_milliseconds();
    int cleanedup = 0;
    for (query_cursor_map_t::iterator i = query_cursor_map.begin(); i != query_cursor_map.end(); i++)
    {
        unsigned long diff = (now - i->second.qt);
        if (diff > QUERY_PAGE_MAX_TIME)
        {
            DBGET << " Expired max time for paged query, removing uid:" << i->first << " query started at:" << diff / 1000 << " s ago";
            if (i->second.qc)
            {
                releaseQuery(i->second.qc);
            }
            query_cursor_map.erase(i);
            cleanedup++;
        }
    }
    if (cleanedup)
    {
        DBGET << " cleaned up " << cleanedup;
    }
}

void ChaosController::update(){
    CDWUniquePtr tmp_data_handler;
    LDBG_<<"UPDATING \""<<path<<"\"";
    int err = mdsChannel->getLastDatasetForDevice(path, tmp_data_handler, MDS_TIMEOUT);
    if(err!=0 || !tmp_data_handler.get()) return;
    datasetDB.addAttributeToDataSetFromDataWrapper(*tmp_data_handler);
    initializeAttributeIndexMap();
}
ChaosController::ChaosController(std::string p, uint32_t timeo_) : ::common::misc::scheduler::SchedTimeElem(p, 0), timeo(timeo_),datasetDB(true)
{
    int ret;
    heart = 0;
    mdsChannel = NetworkBroker::getInstance()->getMetadataserverMessageChannel();
    if (!mdsChannel)
        throw chaos::CException(-1, "No MDS Channel created", "ChaosController()");
    initializeClient();

    if ((ret = init(p, timeo_)) != 0)
    {
        throw chaos::CException(ret, "cannot allocate controller for:" + path + " check if exists", __FUNCTION__);
    }
    cachedChannel_v.resize(DPCK_LAST_DATASET_INDEX + 1);
    cached_channels.resize(DPCK_LAST_DATASET_INDEX + 1);
    /*db = ::common::misc::data::DBbaseFactory::getInstance(DEFAULT_DBTYPE,DEFAULT_DBNAME);
     db->setDBParameters("replication",DEFAULT_DBREPLICATION);

     db->addDBServer("127.0.0.1");
     if(db->connect()==0){
     DPRINT("connected to cassandra");
     } else {
     ERR("cannot connect to cassandra");
     }*/
    /*for (int cnt = -1; cnt <= DPCK_LAST_DATASET_INDEX; cnt++)
    {
        cachedJsonChannels[cnt] = fetch(cnt);
    }*/
}

void ChaosController::initializeClient()
{
    mds_client = ChaosMetadataServiceClient::getInstance();

    if (!mds_client)
        throw chaos::CException(-1, "cannot instatiate MDS CLIENT", "ChaosController()");

    mds_client->init();
    mds_client->start();
    live_driver = ChaosMetadataServiceClient::getInstance()->getDataProxyChannelNewInstance();
    if (!live_driver)
    {
        throw chaos::CException(-1, "No LIVE Channel created", "ChaosController()");
    }
    CDWUniquePtr best_available_da_ptr;

    if (!mdsChannel->getDataDriverBestConfiguration(best_available_da_ptr, timeo)){
        live_driver->updateConfiguration(best_available_da_ptr.get());
    }
    cached_channels=getLiveAllChannels();
}
void ChaosController::deinitializeClient()
{

    /*
     * if(mds_client){
        mds_client->stop();
    }sc
     */
}
#define CU_INPUT_UPDATE_US 500 * 1000
#define CU_CUSTOM_UPDATE_US 2000 * 1000

#define CU_SYSTEM_UPDATE_US 2000 * 1000
#define CU_HEALTH_UPDATE_US 2000 * 1000
#define CU_FREQ_UPDATE_US 5 * 1000 * 1000

uint64_t ChaosController::sched(uint64_t ts)
{
    CDataWrapper all, common;
   // chaos::common::data::VectorCDWShrdPtr channels;
    if((cached_channels.size()==0)||((ts-update_all_channels_ts)>CU_HEALTH_UPDATE_US)){
        cached_channels=getLiveAllChannels();
        update_all_channels_ts=ts;
    } else {
        
        cached_channels[KeyDataStorageDomainOutput]=getLiveChannel(path,KeyDataStorageDomainOutput);
    }
    if(cached_channels.size()==0){
        return 0;
    }
    delta_update = CU_HEALTH_UPDATE_US;
    for (int cnt = 0; cnt < cached_channels.size(); cnt++)
    {
        ChaosWriteLock l(iomutex);

        //if(channels[cnt]->hasKey("ndk_uid")&&(channels[cnt]->getString("ndk_uid")!=controller->)
        if(cached_channels[cnt].get()){
            std::string tmp= cached_channels[cnt]->getCompliantJSONString();
            if(tmp.size()>2){
                cachedJsonChannels[cnt] =tmp;
                all.addCSDataValue(chaos::datasetTypeToHuman(cnt), *(cached_channels[cnt].get()));

            }
            
        }
        if ((static_cast<chaos::cu::data_manager::KeyDataStorageDomain>(cnt) == KeyDataStorageDomainHealth) ||
            (static_cast<chaos::cu::data_manager::KeyDataStorageDomain>(cnt) == KeyDataStorageDomainSystem) ||
            (static_cast<chaos::cu::data_manager::KeyDataStorageDomain>(cnt) == KeyDataStorageDomainDevAlarm) ||
            (static_cast<chaos::cu::data_manager::KeyDataStorageDomain>(cnt) == KeyDataStorageDomainCUAlarm))
        {
            if(cached_channels[cnt].get()){

                common.addCSDataValue(chaos::datasetTypeToHuman(cnt), *(cached_channels[cnt].get()));
            }
        }

        /*
        *  if(!channels[cnt]->hasKey(chaos::DataPackCommonKey::DPCK_SEQ_ID))
            continue;
        tss=channels[cnt]->getInt64Value(chaos::DataPackCommonKey::DPCK_TIMESTAMP);
        pckid=channels[cnt]->getInt64Value(chaos::DataPackCommonKey::DPCK_SEQ_ID);
        if(pckid>last_pckid[cnt]){
          delta_update=std::min(delta_update,(tss-last_ts[cnt])*1000/(2*(pckid-last_pckid[cnt])));
          DBGET<<"reducing delta update to:"<<delta_update << " delta packets:"<<(pckid-last_pckid[cnt])<<" delta time:"<<(tss-last_ts[cnt]);
          ;
          last_pckid[cnt]= pckid;
          last_ts[cnt]=ts;
        } else{
          delta_update+=50;
        }
          */
    }
    all.appendAllElement(*bundle_state.getData());
    {
        ChaosWriteLock l(iomutex);
        cachedJsonChannels[-1] = all.getCompliantJSONString();
        cachedJsonChannels[255] = common.getCompliantJSONString();
    }
    double rate;
    if(cached_channels[chaos::DataPackCommonKey::DPCK_DATASET_TYPE_HEALTH].get()){
        last_health_ts=(cached_channels[chaos::DataPackCommonKey::DPCK_DATASET_TYPE_HEALTH]->hasKey(chaos::DataPackCommonKey::DPCK_TIMESTAMP)?cached_channels[chaos::DataPackCommonKey::DPCK_DATASET_TYPE_HEALTH]->getInt64Value(chaos::DataPackCommonKey::DPCK_TIMESTAMP):last_health_ts);
        if (cached_channels[chaos::DataPackCommonKey::DPCK_DATASET_TYPE_HEALTH]->hasKey(chaos::ControlUnitHealtDefinitionValue::CU_HEALT_OUTPUT_DATASET_PUSH_RATE) &&
        ((rate = cached_channels[chaos::DataPackCommonKey::DPCK_DATASET_TYPE_HEALTH]->getDoubleValue(chaos::ControlUnitHealtDefinitionValue::CU_HEALT_OUTPUT_DATASET_PUSH_RATE)) > 0))
    {
        delta_update = (1000 * 1000.0) / (2 * rate);
    }
        
    } else if ((cached_channels[chaos::DataPackCommonKey::DPCK_DATASET_TYPE_SYSTEM].get())&&cached_channels[chaos::DataPackCommonKey::DPCK_DATASET_TYPE_SYSTEM]->hasKey(chaos::ControlUnitDatapackSystemKey::THREAD_SCHEDULE_DELAY)&& (cached_channels[chaos::DataPackCommonKey::DPCK_DATASET_TYPE_SYSTEM]->getDoubleValue(chaos::ControlUnitDatapackSystemKey::THREAD_SCHEDULE_DELAY)>0))
    {

        delta_update = cached_channels[chaos::DataPackCommonKey::DPCK_DATASET_TYPE_SYSTEM]->getDoubleValue(chaos::ControlUnitDatapackSystemKey::THREAD_SCHEDULE_DELAY) / 2.0;
    }
    delta_update = std::min(delta_update, (uint64_t)CU_HEALTH_UPDATE_US);
    setQuantum(delta_update);
    //DBGET<<"SCHED STOP delta update:"<<delta_update;
    return delta_update;
}

CDWShrdPtr ChaosController::getLiveChannel(const std::string &key, int domain)
{
    size_t value_len = 0;
    ChaosSharedPtr<chaos::common::data::CDataWrapper> ret;
    std::string CUNAME=(key=="")?path:key;
    std::string lkey = CUNAME + chaos::datasetTypeToPostfix(domain);
    char *value = live_driver->retriveRawData(lkey, (size_t *)&value_len);
    if (value)
    {
        chaos::common::data::CDataWrapper *tmp = new CDataWrapper(value);
        ret.reset(tmp);
        delete []value;
        return ret;
    } else {
      DBGETERR << "error fetching data from \"" << lkey;;
    }
    return ret;
}
chaos::common::data::VectorCDWShrdPtr ChaosController::getLiveChannel(const std::vector<string> &keys, int domain)
{
    chaos::common::data::VectorCDWShrdPtr results;
    std::vector<std::string> channels;
    for ( std::vector<std::string>::const_iterator i = keys.begin(); i != keys.end(); i++)
    {
        channels.push_back(*i + chaos::datasetTypeToPostfix(domain));
    }
    live_driver->retriveMultipleData(channels, results);
    return results;
}
chaos::common::data::VectorCDWShrdPtr ChaosController::getLiveChannel(const std::vector<std::string>&channels){
    chaos::common::data::VectorCDWShrdPtr results;
    live_driver->retriveMultipleData(channels, results);
    return  results;
}

chaos::common::data::VectorCDWShrdPtr ChaosController::getLiveAllChannels(const std::string&n){
    chaos::common::data::VectorCDWShrdPtr results;
    std::vector<std::string> channels;
    std::string CUNAME=(n=="")?path:n;
    channels.push_back(CUNAME + chaos::datasetTypeToPostfix(KeyDataStorageDomainOutput));
    channels.push_back(CUNAME + chaos::datasetTypeToPostfix(KeyDataStorageDomainInput));
    channels.push_back(CUNAME + chaos::datasetTypeToPostfix(KeyDataStorageDomainCustom));
    channels.push_back(CUNAME + chaos::datasetTypeToPostfix(KeyDataStorageDomainSystem));
    channels.push_back(CUNAME + chaos::datasetTypeToPostfix(KeyDataStorageDomainHealth));
    channels.push_back(CUNAME + chaos::datasetTypeToPostfix(KeyDataStorageDomainDevAlarm));
    channels.push_back(CUNAME + chaos::datasetTypeToPostfix(KeyDataStorageDomainCUAlarm));
    if(live_driver->retriveMultipleData(channels, results)!=0){
        CTRLERR_<<"Error retriving multiple data for:"<<CUNAME;
    }
    return results;
}

chaos::common::data::VectorCDWShrdPtr ChaosController::getLiveChannel(chaos::common::data::CMultiTypeDataArrayWrapper *keys, int domain)
{
    chaos::common::data::VectorCDWShrdPtr results;
    std::vector<std::string> channels;
    for (int cnt = 0; cnt < keys->size(); cnt++)
    {
        channels.push_back(keys->getStringElementAtIndex(cnt) + chaos::datasetTypeToPostfix(domain));
    }
    if(live_driver->retriveMultipleData(channels, results)!=0){
        CTRLERR_<<"Error retriving multiple data";
 
    }
    return results;
}

ChaosController::ChaosController() : ::common::misc::scheduler::SchedTimeElem("none", 0), state(chaos::CUStateKey::UNDEFINED), queryuid(0), last_access(0), heart(0), reqtime(0), tot_us(0), naccess(0), refresh(0),update_all_channels_ts(0), timeo(DEFAULT_TIMEOUT_FOR_CONTROLLER),datasetDB(true)

{

    mdsChannel = NetworkBroker::getInstance()->getMetadataserverMessageChannel();
    cachedChannel_v.resize(DPCK_LAST_DATASET_INDEX + 1);

    if (!mdsChannel)
        throw chaos::CException(-1, "No MDS Channel created", "ChaosController()");

    //cassandra = ;
    /*	db = ::common::misc::data::DBbaseFactory::getInstance(DEFAULT_DBTYPE,DEFAULT_DBNAME);
     db->setDBParameters("replication",DEFAULT_DBREPLICATION);

     db->addDBServer("127.0.0.1");
     if(db->connect()==0){
     DPRINT("connected to cassandra");
     } else {
     ERR("cannot connect to cassandra");
     }

     */
    initializeClient();
}

ChaosController::~ChaosController()
{
    /*if(db){
     db->disconnect();
     }
     */

    if (mdsChannel)
    {
        NetworkBroker::getInstance()->disposeMessageChannel(mdsChannel);
        mdsChannel = NULL;
    }
   
    
    deinitializeClient();
    DBGET<<"deleted ChaosController:"<<getPath();
}

boost::shared_ptr<chaos::common::data::CDataWrapper> ChaosController::combineDataSets(std::map<int, chaos::common::data::CDataWrapper *> set)
{
    std::map<int, chaos::common::data::CDataWrapper *>::iterator i;
    boost::shared_ptr<chaos::common::data::CDataWrapper> data;
    chaos::common::data::CDataWrapper resdata;
    uint64_t time_stamp = boost::posix_time::microsec_clock::local_time().time_of_day().total_milliseconds();
    resdata.addStringValue("name", getPath());
    resdata.addInt64Value("timestamp", time_stamp);

    for (i = set.begin(); i != set.end(); i++)
    {
        if (i->second)
        {
          #if 1
            data = normalizeToJson(i->second, binaryToTranslate);
            //out<<",\"input\":"<<data->getCompliantJSONString();
            resdata.addCSDataValue(chaos::datasetTypeToHuman(i->first), *(data.get()));
          #else
                resdata.addCSDataValue(chaos::datasetTypeToHuman(i->first), *(i->second));
          #endif
        }
        else
        {
            std::stringstream ss;
            chaos::common::data::CDataWrapper empty;
            resdata.addCSDataValue(chaos::datasetTypeToHuman(i->first), empty);

            DBGETERR << "error fetching data from \"" << chaos::datasetTypeToHuman(i->first) << "\" channel ";

            //bundle_state.append_error(ss.str());
            //return bundle_state.getData();
        }
    }
    if (data.get())
    {
        data->reset();
        data->appendAllElement(resdata);
        //	data->appendAllElement(*bundle_state.getData());
        //	DBGET<<"channel "<<channel<<" :"<<odata->getCompliantJSONString();
    }
    return data;
}
const std::string ChaosController::fetchJson(int channel)
{
    ChaosReadLock ll(iomutex);
    std::string ret=cachedJsonChannels[channel];
    return ret;
}

chaos::common::data::CDWUniquePtr ChaosController::fetch(int channel)
{
    //	boost::mutex::scoped_lock(iomutex);

    chaos::common::data::CDWUniquePtr retdata(new CDataWrapper);
    try
    {
        if (channel == -1)
        {
           
            chaos::common::data::VectorCDWShrdPtr res=getLiveAllChannels();
#if 0       
            chaos::common::data::CDataWrapper *idata = NULL, *odata = NULL;
            chaos::common::data::CDataWrapper resdata;
            std::stringstream out;
            uint64_t ts = 0;
            std::map<int, chaos::common::data::CDataWrapper *> set;
            CDataWrapper ch[7];
            if(res.size()>=7){
                
                set[KeyDataStorageDomainOutput] = res[0].get();         

                set[KeyDataStorageDomainInput] = res[1].get();
                set[KeyDataStorageDomainCustom] =res[2].get();         
                set[KeyDataStorageDomainSystem] =res[3].get();         
                set[KeyDataStorageDomainHealth] =res[4].get();         
                set[KeyDataStorageDomainDevAlarm] =res[5].get();         
                set[KeyDataStorageDomainCUAlarm] = res[6].get();         
                retdata = combineDataSets(set);
            }
#else
        for (int cnt = 0; cnt < res.size(); cnt++){
            if(res[cnt].get()){
                retdata->addCSDataValue(chaos::datasetTypeToHuman(cnt), *(res[cnt].get()));
            }
	    //            DBGET<<"channel "<<chaos::datasetTypeToHuman(cnt)<<" :"<<res[cnt]->getCompliantJSONString();
        }

#endif
        } else if(channel==128){
            CDWShrdPtr custom=getLiveChannel(path,KeyDataStorageDomainCustom);
            CDWShrdPtr poiv=getLiveChannel(path,KeyDataStorageDomainOutput);

            if(custom.get()&&custom->hasKey("cudk_load_param")){
                CDWUniquePtr cudk_load_param=custom->getCSDataValue("cudk_load_param");
                if(cudk_load_param.get()&&cudk_load_param->hasKey("poi")){

                    CDWUniquePtr poi=cudk_load_param->getCSDataValue("poi");
                    ChaosStringVector s;
                    poi->getAllKey(s);
                    for(ChaosStringVector::iterator i=s.begin();i!=s.end();i++){
                       // CDataWrapper po;
                       // po.addStringValue(*i,poi->getStringValue(*i));
                       // retdata->appendCDataWrapperToArray(po);
                       retdata->appendStringToArray(*i);
                    }
              /*      if(s.size()==0){
                        retdata->appendStringToArray("");

                    }*/
                    retdata->finalizeArrayForKey("poilist");
                    if(poiv.get()&&poiv->hasKey("POI")){
                        retdata->addStringValue("poivalue",poiv->getStringValue("POI"));

                    } else {
                        retdata->addStringValue("poivalue","");

                    }


                  
                } else {
                    // empty
               //     retdata->appendStringToArray("");

                    retdata->finalizeArrayForKey("poilist");
                    retdata->addStringValue("poivalue","");


                }


            } else {
                 //   retdata->appendStringToArray("");

                    retdata->finalizeArrayForKey("poilist");
                    retdata->addStringValue("poivalue","");

            }
            CDWShrdPtr system=getLiveChannel(path,KeyDataStorageDomainSystem);
            if(system.get()){
                retdata->addCSDataValue("system",*system.get());
            } else {
                retdata->addCSDataValue("system",CDataWrapper());

            }
             if(poiv.get()&&poiv->hasKey(chaos::DataPackCommonKey::DPCK_TIMESTAMP)){
                        retdata->addInt64Value(chaos::DataPackCommonKey::DPCK_TIMESTAMP,poiv->getInt64Value(chaos::DataPackCommonKey::DPCK_TIMESTAMP));

                    } else {
                        retdata->addInt64Value(chaos::DataPackCommonKey::DPCK_TIMESTAMP,(int64_t)0);


                    }
            return retdata;
        } else if (channel == 255)
        {
           
            std::vector<std::string> channels;

            channels.push_back(path + chaos::datasetTypeToPostfix(KeyDataStorageDomainHealth));
            channels.push_back(path + chaos::datasetTypeToPostfix(KeyDataStorageDomainSystem));
            channels.push_back(path + chaos::datasetTypeToPostfix(KeyDataStorageDomainDevAlarm));
            channels.push_back(path + chaos::datasetTypeToPostfix(KeyDataStorageDomainCUAlarm));
            chaos::common::data::VectorCDWShrdPtr res=getLiveChannel(channels);
#if 0
         chaos::common::data::CDataWrapper *idata = NULL, *odata = NULL;
            chaos::common::data::CDataWrapper resdata;
            std::stringstream out;
            uint64_t ts = 0;
            std::map<int, chaos::common::data::CDataWrapper *> set;
            CDataWrapper ch[7];
            if(res.size()>=4){
                set[KeyDataStorageDomainHealth] =res[0].get();         
                set[KeyDataStorageDomainSystem] =res[1].get();         
                set[KeyDataStorageDomainDevAlarm] =res[2].get();         
                set[KeyDataStorageDomainCUAlarm] = res[3].get();        
                retdata = combineDataSets(set);
            }
#else
            if(res.size()){
                if(res[0].get()){
                    retdata->addCSDataValue(chaos::datasetTypeToHuman(KeyDataStorageDomainHealth), *(res[0].get()));
                }
                if(res[1].get()){
                    retdata->addCSDataValue(chaos::datasetTypeToHuman(KeyDataStorageDomainSystem), *(res[1].get()));
                }
                if(res[2].get()){
                    retdata->addCSDataValue(chaos::datasetTypeToHuman(KeyDataStorageDomainDevAlarm), *(res[2].get()));
                }
                if(res[3].get()){
                    retdata->addCSDataValue(chaos::datasetTypeToHuman(KeyDataStorageDomainCUAlarm), *(res[3].get()));
                }
            }
            
#endif
        }
        else
        {
            CDataWrapper data;
            ChaosSharedPtr<chaos::common::data::CDataWrapper> res=getLiveChannel(path,channel);

            if (res.get()==NULL)
            {
                std::stringstream ss;
                ss << "error fetching data from channel " << channel;
                bundle_state.append_error(ss.str());
                retdata->appendAllElement(*bundle_state.getData());
                return retdata;
            }
           // retdata = normalizeToJson(res.get(), binaryToTranslate);
            retdata->appendAllElement(*(res.get()));

        }

        //        DBGET<<"channel "<<channel<<" :"<<data->getCompliantJSONString();
    }
    catch (chaos::CException &e)
    {
        std::stringstream ss;
        retdata.reset(new CDataWrapper());
        ss << "exception fetching data from channel " << channel << " \"" << e.what() << "\"";
        bundle_state.append_error(ss.str());
        retdata->appendAllElement(*bundle_state.getData());
        return retdata;
    }
   #if 0
    if (retdata.get())
    {
        retdata->appendAllElement(*bundle_state.getData());
    }
    else
    {
        retdata.reset(new CDataWrapper());
        retdata->appendAllElement(*bundle_state.getData());
    }
    #endif
    return retdata;
}

std::string ChaosController::map2Json(std::map<uint64_t, std::string> &node)
{
    std::stringstream ss;
    ss << "[";
    for (std::map<uint64_t, std::string>::iterator jj = node.begin(); jj != node.end(); jj++)
    {
        if (std::distance(jj, node.end()) > 1)
        {
            ss << "{\"name\":\"" << jj->second << "\",\"ts\":" << jj->first << "},";
        }
        else
        {
            ss << "{\"name\":\"" << jj->second << "\",\"ts\":" << jj->first << "}";
        }
    }
    ss << "]";
    return ss.str();
}

std::string ChaosController::dataset2Var(chaos::common::data::CDataWrapper *data, std::string &var_name)
{
    std::stringstream res;
    uint64_t ts = data->getInt64Value("dpck_ats");
    uint64_t seq = data->getInt64Value("dpck_seq_id");
    if (var_name.size() == 0 || !data->hasKey(var_name))
    {
        res << "{\"ts\":" << ts << ","
            << "\"seq\":" << seq << ",\"val\": {}}";
        return res.str();
    }
    if (data->isVectorValue(var_name))
    {
        ChaosSharedPtr<CMultiTypeDataArrayWrapper> dw = data->getVectorValue(var_name);
        //ChaosUniquePtr<CDataWrapper> dw(data->getCSDataValue(var_name));
        //SONElement obj(data->getRawValuePtr(var_name));
        res << "{\"ts\":" << ts << ","
            << "\"seq\":" << seq << ",\"val\":" << dw->getCanonicalJSONString() << "}";
    }
    else
    {
        chaos::common::data::CDataVariant v = data->getVariantValue(var_name);
        res << "{\"ts\":" << ts << ","
            << "\"seq\":" << seq << ",\"val\":" << v.asString() << "}";
    }

    return res.str();
}

std::string ChaosController::vector2Json(ChaosStringVector &node_found)
{
    std::stringstream ss;
    ss << "[";
    for (ChaosStringVector::iterator i = node_found.begin(); i != node_found.end(); i++)
    {
        if (std::distance(i, node_found.end()) > 1)
        {
            ss << "\"" << *i << "\",";
        }
        else
        {
            ss << "\"" << *i << "\"";
        }
    }
    ss << "]";
    return ss.str();
}

void ChaosController::parseClassZone(ChaosStringVector &v)
{
    const boost::regex e("^(.*)/(.*)/(.*)$");
    boost::cmatch what;
    zone_to_cuname.clear();
    class_to_cuname.clear();
    for (ChaosStringVector::iterator i = v.begin(); i != v.end(); i++)
    {
        if (boost::regex_match(i->c_str(), what, e))
        {
            zone_to_cuname[what[1]] = *i;
            class_to_cuname[what[2]] = *i;
        }
    }
}

#define PARSE_QUERY_PARMS(args, check_name, check_what)                                            \
    std::string name = "";                                                                         \
    std::string what = "";                                                                         \
    bool alive = true;                                                                             \
    chaos_data::CDataWrapper p;                                                                    \
    ChaosSharedPtr<chaos::common::data::CMultiTypeDataArrayWrapper> names;                         \
    ChaosSharedPtr<chaos::common::data::CMultiTypeDataArrayWrapper> node_list;                     \
    ChaosUniquePtr<chaos::common::data::CDataWrapper> json_value;                                  \
    std::stringstream serr;                                                                        \
    std::string node_type, parent;                                                                 \
    int64_t seq_id = 0, start_ts = 0, end_ts = chaos::common::utility::TimingUtil::getTimeStamp(); \
    int page = 30;                                                                                 \
    if (args != NULL)                                                                              \
    {                                                                                              \
        p.setSerializedJsonData(args);                                                             \
        if (p.hasKey("names") && p.isVector("names"))                                              \
        {                                                                                          \
            names = p.getVectorValue("names");                                                     \
        }                                                                                          \
        if (p.hasKey("seq"))                                                                       \
        {                                                                                          \
            seq_id = p.getInt64Value("seq");                                                       \
        }                                                                                          \
        if (p.hasKey("page"))                                                                      \
        {                                                                                          \
            page = p.getInt32Value("page");                                                        \
        }                                                                                          \
        if (p.hasKey("type"))                                                                      \
        {                                                                                          \
            node_type = p.getStringValue("type");                                                  \
        }                                                                                          \
        if (p.hasKey("start"))                                                                     \
        {                                                                                          \
            start_ts = p.getInt64Value("start");                                                   \
        }                                                                                          \
        if (p.hasKey("end"))                                                                       \
        {                                                                                          \
            end_ts = p.getInt64Value("end");                                                       \
        }                                                                                          \
        if (p.hasKey("parent"))                                                                    \
        {                                                                                          \
            parent = p.getStringValue("parent");                                                   \
        }                                                                                          \
        if (p.hasKey("alive"))                                                                     \
        {                                                                                          \
            alive = p.getBoolValue("alive");                                                       \
        }                                                                                           \
        if (p.hasKey("name"))\
        {        \
            if(p.isVector("name")){                                                                 \
                names=p.getVectorValue("name");   \
            } else {                                                                                 \
                name = p.getStringValue("name");                                                       \
            } \
        }\
        if (p.hasKey("what"))                                                                      \
        {                                                                                          \
            what = p.getStringValue("what");                                                       \
        }                                                                                          \
        if (p.hasKey("value"))                                                                     \
        {                                                                                          \
            json_value = p.getCSDataValue("value");                                                \
            if (json_value.get() == NULL)                                                          \
            {                                                                                      \
                serr << cmd << " bad json format" << args;                                         \
                bundle_state.append_error(serr.str());                                             \
                json_buf = bundle_state.getData()->getCompliantJSONString();                       \
                return CHAOS_DEV_CMD;                                                              \
            }                                                                                      \
        }                                                                                          \
        if (p.hasKey("node_list") && p.isVector("node_list"))                                      \
        {                                                                                          \
            node_list = p.getVectorValue("node_list");                                             \
        }                                                                                          \
        if ((names.get() == NULL) && name.empty() && check_name)                                   \
        {                                                                                          \
            serr << "missing 'name' or 'names' in command:\"" << cmd << "\"";                      \
            bundle_state.append_error(serr.str());                                                 \
            json_buf = bundle_state.getData()->getCompliantJSONString();                           \
            return CHAOS_DEV_CMD;                                                                  \
        }                                                                                          \
        if (check_what && what.empty())                                                            \
        {                                                                                          \
            serr << "missing operation 'what'" << cmd;                                             \
            bundle_state.append_error(serr.str());                                                 \
            json_buf = bundle_state.getData()->getCompliantJSONString();                           \
            return CHAOS_DEV_CMD;                                                                  \
        }                                                                                          \
    }

#define CHECK_VALUE_PARAM                                            \
    if (json_value.get() == NULL)                                    \
    {                                                                \
        serr << "missing (json) value" << cmd;                       \
        bundle_state.append_error(serr.str());                       \
        json_buf = bundle_state.getData()->getCompliantJSONString(); \
        return CHAOS_DEV_CMD;                                        \
    }

#define RETURN_ERROR(msg)                                            \
    {                                                                \
        std::stringstream serr;                                      \
        serr << cmd << " \"" << args << "\" " << msg;                \
        bundle_state.append_error(serr.str());                       \
        json_buf = bundle_state.getData()->getCompliantJSONString(); \
        return CHAOS_DEV_CMD;                                        \
    }

#define CHECK_PARENT                          \
    if (parent.empty())                       \
    {                                         \
        RETURN_ERROR("must specify 'parent'") \
    };

#define CALL_CHAOS_API(api_name, time_out, cdp)                                                                         \
    DBGET << " "                                                                                                        \
          << " Executing CHAOS Api:\"" << #api_name << "\" parms:" << cdp->getCompliantJSONString();                    \
    chaos::metadata_service_client::api_proxy::ApiProxyResult apires = GET_CHAOS_API_PTR(api_name)->callApi(cdp);       \
    apires->setTimeout(time_out);                                                                                       \
    apires->wait();                                                                                                     \
    if (apires->getError())                                                                                             \
    {                                                                                                                   \
        std::stringstream ss;                                                                                           \
        ss << " error in :" << __FUNCTION__ << "|" << __LINE__ << "|" << #api_name << " " << apires->getErrorMessage(); \
        bundle_state.append_error(ss.str());                                                                            \
        json_buf = bundle_state.getData()->getCompliantJSONString();                                                    \
        return CHAOS_DEV_CMD;                                                                                           \
    }
#define EXECUTE_CHAOS_RET_API(ret,api_name, time_out, ...)                                                                        \
    DBGET << " "                                                                                                          \
          << " Executing Api:\"" << #api_name << "\"";                                                                    \
    chaos::metadata_service_client::api_proxy::ApiProxyResult apires = GET_CHAOS_API_PTR(api_name)->execute(__VA_ARGS__); \
    apires->setTimeout(time_out);                                                                                         \
    if(apires->wait()){ret= apires->getError();} else {ret =-1;}                                                                                

#define EXECUTE_CHAOS_API(api_name, time_out, ...)                                                                        \
    DBGET << " "                                                                                                          \
          << " Executing Api:\"" << #api_name << "\" name:"<<name;                                                                    \
    chaos::metadata_service_client::api_proxy::ApiProxyResult apires = GET_CHAOS_API_PTR(api_name)->execute(__VA_ARGS__); \
    apires->setTimeout(time_out);                                                                                         \
    json_buf = "{}";                                                                                                      \
    if ( (apires->wait() ==false) ||( apires->getError()) )                                                                                               \
    {                                                                                                                     \
        std::stringstream ss;                                                                                             \
        ss << " error in :" << __FUNCTION__ << "|" << __LINE__ << "|" << #api_name << " " << apires->getErrorMessage();   \
        bundle_state.append_error(ss.str());                                                                              \
        json_buf = bundle_state.getData()->getCompliantJSONString();                                                      \
        execute_chaos_api_error++;                                                                                          \
    } else {chaos::common::data::CDWUniquePtr r=apires->detachResult();if((r.get()!=NULL)) json_buf=r->getCompliantJSONString();}



chaos::common::data::CDWUniquePtr ChaosController::executeAPI(const std::string&group,const std::string&name,CDWUniquePtr& msg,int& err){
    
     err = 0;
    ChaosUniquePtr<MultiAddressMessageRequestFuture> request_future = mdsChannel->sendRequestWithFuture(group,
                                                                                            name,
                                                                                            MOVE(msg));
        request_future->setTimeout(timeo);
    if(request_future->wait()) {
        err = request_future->getError();
    } else {
        err = -1;
    }
    return request_future->detachResult();

}

static uint64_t getMSSince1970Until(const std::string& dateAndHour ) {

struct tm tm;
char buf[255];
uint64_t res=0;
memset(&tm, 0, sizeof(struct tm));
strptime(dateAndHour.c_str(), "%y%m%d%H%M%S", &tm);
           
  boost::chrono::system_clock::time_point tp = boost::chrono::system_clock::from_time_t(mktime(&tm));
    res=boost::chrono::duration_cast<boost::chrono::milliseconds>(tp.time_since_epoch()).count();
   LDBG_ << " Converting DATE:"<<dateAndHour<< " EPOCH:"<<res;

  return res;

}

uint64_t ChaosController::offsetToTimestamp(const std::string &off)
{
    boost::smatch what;
    boost::regex ts_ms("(^[0-9]+)$");
    boost::regex ts_data("^[0-9]{12}$");
    if((off=="NOW") || (off=="now")){
        return chaos::common::utility::TimingUtil::getTimeStamp();
    }
    if(boost::regex_match(off, what, ts_data)){
        return getMSSince1970Until(off);
    }

    if (boost::regex_match(off, what, ts_ms))
    {
        std::string dd = what[1];

        return strtoull(dd.c_str(), 0, 0);
    }
    
    boost::regex mm("(\\-){0,1}([0-9]+d){0,1}([0-9]+h){0,1}([0-9]+m){0,1}([0-9]+s){0,1}([0-9]+ms){0,1}");

    //        std::string::const_iterator start = input.begin() ;
    //std::string::const_iterator end = input.end() ;
    if (boost::regex_match(off, what, mm))
    {
        int64_t toff = 0;
        std::string dd = what[2];
        std::string h = what[3];
        std::string m = what[4];
        std::string s = what[5];
        std::string ms = what[6];
        std::string sign = what[1];
        toff += (strtoull(dd.c_str(), 0, 0) * (3600 * 24)) +
                (strtoull(h.c_str(), 0, 0) * (3600)) +
                (strtoull(m.c_str(), 0, 0) * (60)) +
                (strtoull(s.c_str(), 0, 0));
        toff *= 1000 * ((sign == "-") ? -1 : 1);
        toff += (strtoul(s.c_str(), 0, 0));
        uint64_t ret = chaos::common::utility::TimingUtil::getTimeStamp();
        ret += toff;
        //DBGET<<"offset "<<off<<" offset epoch: "<<std::dec<<ret;
        return ret;
    }
    namespace bt = boost::posix_time;
    const std::locale formats[] = {
        std::locale(std::locale::classic(), new bt::time_input_facet("%Y-%m-%d %H:%M:%S")),
        std::locale(std::locale::classic(), new bt::time_input_facet("%Y/%m/%d %H:%M:%S")),
        std::locale(std::locale::classic(), new bt::time_input_facet("%d.%m.%Y %H:%M:%S")),
        std::locale(std::locale::classic(), new bt::time_input_facet("%Y-%m-%d"))};
    const int formats_n = sizeof(formats) / sizeof(formats[0]);
    bt::ptime pt;
    for (int i = 0; i < formats_n; ++i)
    {
        std::istringstream is(off);
        is.imbue(formats[i]);
        is >> pt;
        if (pt != bt::ptime())
            break;
    }
    bt::ptime timet_start(boost::gregorian::date(1970, 1, 1));
    bt::time_duration diff = pt - timet_start;
    return (diff.ticks() / bt::time_duration::rep_type::ticks_per_second) * 1000;
}
/*
void ChaosController::dumpHistoryToTgz(const std::string& fname,const std::string& start,const std::string& end,int channel,std::string tagname){
    std::ofstream f(fname);
    std::string mpath=path;
    mpath.replace(mpath.begin(),mpath.end(),"/","_");
    uint64_t start_ts = offsetToTimestamp(start);
    uint64_t end_ts = offsetToTimestamp(end);
    chaos::common::io::QueryCursor *query_cursor = NULL;
    ChaosStringSet tagv;
    if(tagname.size()>0){
        tagv.insert(tagname);
    }
    controller->executeTimeIntervallQuery((chaos::metadata_service_client::node_controller::DatasetDomain)channel, start_ts, end_ts,tagv, &query_cursor);
    while ((query_cursor->hasNext())){
            ChaosSharedPtr<CDataWrapper> q_result(query_cursor->next());
            boost::shared_ptr<CDataWrapper> cd = normalizeToJson(q_result.get(), binaryToTranslate);
            
    }

}
*/


int ChaosController::setSchedule(uint64_t us, const std::string& cuname){
    int ret;
    std::string name=(cuname=="")?path:cuname;
    chaos::common::property::PropertyGroup pg(chaos::ControlUnitPropertyKey::P_GROUP_NAME);
    pg.addProperty(chaos::ControlUnitDatapackSystemKey::THREAD_SCHEDULE_DELAY, CDataVariant(static_cast<uint64_t>(us)));
    DBGET<<"["<<name<<"] set schedule to:"<<us<<" us";
    EXECUTE_CHAOS_RET_API(ret,chaos::metadata_service_client::api_proxy::node::UpdateProperty,MDS_TIMEOUT,name,pg);
    setQuantum(us);
    return ret;
}

int ChaosController::setAttributeToValue(const char *attributeName, const char *attributeValue, const std::string& cuname){
    std::string name=(cuname=="")?path:cuname;
    int ret;
    DBGET<<"["<<name<<"] Set attribute '"<<attributeName<<"'='"<<attributeValue<<"'";
   ChaosSharedPtr<chaos::metadata_service_client::api_proxy::control_unit::InputDatasetAttributeChangeValue>  att_obj(new chaos::metadata_service_client::api_proxy::control_unit::InputDatasetAttributeChangeValue(attributeName,attributeValue));
   std::vector<ChaosSharedPtr<chaos::metadata_service_client::api_proxy::control_unit::InputDatasetAttributeChangeValue> > vc;
   vc.push_back(att_obj);
   ChaosSharedPtr<chaos::metadata_service_client::api_proxy::control_unit::ControlUnitInputDatasetChangeSet> tmp(new chaos::metadata_service_client::api_proxy::control_unit::ControlUnitInputDatasetChangeSet(name,vc));

   std::vector<ChaosSharedPtr<chaos::metadata_service_client::api_proxy::control_unit::ControlUnitInputDatasetChangeSet> > change_set;
    change_set.push_back(tmp);
    EXECUTE_CHAOS_RET_API(ret,api_proxy::control_unit::SetInputDatasetAttributeValues, MDS_TIMEOUT, change_set);

    return ret;
}
int ChaosController::initDevice(const std::string& dev){
    int ret;
    std::string name=(dev=="")?path:dev;
    
    EXECUTE_CHAOS_RET_API(ret,api_proxy::control_unit::InitDeinit, MDS_TIMEOUT, name, true);
    return ret;

}
int ChaosController::stopDevice(const std::string& dev){
    int ret;
    std::string name=(dev=="")?path:dev;

    EXECUTE_CHAOS_RET_API(ret,api_proxy::control_unit::StartStop, MDS_TIMEOUT, name, false);
    return ret;
}
int ChaosController::startDevice(const std::string& dev){
    int ret;
    std::string name=(dev=="")?path:dev;

    EXECUTE_CHAOS_RET_API(ret,api_proxy::control_unit::StartStop, MDS_TIMEOUT, name, true);
    return ret;
}
int ChaosController::deinitDevice(const std::string& dev){
    int ret;
    std::string name=(dev=="")?path:dev;

    EXECUTE_CHAOS_RET_API(ret,api_proxy::control_unit::InitDeinit, MDS_TIMEOUT, name, false);
    return ret;
}
int ChaosController::loadDevice(const std::string& dev){
    int ret;
    std::string name=(dev=="")?path:dev;

    EXECUTE_CHAOS_RET_API(ret,api_proxy::unit_server::LoadUnloadControlUnit, MDS_TIMEOUT, name, true);
    return ret;
}
int ChaosController::unloadDevice(const std::string& dev){
    int ret;
    std::string name=(dev=="")?path:dev;

    EXECUTE_CHAOS_RET_API(ret,api_proxy::unit_server::LoadUnloadControlUnit, MDS_TIMEOUT, name, false);
    return ret;
}
int32_t ChaosController::queryHistory(const std::string &start, const std::string &end, int channel, std::vector<boost::shared_ptr<CDataWrapper> > &res,  const ChaosStringSet& projection,int page){
    std::vector<std::string> tags;
    return queryHistory(start,end,0,0,tags,channel,res,projection, page);
}

void ChaosController::executeTimeIntervalQuery(DatasetDomain domain,
                                             uint64_t      start_ts,
                                             uint64_t      end_ts,
                                             QueryCursor** query_cursor,
                                            const std::string&name,

                                             uint32_t      page) {
  executeTimeIntervalQuery(
      domain,
      start_ts,
      end_ts,
      ChaosStringSet(),
      query_cursor,
        ChaosStringSet(),

      name,
      page);
}
void ChaosController::executeTimeIntervalQuery(DatasetDomain domain,
                                             uint64_t start_ts,
                                             uint64_t end_ts,
                                             const ChaosStringSet& meta_tags,
                                             QueryCursor **query_cursor,
                                             const ChaosStringSet& projection,

                                             const std::string&name,

                                             uint32_t page) {
    if((domain>=0) && (domain<=DPCK_LAST_DATASET_INDEX)){
        std::string n=(name=="")?path:name;
        std::string lkey = n + chaos::datasetTypeToPostfix(domain);

        *query_cursor = live_driver->performQuery(lkey,
                                                       start_ts,
                                                       end_ts,
                                                       meta_tags,
                                                       projection,
                                                       page);
    }
}



void ChaosController::executeTimeIntervalQuery(const DatasetDomain domain,
                                            const uint64_t start_ts,
                                            const uint64_t end_ts,
                                            const uint64_t seqid,
                                            const uint64_t runid,
                                            const ChaosStringSet& meta_tags,
                                            chaos::common::io::QueryCursor **query_cursor,
                                            const ChaosStringSet& projection,

                                            const std::string&name,

                                            const uint32_t page_len){
    if((domain>=0) && (domain<=DPCK_LAST_DATASET_INDEX)){
        std::string n=(name=="")?path:name;
        std::string lkey = n + chaos::datasetTypeToPostfix(domain);

        *query_cursor = live_driver->performQuery(lkey,
                                                       start_ts,
                                                       end_ts,
                                                       seqid,
                                                       runid,
                                                       meta_tags,
                                                       projection,
                                                       page_len);
    }
    
}


int32_t ChaosController::queryHistory(const std::string& start,const std::string& end, uint64_t runid,uint64_t seqid,const std::vector<std::string>& tags, int channel,std::vector<boost::shared_ptr<chaos::common::data::CDataWrapper> >&res, const ChaosStringSet& projection,int page){
    uint64_t start_ts = offsetToTimestamp(start);
    uint64_t end_ts = offsetToTimestamp(end);
    int32_t ret = 0, err = 0;
    chaos::common::io::QueryCursor *query_cursor = NULL;
    if (page == 0)
    {
        std::set<std::string> tagsv(tags.begin(),tags.end());
        executeTimeIntervalQuery((chaos::metadata_service_client::node_controller::DatasetDomain)channel, start_ts, end_ts,seqid,runid,tagsv, &query_cursor,projection);
        if (query_cursor == NULL)
        {
            CTRLERR_ << " error during intervall query, no cursor available";
            return -1;
        }
        while ((query_cursor->hasNext()))
        {

            ChaosSharedPtr<CDataWrapper> q_result(query_cursor->next());
            boost::shared_ptr<CDataWrapper> cd = normalizeToJson(q_result.get(), binaryToTranslate);
            res.push_back(cd);
        }
    }
    else
    {
        int cnt = 0;
        executeTimeIntervalQuery((chaos::metadata_service_client::node_controller::DatasetDomain)channel, start_ts, end_ts,seqid,runid, ChaosStringSet(),&query_cursor, projection,path,page);
        if (query_cursor == NULL)
        {
            CTRLERR_ << " error during intervall query, no cursor available";
            return -1;
        }
        if (query_cursor->hasNext())
        {
            uint32_t elems = query_cursor->size();
            while (cnt < elems)
            {
                ChaosSharedPtr<CDataWrapper> q_result(query_cursor->next());
                boost::shared_ptr<CDataWrapper> cd = normalizeToJson(q_result.get(), binaryToTranslate);
                res.push_back(cd);
                cnt++;
            }
        }

        if ((query_cursor->hasNext()))
        {
            qc_t q_nfo;
            q_nfo.qt = reqtime / 1000;
            q_nfo.qc = query_cursor;
            query_cursor_map[++queryuid] = q_nfo;
            return queryuid;
        }
    }
    if (query_cursor && (err = query_cursor->getError()))
    {
        releaseQuery(query_cursor);
        return -abs(err);
    }
    else
    {
        releaseQuery(query_cursor);
    }
    return 0;
}
bool ChaosController::queryHasNext(int32_t uid)
{
    chaos::common::io::QueryCursor *query_cursor = NULL;
    if (query_cursor_map.find(uid) != query_cursor_map.end())
    {
        query_cursor = query_cursor_map[uid].qc;
        query_cursor_map[uid].qt = reqtime / 1000;
        if (query_cursor)
        {
            return query_cursor->hasNext();
        }
    }
    return false;
}

int32_t ChaosController::queryNext(int32_t uid, std::vector<boost::shared_ptr<CDataWrapper> >  &res)
{
    chaos::common::io::QueryCursor *query_cursor = NULL;
    int cnt, err;
    if (query_cursor_map.find(uid) != query_cursor_map.end())
    {
        query_cursor = query_cursor_map[uid].qc;
        query_cursor_map[uid].qt = reqtime / 1000;
        if (query_cursor)
        {
            cnt = 0;
            uint32_t page = query_cursor->getPageLen();
            if (query_cursor->hasNext())
            {
                ChaosSharedPtr<CDataWrapper> q_result(query_cursor->next());
                if ((err = query_cursor->getError()))
                {
                    query_cursor_map.erase(query_cursor_map.find(uid));
                    releaseQuery(query_cursor);
                    return -abs(err);
                }
                else
                {
                    boost::shared_ptr<CDataWrapper> cd = normalizeToJson(q_result.get(), binaryToTranslate);
                    res.push_back(cd);
                    if (!query_cursor->hasNext())
                    {
                        query_cursor_map.erase(query_cursor_map.find(uid));
                        releaseQuery(query_cursor);
                        return 0;
                    }
                    return uid;
                }
            }
            else
            {
                query_cursor_map.erase(query_cursor_map.find(uid));
                releaseQuery(query_cursor);
                return 0;
            }
        }
    }
    CTRLERR_ << " no cursor available with uid " << uid;

    return -1;
}
chaos::common::data::CDWUniquePtr ChaosController::getNodeDesc(const std::string& name,int&ret){
  chaos::common::data::CDWUniquePtr r;
    EXECUTE_CHAOS_RET_API(ret,api_proxy::node::GetNodeDescription, MDS_TIMEOUT, name);
    if(ret == 0){
        r=apires->detachResult();
        /*if(r->hasKey(chaos::NodeDefinitionKey::NODE_TYPE)&&(r->getStringValue(chaos::NodeDefinitionKey::NODE_TYPE)==chaos::NodeType::NODE_TYPE_UNIT_SERVER)){
            EXECUTE_CHAOS_RET_API(ret,api_proxy::agent::GetAgentForNode, MDS_TIMEOUT, name);
            if(ret==0){
                  chaos::common::data::CDWUniquePtr r1=apires->detachResult();
                  if(r1->hasKey(chaos::NodeDefinitionKey::NODE_UNIQUE_ID)){
                      if(!r->hasKey(chaos::NodeDefinitionKey::NODE_PARENT)){
                        r->addStringValue(chaos::NodeDefinitionKey::NODE_PARENT,r1->getStringValue(chaos::NodeDefinitionKey::NODE_UNIQUE_ID));

                      }
                  }

            } else {
                std::stringstream ss;                                                                                             
                ss << " error in :" << __FUNCTION__ << "|" << __LINE__ << "| GetAgentForNode :" << apires->getErrorMessage();  
                 bundle_state.append_error(ss.str());                                                                              
                    
            }

        }*/
    } else {
        std::stringstream ss;                                                                                             
        ss << " error in :" << __FUNCTION__ << "|" << __LINE__ << "| GetNodeDescription :" << apires->getErrorMessage();  
        bundle_state.append_error(ss.str());                                                                              
                    
    }
    return r;
}

int ChaosController::createNewSnapshot(const std::string& snapshot_tag,
                                    const std::vector<std::string>& other_snapped_device) {
    
 
    std::vector<std::string> device_id_in_snap = other_snapped_device;
    device_id_in_snap.push_back(path);
    return mdsChannel->createNewSnapshot(snapshot_tag,
                                         device_id_in_snap);
}
int ChaosController::deleteSnapshot(const std::string& snapshot_tag) {

        return mdsChannel->deleteSnapshot(snapshot_tag);
}
int ChaosController::getSnapshotList(ChaosStringVector& snapshot_list) {
    CHAOS_ASSERT(mdsChannel)
    return mdsChannel->searchSnapshotForNode(path,
                                             snapshot_list,
                                             MDS_TIMEOUT);
}
int ChaosController::restoreDeviceToTag(const std::string& restore_tag) {
    return mdsChannel->restoreSnapshot(restore_tag, MDS_TIMEOUT);

}
int ChaosController::recoverDeviceFromError(const std::string& cu) {
    int ret;
    std::vector<std::string> names;
    names.push_back((cu==""?path:cu));
    EXECUTE_CHAOS_RET_API(ret,chaos::metadata_service_client::api_proxy::control_unit::RecoverError,MDS_TIMEOUT,names);

    return ret;
}
CDataWrapper ChaosController::getSnapshotDataset(const std::string&snapname,const std::string& cuname){
   CDataWrapper res;
   mdsChannel->loadSnapshotNodeDataset(snapname, cuname, res, MDS_TIMEOUT);
   return res; 
}
std::vector<std::string> ChaosController::searchAlive(const std::string& name,const std::string& what){
            ChaosStringVector node_found;
            chaos::NodeType::NodeSearchType node_type=human2NodeType(what);

            mdsChannel->searchNode(name,node_type,true,0,MAX_QUERY_ELEMENTS,node_found,MDS_TIMEOUT);
            return node_found;
}
ChaosController::chaos_controller_error_t ChaosController::get(const std::string &cmd, char *args, int timeout, int prio, int sched, int submission_mode, int channel, std::string &json_buf)
{
    int err;
    last_access = reqtime;
    reqtime = chaos::common::utility::TimingUtil::getTimeStampInMicroseconds();
    naccess++;
    bundle_state.reset();
    bundle_state.status(state);
    DBGET << "cmd:" << cmd << " args:" << args << " last access:" << (reqtime - last_access) * 1.0 / 1000.0 << " ms ago"
          << " timeo:" << timeo << " ptr:0x" << std::hex << this << std::dec;
    json_buf = "[]";

    try
    {
        // global commands
        int execute_chaos_api_error=0;
        if (cmd == "search")
        {
            std::stringstream res;
            PARSE_QUERY_PARMS(args, false, false);
            if (what == "")
            {
                what = "cu";
            }
            DBGET << "searching what " << what;
            ChaosStringVector node_found;
            if (what == "cu" || what == "us" || what == "agent" || what=="mds" || what=="server" || what=="root"|| what=="webui" || what=="variable" || what=="tag")
            {
                json_buf = "[]";
                chaos::NodeType::NodeSearchType node_type=human2NodeType(what);
                uint32_t maxpage=MAX_QUERY_ELEMENTS;
                uint32_t page_start=0;
                bool pageaccess=false;
                uint32_t npages=0;
                std::string impl;
                if(p.hasKey("pagelen")){
                    maxpage=p.getInt32Value("pagelen");
                }
                if(p.hasKey("pagestart")){
                    page_start=p.getInt32Value("pagestart");
                }
                if(p.hasKey("pagestart")&&p.hasKey("pagelen")){
                    pageaccess=true;
                }
                if(p.hasKey("impl")){
                    impl=p.getStringValue("impl");
                }
                if ((names.get()) && names->size())
                {
                    DBGET << "list nodes of type:" << node_type << "(" << what << ")";

                    for (int idx = 0; idx < names->size(); idx++)
                    {
                        ChaosStringVector node_tmp;

                        const std::string domain = names->getStringElementAtIndex(idx);
                        if(pageaccess){
                            if (mdsChannel->searchNode(domain, node_type, alive, page_start, maxpage, npages,node_tmp, MDS_TIMEOUT,impl) == 0){
                                node_found.insert(node_found.end(), node_tmp.begin(), node_tmp.end());
                            }
                        } else {
                            if (mdsChannel->searchNode(domain, node_type, alive, 0, maxpage, node_tmp, MDS_TIMEOUT,impl) == 0){
                                node_found.insert(node_found.end(), node_tmp.begin(), node_tmp.end());
                            }
                        }
                    }
                    if(pageaccess){
                        std::stringstream ss;
                        ss<<"{\"pages\":"<<npages<<",\"list\":"<<vector2Json(node_found)<<"}";
                        json_buf=ss.str();
                    } else {
                        json_buf = vector2Json(node_found);
                    }
                    CALC_EXEC_TIME;
                    return CHAOS_DEV_OK;
                }
                else
                {
                    int err;
                    DBGET << "searching node \"" << name << "\" type:" << node_type << " (" << what << ")";
                    if(pageaccess){
                        if ((err = mdsChannel->searchNode(name,
                                                        node_type,
                                                        alive,
                                                        page_start,
                                                        maxpage,
                                                        npages,
                                                        node_found,
                                                        MDS_TIMEOUT,impl)) == 0)
                        {
                        std::stringstream ss;
                        ss<<"{\"pages\":"<<npages<<",\"list\":"<<vector2Json(node_found)<<"}";
                       
                        json_buf=ss.str();
                            CALC_EXEC_TIME;
                            return CHAOS_DEV_OK;
                        }else{
                            serr << "searching node: \"" << name << "\" err:" << err;
                        }
                    } else {

                        if ((err = mdsChannel->searchNode(name,
                                                        node_type,
                                                        alive,
                                                        0,
                                                        maxpage,
                                                        node_found,
                                                        MDS_TIMEOUT,impl)) == 0)
                        {

                            json_buf = vector2Json(node_found);
                            CALC_EXEC_TIME;
                            return CHAOS_DEV_OK;
                        }else{
                            serr << "searching node: \"" << name << "\" err:" << err;
                        }
                    }
                }
            }
            else if (what == "zone")
            {
                json_buf = "[]";
                DBGET << "searching ZONE";

                ChaosStringVector dev_zone;
                if (mdsChannel->searchNode(name,
                                           chaos::NodeType::NodeSearchType::node_type_cu,
                                           false,
                                           0,
                                           MAX_QUERY_ELEMENTS,
                                           node_found,
                                           MDS_TIMEOUT) == 0)
                {
                    parseClassZone(node_found);
                    std::map<std::string, std::string>::iterator c;
                    for (c = zone_to_cuname.begin(); c != zone_to_cuname.end(); c++)
                    {
                        dev_zone.push_back(c->first);
                    }

                    json_buf = vector2Json(dev_zone);
                    CALC_EXEC_TIME;
                    return CHAOS_DEV_OK;
                }
                else
                {
                    serr << "searching zone:" << name;
                }
            }
            else if (what == "class")
            {
                json_buf = "[]";
                ChaosStringVector dev_class;

                if (names.get() && names->size())
                {
                    DBGET << "searching CLASS LIST";

                    for (int idx = 0; idx < names->size(); idx++)
                    {
                        ChaosStringVector node_tmp;
                        const std::string domain = names->getStringElementAtIndex(idx);
                        if (mdsChannel->searchNode(domain, chaos::NodeType::NodeSearchType::node_type_cu, false, 0, MAX_QUERY_ELEMENTS, node_tmp, MDS_TIMEOUT) == 0)
                        {
                            node_found.insert(node_found.end(), node_tmp.begin(), node_tmp.end());
                        }
                        else
                        {
                            serr << "searching class:" << domain;
                            bundle_state.append_error(serr.str());
                            json_buf = bundle_state.getData()->getCompliantJSONString();
                            return CHAOS_DEV_CMD;
                        }
                    }
                    parseClassZone(node_found);
                    std::map<std::string, std::string>::iterator c;
                    for (c = class_to_cuname.begin(); c != class_to_cuname.end(); c++)
                    {
                        dev_class.push_back(c->first);
                    }

                    json_buf = vector2Json(dev_class);
                    CALC_EXEC_TIME;
                    return CHAOS_DEV_OK;
                }
                else
                {
                    DBGET << "searching CLASS inside:" << name;

                    if (mdsChannel->searchNode(name,
                                               chaos::NodeType::NodeSearchType::node_type_cu,
                                               false,
                                               0,
                                               MAX_QUERY_ELEMENTS,
                                               node_found,
                                               MDS_TIMEOUT) == 0)
                    {
                        parseClassZone(node_found);
                        std::map<std::string, std::string>::iterator c;
                        for (c = class_to_cuname.begin(); c != class_to_cuname.end(); c++)
                        {
                            dev_class.push_back(c->first);
                        }

                        json_buf = vector2Json(dev_class);
                        CALC_EXEC_TIME;
                        return CHAOS_DEV_OK;
                    }
                    else
                    {
                        serr << "searching node:" << name;
                    }
                }
                bundle_state.append_error(serr.str());
                json_buf = bundle_state.getData()->getCompliantJSONString();
                return CHAOS_DEV_CMD;
            }
            else if (what == "snapshots")
            {
                DBGET << "searching SNAPSHOTS";

                std::map<uint64_t, std::string> snap_l;
                if (mdsChannel->searchSnapshot(name, snap_l, MDS_TIMEOUT) == 0)
                {
                    json_buf = map2Json(snap_l);
                    CALC_EXEC_TIME;
                    return CHAOS_DEV_OK;
                }
                else
                {
                    serr << " searching snapshot:" << name;
                }
            }
            else if (what == "insnapshot")
            {
                json_buf = "[]";
                DBGET << "searching SNAPSHOT of the CU:" << name;

                if (mdsChannel->searchNodeForSnapshot(name, node_found, MDS_TIMEOUT) == 0)
                {

                    json_buf = vector2Json(node_found);
                    DBGET << "OK searchNodeForSnapshot snap:" << json_buf;

                    CALC_EXEC_TIME;
                    return CHAOS_DEV_OK;
                }
                else
                {
                    DBGET << "ERRORE searchNodeForSnapshot snap:" << name;
                    serr << " searching insnapshot:" << name;
                }
            }
            else if (what == "snapshotsof")
            {
                json_buf = "[]";
                DBGET << "searching CU within SNAPSHOT:" << name;
                std::map<uint64_t, std::string> snap_l;
                if (getSnapshotsofCU(name, snap_l) == 0)
                {
                    json_buf = map2Json(snap_l);
                    CALC_EXEC_TIME;
                    return CHAOS_DEV_OK;
                }
                else
                {
                    DBGET << "ERRORE searchSnapshotForNode ";
                    serr << " searching snapshotsof:" << name;
                }
            }
            else if (what == "desc")
            {
                DBGET << "searching DESC of the CU:" << name;

                json_buf = "[]";
                if (name.size() > 0)
                {
                    CDWUniquePtr out;
                    if (mdsChannel->getFullNodeDescription(name, out, MDS_TIMEOUT) == 0)
                    {
                        json_buf = "{}";
                        if (out.get())
                        {
                            json_buf = out->getCompliantJSONString();
                            CALC_EXEC_TIME;
                        }
                        return CHAOS_DEV_OK;
                    }
                }
                else
                {
                    serr << " missing name";
                }
            }
            else if (what == "script")
            {
                EXECUTE_CHAOS_API(chaos::metadata_service_client::api_proxy::script::SearchScript, MDS_TIMEOUT, name);
                return (execute_chaos_api_error==0)?CHAOS_DEV_OK:CHAOS_DEV_CMD;
            }
            else
            {

                serr << "unknown 'search' arg:" << args;
            }
            bundle_state.append_error(serr.str());
            json_buf = bundle_state.getData()->getCompliantJSONString();
            return CHAOS_DEV_CMD;
        }
        else if (cmd == "snapshot")
        {
            std::stringstream res;
            ChaosStringVector node_found;
            PARSE_QUERY_PARMS(args, false, true);

            DBGET << "snapshot what " << what;
            if (what == "burst")
            {
                if (json_value.get() != NULL)
                {
                    CALL_CHAOS_API(api_proxy::control_unit::SendStorageBurst, MDS_TIMEOUT, json_value);

                    json_buf = "{}";
                    res << json_buf;
                    CALC_EXEC_TIME;
                    return CHAOS_DEV_OK;
                }
                serr << "error performing snapshot burst query" << name;
                bundle_state.append_error(serr.str());
                json_buf = bundle_state.getData()->getCompliantJSONString();
                return CHAOS_DEV_CMD;
            }

            if (node_list.get() && node_list->size())
            {
                for (int idx = 0; idx < node_list->size(); idx++)
                {
                    const std::string domain = node_list->getStringElementAtIndex(idx);
                    node_found.push_back(domain);
                    //DBGET << "adding \"" << domain << "\" to snapshot name:\"" << name << "\"";
                }
            }
            if (what == "create")
            {
                if (node_found.empty())
                {
                    serr << "missing \"node_list\" json vector";
                }
                else
                {
                    if (mdsChannel->createNewSnapshot(name, node_found, MDS_TIMEOUT) == 0)
                    {
                        DBGET << "Created snapshot name:\"" << name << "\"";

                        json_buf = vector2Json(node_found);
                        CALC_EXEC_TIME;
                        return CHAOS_DEV_OK;
                    }
                    else
                    {
                        serr << "error creating snapshot:" << name;
                    }
                }
                bundle_state.append_error(serr.str());
                json_buf = bundle_state.getData()->getCompliantJSONString();
                return CHAOS_DEV_CMD;
            }
            else if (what == "delete")
            {
                if (mdsChannel->deleteSnapshot(name, MDS_TIMEOUT) == 0)
                {
                    DBGET << "Deleted snapshot name:\"" << name << "\"";

                    CALC_EXEC_TIME;
                    return CHAOS_DEV_OK;
                }
                else
                {
                    serr << "error deleting snapshot:" << name;
                }
                bundle_state.append_error(serr.str());
                json_buf = bundle_state.getData()->getCompliantJSONString();
                return CHAOS_DEV_CMD;
            }
            else if (what == "restore")
            {
                if (mdsChannel->restoreSnapshot(name, MDS_TIMEOUT) == 0)
                {
                    DBGET << "Restore snapshot name:\"" << name << "\"";

                    CALC_EXEC_TIME;
                    return CHAOS_DEV_OK;
                }
                else
                {
                    serr << "error restoring snapshot:" << name;
                }
                bundle_state.append_error(serr.str());
                json_buf = bundle_state.getData()->getCompliantJSONString();
                return CHAOS_DEV_CMD;
            }
            else if (what == "load")
            {
                if (node_found.empty() && (mdsChannel->searchNodeForSnapshot(name, node_found, MDS_TIMEOUT) != 0))
                {

                    serr << "missing \"node_list\" json vector";
                }
                else
                {
                    std::stringstream sres;
                    sres << "[";

                    for (ChaosStringVector::iterator i = node_found.begin(); i != node_found.end(); i++)
                    {
                        CDataWrapper res;
                        if (mdsChannel->loadSnapshotNodeDataset(name, *i, res, MDS_TIMEOUT) == 0)
                        {
                            DBGET << "load snapshot name:\"" << name << "\": CU:" << *i;
                            if ((i + 1) == node_found.end())
                            {
                                sres << res.getCompliantJSONString();
                            }
                            else
                            {
                                sres << res.getCompliantJSONString() << ",";
                            }
                        }
                    }
                    sres << "]";
                    json_buf = sres.str();
                    CALC_EXEC_TIME
                    return CHAOS_DEV_OK;
                }
                bundle_state.append_error(serr.str());
                json_buf = bundle_state.getData()->getCompliantJSONString();
                return CHAOS_DEV_CMD;
            }
            else if (what == "set")
            {
                if (json_value.get() == NULL)
                {

                    serr << "missing \"value\"JSON_CU_DATASET dataset";
                }
                else
                {
                    api_proxy::service::VectorDatasetValue datasets;
                    std::string uid;
                    for (int cnt = 0; cnt <= DPCK_LAST_DATASET_INDEX; cnt++)
                    {
                        DBGET << "checking for\"" << chaos::datasetTypeToHuman(cnt) << "\" in:" << json_value->getCompliantJSONString();

                        if (json_value->hasKey(chaos::datasetTypeToHuman(cnt)) && json_value->isCDataWrapperValue(chaos::datasetTypeToHuman(cnt)))
                        {
                            DBGET << "Set Snapshot found \"" << chaos::datasetTypeToHuman(cnt) << "\"";
                            ChaosUniquePtr<CDataWrapper> ds = json_value->getCSDataValue(chaos::datasetTypeToHuman(cnt));
                            ds->addInt32Value(chaos::DataPackCommonKey::DPCK_DATASET_TYPE,cnt);
                            if (ds->hasKey(chaos::NodeDefinitionKey::NODE_UNIQUE_ID) && ds->isStringValue(chaos::NodeDefinitionKey::NODE_UNIQUE_ID))
                            {
                                uid = ds->getStringValue(chaos::NodeDefinitionKey::NODE_UNIQUE_ID);
                                DBGET << "set snapshot name:\"" << name << "\": dataset:" << chaos::datasetTypeToHuman(cnt);
                                std::string dataset_name = uid + std::string(chaos::datasetTypeToPostfix(cnt));
                                datasets.push_back(api_proxy::service::SetSnapshotDatasetsForNode::createDatasetValue(dataset_name, *ds));
                            }
                        }
                    }
                    if (!datasets.empty())
                    {
                        EXECUTE_CHAOS_API(api_proxy::service::SetSnapshotDatasetsForNode, MDS_TIMEOUT, name, uid, datasets);
                    }
                    CALC_EXEC_TIME
                    return (execute_chaos_api_error==0)?CHAOS_DEV_OK:CHAOS_DEV_CMD;
;
                }
                bundle_state.append_error(serr.str());
                json_buf = bundle_state.getData()->getCompliantJSONString();
                return CHAOS_DEV_CMD;
            }
            else
            {

                serr << "unknown 'snapshot' arg:" << args;
            }
            bundle_state.append_error(serr.str());
            json_buf = bundle_state.getData()->getCompliantJSONString();
            return CHAOS_DEV_CMD;
        }
        else if (cmd == "variable")
        {
            std::stringstream res;
            PARSE_QUERY_PARMS(args, true, true);
            DBGET << "variable what " << what;

            ChaosStringVector node_found;
            if (what == "set")
            {
                if (json_value.get())
                {
                    if (mdsChannel->setVariable(name, *json_value.get(), MDS_TIMEOUT) == 0)
                    {
                        DBGET << "Save variable name:\"" << name << "\":" << json_value->getCompliantJSONString();
                        json_buf = json_value->getCompliantJSONString();
                        CALC_EXEC_TIME;
                        return CHAOS_DEV_OK;
                    }
                    else
                    {
                        serr << "error setting variable '" << name << "' to:" << json_value->getCompliantJSONString();
                        bundle_state.append_error(serr.str());
                        json_buf = bundle_state.getData()->getCompliantJSONString();
                        return CHAOS_DEV_CMD;
                    }
                }
                else
                {
                    serr << "no parameters given" << cmd;

                    bundle_state.append_error(serr.str());
                    json_buf = bundle_state.getData()->getCompliantJSONString();
                    return CHAOS_DEV_CMD;
                }
            }
            else if (what == "get")
            {
                CDWUniquePtr res;
                if ((mdsChannel->getVariable(name, res, MDS_TIMEOUT) == 0))
                {
                    if (res.get() == NULL)
                    {
                        json_buf = "{}";
                        DBGET << "no variable name:\"" << name << "\":";
                    }
                    else
                    {
                        json_buf = res->getCompliantJSONString();
                        DBGET << "Retrieved  variable name:\"" << name << "\":" << json_buf;
                    }
                    return CHAOS_DEV_OK;
                    
                }
                else
                {
                    serr << "no variable found with name :\"" << name << "\"";

                    bundle_state.append_error(serr.str());

                    
                    json_buf = bundle_state.getData()->getCompliantJSONString();
                    return CHAOS_DEV_CMD;
                }
            }
            else if (what == "del")
            {
                if (mdsChannel->removeVariable(name, MDS_TIMEOUT) == 0)
                {
                    DBGET << "Removed  variable name:\"" << name << "\":" << json_buf;
                    json_buf = "{}";
                    return CHAOS_DEV_OK;
                }
                else
                {
                    serr << "no variable found with name :\"" << name << "\"";

                    bundle_state.append_error(serr.str());
                    json_buf = bundle_state.getData()->getCompliantJSONString();
                    return CHAOS_DEV_CMD;
                }
            }
            else if (what == "search")
            {
                ChaosStringVector node_found;
                if (mdsChannel->searchVariable(name, node_found, MDS_TIMEOUT) == 0)
                {
                    json_buf = vector2Json(node_found);
                    CALC_EXEC_TIME;
                    DBGET << "list  variable name:\"" << name << "\":" << json_buf;
                    return CHAOS_DEV_OK;
                }
                else
                {
                    serr << "no variable found with name :\"" << name << "\"";

                    bundle_state.append_error(serr.str());
                    json_buf = bundle_state.getData()->getCompliantJSONString();
                    return CHAOS_DEV_CMD;
                }
            }
        }
        else if (cmd == "node")
        {
            // multiple nodes in names
            std::stringstream res;
            int idx;
            chaos_controller_error_t ret = CHAOS_DEV_OK;
            PARSE_QUERY_PARMS(args, true, true);
            if (node_type.empty()&&(what!="health")&&(what!="command"))
            {
                serr << cmd << " parameters must specify 'type'";

                bundle_state.append_error(serr.str());
                json_buf = bundle_state.getData()->getCompliantJSONString();
                return CHAOS_DEV_CMD;
            }
            if (((names.get()) && ((names->size()) > 0)) && (what == "health"))
            {

                // multi get!
                res << "[";
                chaos::common::data::VectorCDWShrdPtr dat = getLiveChannel(names.get());
                for (chaos::common::data::VectorCDWShrdPtr::iterator i = dat.begin(); i != dat.end(); i++)
                {
                    res << "{\"health\":" << (*i)->getCompliantJSONString() << "}";
                    if ((i + 1) != dat.end())
                    {
                        res << ",";
                    }
                }
                res << "]";
                json_buf = res.str();
                return CHAOS_DEV_OK;
            }
            idx = 0;
            if ((names.get()) && ((names->size()) > 0))
            {
                res << "[";
            }
            do
            {
                if ((names.get()) && ((names->size()) > 0))
                {
                    name = names->getStringElementAtIndex(idx);
                }
                if(what == "command"){
                     int err=0;
                     std::string domain=name; //cu
                     std::string action;
                        chaos::common::data::CDWUniquePtr p(new CDataWrapper());
                        p->addStringValue(chaos::NodeDefinitionKey::NODE_UNIQUE_ID, name);
                        if(parent.size()){
                            p->addStringValue(chaos::NodeDefinitionKey::NODE_PARENT,parent);
                        }
                        if(json_value.get()){
                            json_value->copyAllTo(*p);
                            if(json_value->hasKey(chaos::RpcActionDefinitionKey::CS_CMDM_ACTION_DOMAIN)){
                                domain=json_value->getStringValue(chaos::RpcActionDefinitionKey::CS_CMDM_ACTION_DOMAIN);
                            }
                            if(json_value->hasKey(chaos::RpcActionDefinitionKey::CS_CMDM_ACTION_NAME)){
                                action=json_value->getStringValue(chaos::RpcActionDefinitionKey::CS_CMDM_ACTION_NAME);
                            }
                        }
                        
                        chaos::common::data::CDWUniquePtr msg;
                        if(domain==chaos::NodeDomainAndActionRPC::RPC_DOMAIN){
                            msg=executeAPI(chaos::NodeDomainAndActionRPC::RPC_DOMAIN,"NodeGenericCommand",p,err);
                        } else {
                            msg=sendRPCMsg(name,domain,action,p);
                            if(msg.get()==NULL){
                                err=-1;
                            }
                        }
                        if(err!=0){
                            execute_chaos_api_error++;                                                                                          
                            std::stringstream ss;                                                                                             
                            ss << " error in :" << __FUNCTION__ << "|" << __LINE__ ;;   
                        bundle_state.append_error(ss.str());                                                                              
                        json_buf = bundle_state.getData()->getCompliantJSONString();                                                      
        

                        } else {
                            json_buf=(msg.get())?msg->getCompliantJSONString():"{}";
                        }
                        //EXECUTE_CHAOS_API(api_proxy::unit_server::DeleteUS, MDS_TIMEOUT, name);
                        res << json_buf;

                } else if (what == "health")
                {
                    ChaosSharedPtr<chaos::common::data::CDataWrapper> dt;
                    dt = getLiveChannel(name);
                    if (dt.get())
                    {
                        res << "{\"health\":" << dt->getCompliantJSONString() << "}";
                    }
                    else
                    {
                        json_buf = "{}";
                        serr << cmd << " API error command format";
                        bundle_state.append_error(serr.str());
                        json_buf = bundle_state.getData()->getCompliantJSONString();
                        res << json_buf;
                        ret = CHAOS_DEV_CMD;
                    }
                }
                else if (what == "desc")
                {
                    int rets;
                    chaos::common::data::CDWUniquePtr r=getNodeDesc(name,rets);

                    if(rets==0){
                        res<<r->getCompliantJSONString();

                    } else {
                    json_buf = bundle_state.getData()->getCompliantJSONString();                                                      
                    execute_chaos_api_error++; 
                    }

                } else if (what == "shutdown"){
                        chaos::common::data::CDWUniquePtr infos(new CDataWrapper());
                        infos->addBoolValue("kill",true);
                        sendRPCMsg(name,chaos::NodeDomainAndActionRPC::ACTION_NODE_SHUTDOWN,MOVE(infos),node_type);
                } else if (what == "deletedata"){
                    CHECK_VALUE_PARAM;

                    if(json_value->hasKey("start")&&json_value->hasKey("end")&&
                     json_value->isStringValue("start")&&("remove")&&json_value->isStringValue("end")){
                //remove data
                        uint64_t start_ts,end_ts;
                        int err=0;
                        start_ts=offsetToTimestamp(json_value->getStringValue("start"));
                        end_ts=offsetToTimestamp(json_value->getStringValue("end"));
                        for(int cnt=0;cnt<=chaos::DataPackCommonKey::DPCK_DATASET_TYPE_CU_ALARM;cnt++){
                            err+=live_driver->removeData(name+chaos::datasetTypeToPostfix(cnt),start_ts,end_ts);
                        }
                        if(err==0){
                            json_buf = "{}";
                            return CHAOS_DEV_OK;
                        } 
                        bundle_state.append_error("An error occurred removing data of:"+name);
                        json_buf = bundle_state.getData()->getCompliantJSONString();
                        CALC_EXEC_TIME;
                        return CHAOS_DEV_CMD;
                    
                     } else {
                        bundle_state.append_error("API must specify start and end delete inteval:"+name);
                        json_buf = bundle_state.getData()->getCompliantJSONString();
                        CALC_EXEC_TIME;
                        return CHAOS_DEV_CMD;
                     
                     }

                }else if (what == "deletenode"){
                        int err;
                        chaos::common::data::CDWUniquePtr p(new CDataWrapper());
                        p->addStringValue(chaos::NodeDefinitionKey::NODE_UNIQUE_ID, name);

                        p->addStringValue(chaos::NodeDefinitionKey::NODE_TYPE, nodeTypeToString(human2NodeType(node_type)));
                        p->addBoolValue("reset",true);
                        chaos::common::data::CDWUniquePtr msg=executeAPI(chaos::NodeDomainAndActionRPC::RPC_DOMAIN,"nodeNewDelete",p,err);
                        if(err!=0){
                            execute_chaos_api_error++;                                                                                          
                            std::stringstream ss;                                                                                             
                            ss << " error in :" << __FUNCTION__ << "|" << __LINE__ ;;   
                        bundle_state.append_error(ss.str());                                                                              
                        json_buf = bundle_state.getData()->getCompliantJSONString();                                                      
        

                        } else {
                            json_buf=(msg.get())?msg->getCompliantJSONString():"{}";
                        }
                        //EXECUTE_CHAOS_API(api_proxy::unit_server::DeleteUS, MDS_TIMEOUT, name);
                        res << json_buf;
                } else if (what == "nodeupdate"){
                        int err;
                        chaos::common::data::CDWUniquePtr p(new CDataWrapper());
                        p->addStringValue(chaos::NodeDefinitionKey::NODE_UNIQUE_ID, name);
                        if(parent.size()){
                            p->addStringValue(chaos::NodeDefinitionKey::NODE_PARENT,parent);
                        }
                        p->addStringValue(chaos::NodeDefinitionKey::NODE_TYPE, nodeTypeToString(human2NodeType(node_type)));
                        if(json_value.get()){
                            json_value->copyAllTo(*p);
                        }
                        chaos::common::data::CDWUniquePtr msg=executeAPI(chaos::NodeDomainAndActionRPC::RPC_DOMAIN,"setNodeDescription",p,err);
                        if(err!=0){
                            execute_chaos_api_error++;                                                                                          
                            std::stringstream ss;                                                                                             
                            ss << " error in :" << __FUNCTION__ << "|" << __LINE__ ;;   
                        bundle_state.append_error(ss.str());                                                                              
                        json_buf = bundle_state.getData()->getCompliantJSONString();                                                      
        

                        } else {
                            json_buf=(msg.get())?msg->getCompliantJSONString():"{}";
                        }
                        //EXECUTE_CHAOS_API(api_proxy::unit_server::DeleteUS, MDS_TIMEOUT, name);
                        res << json_buf;
                } else if (what == "new"){
                        int err;
                        chaos::common::data::CDWUniquePtr p(new CDataWrapper());
                        p->addStringValue(chaos::NodeDefinitionKey::NODE_UNIQUE_ID, name);
                        if(parent.size()){
                            p->addStringValue(chaos::NodeDefinitionKey::NODE_PARENT,parent);
                        }
                        p->addStringValue(chaos::NodeDefinitionKey::NODE_TYPE, nodeTypeToString(human2NodeType(node_type)));
                        if(json_value.get()){
                            json_value->copyAllTo(*p);
                        }
                        chaos::common::data::CDWUniquePtr msg=executeAPI(chaos::NodeDomainAndActionRPC::RPC_DOMAIN,"nodeNewDelete",p,err);
                        if(err!=0){
                            execute_chaos_api_error++;                                                                                          
                            std::stringstream ss;                                                                                             
                            ss << " error in :" << __FUNCTION__ << "|" << __LINE__ ;;   
                        bundle_state.append_error(ss.str());                                                                              
                        json_buf = bundle_state.getData()->getCompliantJSONString();                                                      
        

                        } else {
                            json_buf=(msg.get())?msg->getCompliantJSONString():"{}";
                        }
                        //EXECUTE_CHAOS_API(api_proxy::unit_server::DeleteUS, MDS_TIMEOUT, name);
                        res << json_buf;
                }
                else if (node_type == "us" || node_type == "root")
                {

                    if (what == "set")
                    {
                        EXECUTE_CHAOS_API(api_proxy::unit_server::GetSetFullUnitServer, MDS_TIMEOUT, name, 0, json_value.get());
                        res << json_buf;
                    }
                   
                    else if (what == "create")
                    {
                        EXECUTE_CHAOS_API(api_proxy::unit_server::NewUS, MDS_TIMEOUT, name);
                        res << json_buf;
                    }
                    else if (what == "get")
                    {
                        EXECUTE_CHAOS_API(api_proxy::unit_server::GetSetFullUnitServer, MDS_TIMEOUT, name);
                        res << json_buf;
                    }
                    else if (what == "parent")
                    {
                        EXECUTE_CHAOS_API(api_proxy::agent::GetAgentForNode, MDS_TIMEOUT, name);
                        res << json_buf;

                    }
                    else if (what == "start")
                    {
                        EXECUTE_CHAOS_API(chaos::metadata_service_client::api_proxy::agent::NodeOperation, MDS_TIMEOUT, name, chaos::service_common::data::agent::NodeAssociationOperationLaunch);
                        res << json_buf;
                    }
                    else if (what == "stop")
                    {
                        EXECUTE_CHAOS_API(chaos::metadata_service_client::api_proxy::agent::NodeOperation, MDS_TIMEOUT, name, chaos::service_common::data::agent::NodeAssociationOperationStop);
                        res << json_buf;
                    }
                    else if (what == "kill")
                    {
                        EXECUTE_CHAOS_API(chaos::metadata_service_client::api_proxy::agent::NodeOperation, MDS_TIMEOUT, name, chaos::service_common::data::agent::NodeAssociationOperationKill);
                        res << json_buf;
                    }
                    else if (what == "restart")
                    {
                        EXECUTE_CHAOS_API(chaos::metadata_service_client::api_proxy::agent::NodeOperation, MDS_TIMEOUT, name, chaos::service_common::data::agent::NodeAssociationOperationRestart);
                        res << json_buf;
                    } 
                }
                else if (node_type == "cu")
                {

                    if (what == "set")
                    {
                        if (json_value.get() && json_value->hasKey("properties"))
                        {
                            // set properties
                            if (json_value->isVectorValue("properties"))
                            {
                                chaos::common::property::PropertyGroup pg(chaos::ControlUnitPropertyKey::P_GROUP_NAME);

                                //							ChaosSharedPtr<chaos::metadata_service_client::api_proxy::node::NodePropertyGroup> cu_property_group(new chaos::metadata_service_client::api_proxy::node::NodePropertyGroup());
                                //						cu_property_group->group_name = "property_abstract_control_unit";
                                //					chaos::metadata_service_client::api_proxy::node::NodePropertyGroupList property_list;

                                ChaosSharedPtr<CMultiTypeDataArrayWrapper> dw = json_value->getVectorValue("properties");
                                for (int idx = 0; idx < dw->size(); idx++)
                                {
                                    if (dw->isCDataWrapperElementAtIndex(idx))
                                    {
                                        ChaosUniquePtr<CDataWrapper> prop = dw->getCDataWrapperElementAtIndex(idx);
                                        if (prop.get() && prop->hasKey(chaos::ControlUnitDatapackSystemKey::BYPASS_STATE))
                                        {
                                            //	ChaosSharedPtr<chaos::common::data::CDataWrapperKeyValueSetter> bool_value(new chaos::common::data::CDataWrapperBoolKeyValueSetter(chaos::ControlUnitDatapackSystemKey::BYPASS_STATE,prop->getBoolValue(chaos::ControlUnitDatapackSystemKey::BYPASS_STATE)));
                                            bool onoff = prop->getBoolValue(chaos::ControlUnitDatapackSystemKey::BYPASS_STATE);
                                            //cu_property_group->group_property_list.push_back(bool_value);
                                            pg.addProperty(chaos::ControlUnitDatapackSystemKey::BYPASS_STATE, CDataVariant(static_cast<bool>(onoff)));
                                        }
                                        if (prop.get() && prop->hasKey(chaos::ControlUnitDatapackSystemKey::THREAD_SCHEDULE_DELAY))
                                        {
                                          //  chaos::common::property::PropertyGroup pg(chaos::ControlUnitPropertyKey::P_GROUP_NAME);
                                            pg.addProperty(chaos::ControlUnitDatapackSystemKey::THREAD_SCHEDULE_DELAY, CDataVariant(static_cast<uint64_t>(prop->getInt64Value(chaos::ControlUnitDatapackSystemKey::THREAD_SCHEDULE_DELAY))));
                                        }
                                        if (prop.get() && prop->hasKey(chaos::ControlUnitPropertyKey::INIT_RESTORE_APPLY))
                                        {
                                            //chaos::common::property::PropertyGroup pg(chaos::ControlUnitPropertyKey::P_GROUP_NAME);
                                            pg.addProperty(chaos::ControlUnitPropertyKey::INIT_RESTORE_APPLY, CDataVariant(static_cast<bool>(prop->getBoolValue(chaos::ControlUnitPropertyKey::INIT_RESTORE_APPLY))));
                                        }
                                        if (prop.get() && prop->hasKey(chaos::ControlUnitPropertyKey::INIT_RESTORE_OPTION))
                                        {
                                            //chaos::common::property::PropertyGroup pg(chaos::ControlUnitPropertyKey::P_GROUP_NAME);
                                            pg.addProperty(chaos::ControlUnitPropertyKey::INIT_RESTORE_OPTION, CDataVariant(static_cast<int32_t>(prop->getInt32Value(chaos::ControlUnitPropertyKey::INIT_RESTORE_OPTION))));
                                        }
                                        if (prop.get() && prop->hasKey(chaos::DataServiceNodeDefinitionKey::DS_STORAGE_HISTORY_TIME))
                                        {
                                            //chaos::common::property::PropertyGroup pg(chaos::ControlUnitPropertyKey::P_GROUP_NAME);
                                            pg.addProperty(chaos::DataServiceNodeDefinitionKey::DS_STORAGE_HISTORY_TIME, CDataVariant(static_cast<uint64_t>(prop->getInt64Value(chaos::DataServiceNodeDefinitionKey::DS_STORAGE_HISTORY_TIME))));
                                        }

                                        if (prop.get() && prop->hasKey(chaos::DataServiceNodeDefinitionKey::DS_STORAGE_LIVE_TIME))
                                        {
                                           // chaos::common::property::PropertyGroup pg(chaos::ControlUnitPropertyKey::P_GROUP_NAME);
                                            pg.addProperty(chaos::DataServiceNodeDefinitionKey::DS_STORAGE_LIVE_TIME, CDataVariant(static_cast<uint64_t>(prop->getInt64Value(chaos::DataServiceNodeDefinitionKey::DS_STORAGE_LIVE_TIME))));
                                        }
                                        if (prop.get() && prop->hasKey(chaos::DataServiceNodeDefinitionKey::DS_STORAGE_TYPE))
                                        {
                                            //chaos::common::property::PropertyGroup pg(chaos::ControlUnitPropertyKey::P_GROUP_NAME);
                                            pg.addProperty(chaos::DataServiceNodeDefinitionKey::DS_STORAGE_TYPE, CDataVariant(static_cast<uint32_t>(prop->getInt32Value(chaos::DataServiceNodeDefinitionKey::DS_STORAGE_TYPE))));
                                        }
                                        if (prop.get() && prop->hasKey(chaos::DataServiceNodeDefinitionKey::DS_STORAGE_HISTORY_AGEING))
                                        {
                                           // chaos::common::property::PropertyGroup pg(chaos::ControlUnitPropertyKey::P_GROUP_NAME);
                                            pg.addProperty(chaos::DataServiceNodeDefinitionKey::DS_STORAGE_HISTORY_AGEING, CDataVariant(static_cast<uint32_t>(prop->getInt32Value(chaos::DataServiceNodeDefinitionKey::DS_STORAGE_HISTORY_AGEING))));
                                        }
                                    }
                                }
                                //	property_list.push_back(cu_property_group);

                                //GET_CHAOS_API_PTR((chaos::metadata_service_client::api_proxy::node::UpdateProperty)->execute(name,property_list)));
                                EXECUTE_CHAOS_API(chaos::metadata_service_client::api_proxy::node::UpdateProperty, MDS_TIMEOUT, name, pg);
                            }
                            else
                            {
                                RETURN_ERROR("'properties' should be a vector of properties");
                            }
                            res << json_buf;
                        }
                        else
                        {
                            std::string par;
                            std::string sub_type;
                            if(json_value->hasKey(chaos::NodeDefinitionKey::NODE_SUB_TYPE)){
                                sub_type=json_value->getStringValue(chaos::NodeDefinitionKey::NODE_SUB_TYPE);
                            }

                            if(json_value->hasKey(chaos::NodeDefinitionKey::NODE_PARENT)){
                                par=json_value->getStringValue(chaos::NodeDefinitionKey::NODE_PARENT);
                            } else {
                                CHECK_PARENT;
                                par =parent;
                            }
                            {
                                if (json_value->hasKey("control_unit_implementation")&&(sub_type!="nt_script_eu"))
                                {
                                    EXECUTE_CHAOS_API(api_proxy::unit_server::ManageCUType, MDS_TIMEOUT, par, json_value->getStringValue("control_unit_implementation"), 0);
                                }
                            }
                            {
                                EXECUTE_CHAOS_API(api_proxy::control_unit::SetInstanceDescription, MDS_TIMEOUT, name, *json_value);
                            }

                            res << json_buf;
                        }
                    }
                    else if (what == "del")
                    {
                        int ret=0,ret1=0;
                        if(!parent.empty())
                        {
                            EXECUTE_CHAOS_RET_API(ret,api_proxy::control_unit::DeleteInstance, MDS_TIMEOUT, parent, name);
                        }

                        {
                            EXECUTE_CHAOS_RET_API(ret1,api_proxy::control_unit::Delete, MDS_TIMEOUT, name);
                        }
                        if(ret!=0 || ret1!=0){
                        std::stringstream ss;                                                                                             

                        ss << " error Deleting CU in :" << __FUNCTION__ << "|" << __LINE__;  
                        bundle_state.append_error(ss.str());                                                                          

                        json_buf = bundle_state.getData()->getCompliantJSONString();                                  

                        } else {
                            json_buf = "{}";
                        }
                        res << json_buf;
                    }
                    else if (what == "get")
                    {
                        EXECUTE_CHAOS_API(api_proxy::control_unit::GetInstance, MDS_TIMEOUT, name);
                        res << json_buf;

                    }
                    else if (what == "load")
                    {
                        EXECUTE_CHAOS_API(api_proxy::unit_server::LoadUnloadControlUnit, MDS_TIMEOUT, name, true);

                        res << json_buf;
                    }
                    else if (what == "unload")
                    {
                        EXECUTE_CHAOS_API(api_proxy::unit_server::LoadUnloadControlUnit, MDS_TIMEOUT, name, false);

                        res << json_buf;
                    }
                    else if (what == "init")
                    {
                        EXECUTE_CHAOS_API(api_proxy::control_unit::InitDeinit, MDS_TIMEOUT, name, true);
                        res << json_buf;
                    }
                    else if (what == "deinit")
                    {
                        EXECUTE_CHAOS_API(api_proxy::control_unit::InitDeinit, MDS_TIMEOUT, name, false);
                        res << json_buf;
                    }
                    else if (what == "start")
                    {
                        EXECUTE_CHAOS_API(api_proxy::control_unit::StartStop, MDS_TIMEOUT, name, true);
                        res << json_buf;
                    }
                    else if (what == "stop")
                    {
                        EXECUTE_CHAOS_API(api_proxy::control_unit::StartStop, MDS_TIMEOUT, name, false);
                        res << json_buf;
                    } else if(what=="clrcmdq"){
                        EXECUTE_CHAOS_API(api_proxy::node::ClearCommandQueue, MDS_TIMEOUT, name);
                        res << json_buf;

                    } else if(what=="killcmd"){
                        EXECUTE_CHAOS_API(api_proxy::node::KillCurrentCommand, MDS_TIMEOUT, name);
                        res << json_buf;

                    }
                }
                else if (node_type == "agent")
                {
                    if (what == "getlog")
                    {
                        EXECUTE_CHAOS_API(chaos::metadata_service_client::api_proxy::agent::logging::GetProcessLogEntries, MDS_TIMEOUT, name, 100, 1, 0);
                        res << json_buf;
                    }
                    else if (what == "enablelog")
                    {
                        // set an association between a Agent and a Unit Server
                        EXECUTE_CHAOS_API(chaos::metadata_service_client::api_proxy::agent::logging::ManageNodeLogging, MDS_TIMEOUT, name, chaos::service_common::data::agent::NodeAssociationLoggingOperationEnable);
                        res << json_buf;
                    }
                    else if (what == "disablelog")
                    {
                        // set an association between a Agent and a Unit Server
                        EXECUTE_CHAOS_API(chaos::metadata_service_client::api_proxy::agent::logging::ManageNodeLogging, MDS_TIMEOUT, name, chaos::service_common::data::agent::NodeAssociationLoggingOperationDisable);
                        res << json_buf;
                    }
                    else if (what == "set")
                    {
                        // set an association between a Agent and a Unit Server
                        EXECUTE_CHAOS_API(chaos::metadata_service_client::api_proxy::agent::SaveNodeAssociation, MDS_TIMEOUT, name, *json_value.get());
                        res << json_buf;
                    }
                    else if (what == "del")
                    {
                        /*if(parent.empty()){
                        serr << cmd <<" must specify 'parent'";

                        bundle_state.append_error(serr.str());
                        json_buf = bundle_state.getData()->getCompliantJSONString();
                        return CHAOS_DEV_CMD;
                    }*/
                        CHECK_PARENT;
                        EXECUTE_CHAOS_API(chaos::metadata_service_client::api_proxy::agent::RemoveNodeAssociation, MDS_TIMEOUT, name, parent);
                        res << json_buf;
                    }
                    else if (what == "get")
                    {
                         CHECK_PARENT;
                        EXECUTE_CHAOS_API(chaos::metadata_service_client::api_proxy::agent::LoadNodeAssociation, MDS_TIMEOUT, name, parent);
                        res << json_buf;
                    }
                    else if (what == "list")
                    {
                        EXECUTE_CHAOS_API(chaos::metadata_service_client::api_proxy::agent::ListNodeForAgent, MDS_TIMEOUT, name);
                        res << json_buf;
                    }
                    else if (what == "info")
                    {
                        EXECUTE_CHAOS_API(chaos::metadata_service_client::api_proxy::agent::LoadAgentDescription, MDS_TIMEOUT, name);
                        res << json_buf;
                    }
                    else if (what == "check")
                    {
                        EXECUTE_CHAOS_API(chaos::metadata_service_client::api_proxy::agent::CheckAgentHostedProcess, MDS_TIMEOUT, name);
                        res << json_buf;
                    }

                }
                if (names.get() && ((idx + 1) < names->size()))
                {
                    res << ",";
                }
            } while (names.get() && (++idx < names->size()));
            if (names.get() && (names->size() > 0))
            {
                res << "]";
                ret = (execute_chaos_api_error==0)?CHAOS_DEV_OK:CHAOS_DEV_CMD;
;

            }
            json_buf = res.str();
            return (execute_chaos_api_error==0)?CHAOS_DEV_OK:CHAOS_DEV_CMD;;
        }
        else if (cmd == "log")
        {
            PARSE_QUERY_PARMS(args, false, false);
            if (what == "search")
            {
                std::vector<std::string> domains;
                if (node_type == "all")
                {
                    domains.push_back("error");
                    domains.push_back("warning");
                    domains.push_back("Info");
                    domains.push_back("log");
                    domains.push_back("command");
                }
                domains.push_back(node_type);
                EXECUTE_CHAOS_API(chaos::metadata_service_client::api_proxy::logging::SearchLogEntry, MDS_TIMEOUT, name, domains, start_ts, end_ts, seq_id, page);
                //EXECUTE_CHAOS_API(chaos::metadata_service_client::api_proxy::logging::GetLogForSourceUID,MDS_TIMEOUT,name,domains,seq_id,page);
                //chaos::common::data::CDWUniquePtr r = apires->detachResult();
               
                return (execute_chaos_api_error==0)?CHAOS_DEV_OK:CHAOS_DEV_CMD;

            }
            serr << cmd << " bad command format";
            bundle_state.append_error(serr.str());
            json_buf = bundle_state.getData()->getCompliantJSONString();
            return CHAOS_DEV_CMD;
        }
        else if (cmd == "script")
        {

            PARSE_QUERY_PARMS(args, false, true);
            if (what == "save")
            {
                CHECK_VALUE_PARAM

                CALL_CHAOS_API(chaos::metadata_service_client::api_proxy::script::SaveScript, MDS_TIMEOUT, json_value);
                json_buf = "{}";
                return CHAOS_DEV_OK;
            }
            else if (what == "del")
            {

                CALL_CHAOS_API(chaos::metadata_service_client::api_proxy::script::RemoveScript, MDS_TIMEOUT, json_value);
                json_buf = "{}";
                return CHAOS_DEV_OK;
            }
            else if (what == "newInstance")
            {
                CHECK_VALUE_PARAM
                if (!json_value->hasKey("create"))
                {
                    json_value->addBoolValue("create", true);
                }

                CALL_CHAOS_API(chaos::metadata_service_client::api_proxy::script::ManageScriptInstance, MDS_TIMEOUT, json_value);
                json_buf = "{}";
                return CHAOS_DEV_OK;
            }
            else if (what == "rmInstance")
            {
                CHECK_VALUE_PARAM
                if (!json_value->hasKey("create"))
                {
                    json_value->addBoolValue("create", false);
                }

                CALL_CHAOS_API(chaos::metadata_service_client::api_proxy::script::ManageScriptInstance, MDS_TIMEOUT, json_value);
                json_buf = "{}";
                return CHAOS_DEV_OK;
            }
            else if (what == "fload") // fast load
            {   
                chaos::common::data::CDWUniquePtr res;
                if(mdsChannel->getScriptDesc(name,res,MDS_TIMEOUT)!=0){
                    serr << cmd << " Error retriving script :"<<name;
                    bundle_state.append_error(serr.str());
                    json_buf = bundle_state.getData()->getCompliantJSONString();
                    return CHAOS_DEV_CMD;
                } 
                json_buf = res->getCompliantJSONString();

                return CHAOS_DEV_OK;
            }
            else if (what == "load")
            {
                CHECK_VALUE_PARAM;
                CALL_CHAOS_API(chaos::metadata_service_client::api_proxy::script::LoadFullScript, MDS_TIMEOUT, json_value);
                chaos::common::data::CDWUniquePtr r = apires->detachResult();
                if (r.get())
                {
                    json_buf = r->getCompliantJSONString();
                }
                else
                {
                    json_buf = "{}";
                    serr << cmd << " API error command format";
                    bundle_state.append_error(serr.str());
                    json_buf = bundle_state.getData()->getCompliantJSONString();
                    return CHAOS_DEV_CMD;
                }
                return CHAOS_DEV_OK;
            }
            else if (what == "searchInstance")
            {
                CHECK_VALUE_PARAM;

                CALL_CHAOS_API(chaos::metadata_service_client::api_proxy::script::SearchInstancesForScript, MDS_TIMEOUT, json_value);
                chaos::common::data::CDWUniquePtr r = apires->detachResult();
                if (r.get())
                {
                    json_buf = r->getCompliantJSONString();
                }
                else
                {
                    json_buf = "{}";
                    serr << cmd << " API error command format";
                    bundle_state.append_error(serr.str());
                    json_buf = bundle_state.getData()->getCompliantJSONString();
                    return CHAOS_DEV_CMD;
                }
                return CHAOS_DEV_OK;
            }
            else if (what == "bind")
            {

                CALL_CHAOS_API(chaos::metadata_service_client::api_proxy::script::UpdateBindType, MDS_TIMEOUT, json_value);
                json_buf = "{}";
                return CHAOS_DEV_OK;
            }
            else if (what == "update")
            {

                CALL_CHAOS_API(chaos::metadata_service_client::api_proxy::script::UpdateScriptOnNode, MDS_TIMEOUT, json_value);
                json_buf = "{}";
                return CHAOS_DEV_OK;
            }
            else
            {
                serr << cmd << " bad 'script' command format";
                bundle_state.append_error(serr.str());
                json_buf = bundle_state.getData()->getCompliantJSONString();
                return CHAOS_DEV_CMD;
            }
        }

        
        //Json::Reader rreader;
        //Json::Value vvalue;
        /*	if (wostate == 0) {
            std::stringstream ss;

            if ((state == chaos::CUStateKey::RECOVERABLE_ERROR) || (state == chaos::CUStateKey::FATAL_ERROR)) {
                chaos::common::data::CDataWrapper*data = fetch(KeyDataStorageDomainHealth);
                std::string ll;

                if (data->hasKey("nh_led")) {
                    ll = std::string("domain:") + data->getCStringValue("nh_led");
                }
                if (data->hasKey("nh_lem")) {
                    ll = ll + std::string(" msg:") + data->getCStringValue("nh_lem");
                }
                bundle_state.append_error(ll);
                json_buf = bundle_state.getData()->getCompliantJSONString();
                CALC_EXEC_TIME;


                return (state == chaos::CUStateKey::RECOVERABLE_ERROR) ? CHAOS_DEV_RECOVERABLE_ERROR : CHAOS_DEV_FATAL_ERROR;
            }
            if (((cmd!="init")|| ((next_state > 0)&&(state != next_state))) &&((reqtime - last_access) > CHECK_HB)) {
                if (checkHB() == 0) {
                    ss << " [" << path << "] HB expired:" << (reqtime - last_access) << " us greater than " << timeo << " us, removing device";
                    bundle_state.append_error(ss.str());
                    json_buf = bundle_state.getData()->getCompliantJSONString();
                    //					init(path, timeo);

                    CALC_EXEC_TIME;
                    return CHAOS_DEV_HB_TIMEOUT;
                }
                last_access = reqtime;

            }

        } else
         */
        if (wostate && (cmd == "status"))
        {
                bundle_state.reset();

            bundle_state.status(chaos::CUStateKey::START);
            state = chaos::CUStateKey::START;
            bundle_state.append_log("stateless device");
            //chaos::common::data::CDataWrapper* data = fetch(KeyDataStorageDomainOutput);
            //json_buf = data->getCompliantJSONString();
            json_buf = fetchJson(KeyDataStorageDomainOutput);
            CALC_EXEC_TIME;
            return CHAOS_DEV_OK;
        }

        if (cmd == "init")
        {
            wostate = 0;
            bundle_state.append_log("init device:" + path);
            err = initDevice();
            if (err != 0)
            {
                init(path, timeo);
                err = initDevice();
            }
            if (err != 0)
            {
                bundle_state.append_error("initializing device:" + path);
                CALC_EXEC_TIME;
                return CHAOS_DEV_CMD;
            }
            next_state = chaos::CUStateKey::INIT;

            json_buf = fetchJson(255);
            return CHAOS_DEV_OK;
        }
        else if (cmd == "start")
        {
            wostate = 0;
            err = startDevice();
            if (err != 0)
            {
                init(path, timeo);
                err = startDevice();
            }
            if (err != 0)
            {
                bundle_state.append_error("starting device:" + path);
                CALC_EXEC_TIME;
                return CHAOS_DEV_CMD;
            }
            next_state = chaos::CUStateKey::START;
            json_buf = fetchJson(255);
            return CHAOS_DEV_OK;
        }
        else if (cmd == "stop")
        {
            wostate = 0;
            err = stopDevice();
            if (err != 0)
            {
                init(path, timeo);
                err = stopDevice();
            }
            if (err != 0)
            {

                bundle_state.append_error("stopping device:" + path);
                json_buf = bundle_state.getData()->getCompliantJSONString();
                CALC_EXEC_TIME;
                return CHAOS_DEV_CMD;
            }
            next_state = chaos::CUStateKey::STOP;

            //chaos::common::data::CDataWrapper* data = fetch(-1);
            //json_buf = data->getCompliantJSONString();
            json_buf = fetchJson(255);
            return CHAOS_DEV_OK;
        }
        else if (cmd == "deinit")
        {
            wostate = 0;
            err = deinitDevice();
            if (err != 0)
            {
                init(path, timeo);
                err = deinitDevice();
            }
            if (err != 0)
            {
                bundle_state.append_error("deinitializing device:" + path);
                json_buf = bundle_state.getData()->getCompliantJSONString();
                CALC_EXEC_TIME;
                return CHAOS_DEV_CMD;
            }
            next_state = chaos::CUStateKey::DEINIT;

            json_buf = fetchJson(255);
            return CHAOS_DEV_OK;
        }
        else if (cmd == "sched" && (args != 0))
        {
            bundle_state.append_log("sched device:" + path + " args:" + std::string(args));
            err = setSchedule(atoll((char *)args));
            if (err != 0)
            {
                init(path, timeo);
                err = setSchedule(atoll((char *)args));
            }
            if (err != 0)
            {
                bundle_state.append_error("error set scheduling:" + path);
                /*			//				init(path, timeo);
                json_buf = bundle_state.getData()->getCompliantJSONString();
                CALC_EXEC_TIME;
                return CHAOS_DEV_CMD;
                 */
            }
            //chaos::common::data::CDataWrapper* data = fetch(-1);
            //json_buf = data->getCompliantJSONString();
            json_buf = fetchJson(255);
            return CHAOS_DEV_OK;
        }
        else if (cmd == "desc")
        {
            bundle_state.append_log("desc device:" + path);

            CDWUniquePtr out ;
            if (mdsChannel->getFullNodeDescription(path, out, MDS_TIMEOUT) == 0)
            {
                json_buf = "{}";
                if (out.get()){
                    json_buf = out->getCompliantJSONString();
                    CALC_EXEC_TIME;
                }

                return CHAOS_DEV_OK;
            }
            else
            {
                bundle_state.append_error("error describing device:" + path);
                //				init(path, timeo);
                //json_buf = bundle_state.getData()->getCompliantJSONString();
                CALC_EXEC_TIME;
                
                //chaos::common::data::CDataWrapper* data = fetch(-1);
                //json_buf = data->getCompliantJSONString();
                json_buf = fetchJson(255);
                return CHAOS_DEV_OK;
            }
        }
        else if (cmd == "channel" && (args != 0))
        {
            // bundle_state.append_log("return channel :" + parm);
            //chaos::common::data::CDataWrapper*data = fetch(atoi((char*) args));
            //json_buf = data->getCompliantJSONString();
            int channel=atoi((char *)args);
            if(channel==128){
                json_buf =fetch(channel)->getCompliantJSONString();
                return CHAOS_DEV_OK;
            }
            std::string ret = fetchJson(channel);

            if(ret.size()<4){
                ret = fetch(channel)->getCompliantJSONString();
                DBGET << "cache not valid retrieved :\"" << ret << "\"";

            }
            json_buf = (ret.size() == 0) ? "{}" : ret;
            return CHAOS_DEV_OK;
        }
        else if (cmd == "readout" && (args != 0))
        {
            // bundle_state.append_log("return channel :" + parm);
            std::string var_name = args;
            chaos::common::data::CDWUniquePtr data = fetch(KeyDataStorageDomainOutput);
            json_buf = dataset2Var(data.get(), var_name);
            return CHAOS_DEV_OK;
        }
        else if (cmd == "readin" && (args != 0))
        {
            // bundle_state.append_log("return channel :" + parm);
            std::string var_name = args;
           chaos::common::data::CDWUniquePtr data = fetch(KeyDataStorageDomainInput);
            json_buf = dataset2Var(data.get(), var_name);
            return CHAOS_DEV_OK;
        }
        else if (cmd == "queryhst" && (args != 0))
        {
            chaos_data::CDataWrapper p;
            uint64_t start_ts = 0, end_ts = 0xffffffff;
            int32_t page = DEFAULT_PAGE;
            int paging = 0, cnt = 0;
            int limit = MAX_QUERY_ELEMENTS;
            uint32_t current_query = 0;
            std::stringstream res;
            std::string var_name;
            int channel = 0;
            std::string fmtType;
            chaos::common::io::QueryCursor *query_cursor = NULL;
            p.setSerializedJsonData(args);
            cleanUpQuery();
            if (p.hasKey("start"))
            {
                if (p.isStringValue("start"))
                {
                    start_ts = offsetToTimestamp(p.getStringValue("start"));
                }
                else
                {
                    start_ts = p.getInt64Value("start");
                    
                    if (start_ts == -1)
                    {
                        start_ts = chaos::common::utility::TimingUtil::getLocalTimeStamp();
                    } else {
                        std::stringstream ss;
                        ss<<start_ts;
                        start_ts = offsetToTimestamp(ss.str());
                    }
                }
            }
            if(p.hasKey("fmtType")){
                fmtType=p.getStringValue("fmtType");
            }

            if (p.hasKey("end"))
            {
                if (p.isStringValue("end"))
                {
                    end_ts = offsetToTimestamp(p.getStringValue("end"));
                }
                else
                {
                    end_ts = p.getInt64Value("end");
                    if (end_ts == -1)
                    {
                        end_ts = chaos::common::utility::TimingUtil::getLocalTimeStamp();
                    } else {
                      
                        std::stringstream ss;
                        ss<<end_ts;
                        end_ts = offsetToTimestamp(ss.str());
                    }
                }
            }
            
            if (p.hasKey("page"))
            {
                page = p.getInt32Value("page");
                if (page > MAX_QUERY_ELEMENTS)
                {
                    page = MAX_QUERY_ELEMENTS;
                }
                paging = 1;
            }

            if (p.hasKey("channel"))
            {
                if (p.isInt32Value("channel"))
                {
                    if ((p.getInt32Value("channel") >= 0) && (p.getInt32Value("channel") <= DPCK_LAST_DATASET_INDEX))
                    {
                        channel = p.getInt32Value("channel");
                    }
                }
                if (p.isStringValue("channel"))
                {
                    channel = chaos::HumanTodatasetType(p.getStringValue("channel"));
                }
            }
            if (p.hasKey("limit"))
            {
                limit = p.getInt32Value("limit");
                if ((limit > 0) && (limit <= page))
                {
                    page = limit;
                    paging = 0;
                }
            }

            if (p.hasKey("var"))
            {
                var_name = p.getStringValue("var");
            }
            if (p.hasKey("seq"))
            {
                // perform a query without cursor.
                uint64_t seqid = p.getInt64Value("seq");
                uint64_t runid = 0;
                std::set<std::string> tags,projection;
                if (p.hasKey("runid")){
                    runid = p.getInt64Value("runid");
                }
                if (p.hasKey("tags")){
                    if(p.isVector("tags")){
                        ChaosSharedPtr<CMultiTypeDataArrayWrapper> dw = p.getVectorValue("tags");
                        tags=*dw;
                        
                    } else if(p.isStringValue("tags")){
                        tags.insert(p.getStringValue("tags"));
                    }
                }
                if (p.hasKey("projection")){
                        ChaosSharedPtr<CMultiTypeDataArrayWrapper> dw = p.getVectorValue("projection");
                        projection=*dw;
                    }
                DBGET << "START SEQ QUERY :" << std::dec << start_ts << " end:" << end_ts << "seq id " << seqid << " run id:" << runid << " page:" << page;

                executeTimeIntervalQuery((chaos::metadata_service_client::node_controller::DatasetDomain)channel, start_ts, end_ts, seqid, runid, tags,&query_cursor, projection,path,page);
                if (query_cursor)
                {
                    cnt = 0;
                    if(fmtType=="tgz"){
                        // all the elements into a tgz.
                    }
                    res << "{\"data\":[";
                    boost::shared_ptr<chaos::common::data::CDataWrapper> data;
                    uint32_t reduction_factor=1;
                    uint32_t count_items=0;
                    if(p.hasKey("reduction")){
                        reduction_factor=p.getInt32Value("reduction");
                    }
                    if(p.hasKey("count")){
                        count_items=p.getInt32Value("count");
                    }
                    std::vector<std::string> keys;
                    if (p.hasKey("projection")){
                        ChaosSharedPtr<CMultiTypeDataArrayWrapper> dw = p.getVectorValue("projection");
                        keys=*dw;
                    }
                    if (query_cursor->hasNext())
                    {
                        uint32_t elems = query_cursor->size();
                        int cntt=0;
                        while (cnt < elems)
                        {
                            ChaosSharedPtr<CDataWrapper> q_result(query_cursor->next());
                            data = normalizeToJson(q_result.get(), binaryToTranslate);
                            if( /*((cnt+1)==elems) || */(reduction_factor==1) || ((count_items%reduction_factor)==0)){
                                if(cntt>0){
                                    res<<",";
                                }
                                cntt++;
                            if (var_name.size() && data->hasKey(var_name)){
                                res << dataset2Var(data.get(), var_name);
                            }
                            else{
                                    if (keys.size()){
                                        res<<(data->getCSProjection(keys))->getCompliantJSONString();          
                                    } else {
                                        res << data->getCompliantJSONString();
    
                                    }
                                   
                                }
                            }
                            cnt++;
                            count_items++;
                           
                        }
                    }
                    res << "]";

                    query_cursor->getIndexes(runid, seqid);
                    res << ",\"seqid\":" << seqid << ",\"runid\":" << runid << ",\"count\":"<<count_items<<",\"end\":" << ((query_cursor->hasNext()) ? 0 : 1) << "}";
                    //DBGET <<" returned:"<<res.str();
                    releaseQuery(query_cursor);
                    json_buf = res.str();
                    return CHAOS_DEV_OK;
                }
                else
                {
                    bundle_state.append_error("cannot perform specified query, no data? " + getPath());
                    json_buf = bundle_state.getData()->getCompliantJSONString();
                    CALC_EXEC_TIME;
                    return CHAOS_DEV_CMD;
                }
            }

            if (paging)
            {
                bundle_state.append_error("OLD STYLE PAGED QUERIES NOT ANYMORE SUPPORTED");
                json_buf = bundle_state.getData()->getCompliantJSONString();
                CALC_EXEC_TIME;
                return CHAOS_DEV_CMD;

                if (query_cursor_map.size() < MAX_CONCURRENT_QUERY)
                {
                    DBGET << "START PAGED QUERY :" << std::dec << start_ts << " end:" << end_ts << " page size " << page;

                    executeTimeIntervalQuery((chaos::metadata_service_client::node_controller::DatasetDomain)channel, start_ts, end_ts, &query_cursor,path, page);
                    if (query_cursor)
                    {
                        cnt = 0;
                        bool n = query_cursor->hasNext();
                        res << "{\"data\":[";
                        boost::shared_ptr<chaos::common::data::CDataWrapper> data;
                        DBGET << "paged query start:" << std::dec << start_ts << " end:" << end_ts << " page uid " << queryuid << " has next:" << n;
                        current_query = queryuid;

                        while ((query_cursor->hasNext()) && (cnt < page) && (cnt < limit))
                        {
                            ChaosSharedPtr<CDataWrapper> q_result(query_cursor->next());
                            //	DBGET << " query uid: " <<queryuid << " page:"<<cnt;
                            data = normalizeToJson(q_result.get(), binaryToTranslate);
                            if (var_name.size() && data->hasKey(var_name))
                            {
                                res << dataset2Var(data.get(), var_name);
                            }
                            else
                            {
                                res << data->getCompliantJSONString();
                            }
                            //	DBGET << "OBJ  " <<res;
                            cnt++;
                            //	DBGET << "getting query page  " << cnt;
                            if ((query_cursor->hasNext()) && (cnt < page) && (cnt < limit))
                            {
                                res << ",";
                            }
                        }
                        res << "]";
                        if ((query_cursor->hasNext()))
                        {
                            qc_t q_nfo;
                            q_nfo.qt = reqtime / 1000;
                            q_nfo.qc = query_cursor;
                            query_cursor_map[++queryuid] = q_nfo;
                            res << ",\"uid\":" << queryuid << "}";
                            DBGET << "continue on UID:" << queryuid;
                        }
                        else
                        {
                            int32_t err;
                            if ((err = query_cursor->getError()))
                            {
                                releaseQuery(query_cursor);
                                bundle_state.append_error(CHAOS_FORMAT("error during query '%1%' with  api error: %2%", % getPath() % err));
                                json_buf = bundle_state.getData()->getCompliantJSONString();
                                /// TODO : perche' devo rinizializzare il controller?
                                //init(path, timeo);

                                CALC_EXEC_TIME;
                                return CHAOS_DEV_CMD;
                            }
                            else
                            {
                                res << ",\"uid\":0}";
                                DBGET << "queryhst no more pages items:" << cnt;
                            }
                            releaseQuery(query_cursor);
                        }
                    }
                    else
                    {
                        bundle_state.append_error("cannot perform specified query, no data? " + getPath());
                        json_buf = bundle_state.getData()->getCompliantJSONString();
                        CALC_EXEC_TIME;
                        return CHAOS_DEV_CMD;
                    }
                }
                else
                {
                    bundle_state.append_error("too many concurrent queries, please try later on " + getPath());
                    json_buf = bundle_state.getData()->getCompliantJSONString();
                    CALC_EXEC_TIME;
                    return CHAOS_DEV_CMD;
                }

                json_buf = res.str();
                return CHAOS_DEV_OK;
            }
            else
            {
                boost::shared_ptr<chaos::common::data::CDataWrapper> data;
                DBGET << "START QUERY :" << std::dec << start_ts << " end:" << end_ts << " page size " << page;

                executeTimeIntervalQuery((chaos::metadata_service_client::node_controller::DatasetDomain)channel, start_ts, end_ts, &query_cursor);
                bool n = query_cursor->hasNext();
                if (query_cursor)
                {
                    DBGET << "not paged query start:" << start_ts << " end:" << end_ts << " has next: " << (query_cursor->hasNext());
                    cnt = 0;
                    res << "{\"data\":[";

                    while ((query_cursor->hasNext()) && (cnt < limit))
                    {

                        ChaosSharedPtr<CDataWrapper> q_result(query_cursor->next());
                        data = normalizeToJson(q_result.get(), binaryToTranslate);
                        if (var_name.size() && data->hasKey(var_name))
                        {
                            res << dataset2Var(data.get(), var_name);
                        }
                        else
                        {
                            res << data->getCompliantJSONString();
                        }
                        cnt++;
                        //	DBGET << "getting query page  " << cnt;
                        if ((query_cursor->hasNext()) && (cnt < limit))
                        {
                            res << ",";
                        }
                    }
                    res << "],\"uid\":0}";
                }
                else
                {
                    bundle_state.append_error("cannot perform specified query, no data? " + getPath());
                    json_buf = bundle_state.getData()->getCompliantJSONString();
                    CALC_EXEC_TIME;
                    return CHAOS_DEV_CMD;
                }
            }
            if (query_cursor && ((!query_cursor->hasNext()) || (cnt == limit)))
            {
                DBGET << "queryhst no more pages items:" << cnt;
                releaseQuery(query_cursor);
                if (current_query)
                {
                    query_cursor_map.erase(query_cursor_map.find(current_query));
                }
            }
            json_buf = res.str();
            return CHAOS_DEV_OK;
        }
        else if (cmd == "queryinfo")
        {
            uint32_t uid = 0;
            std::stringstream res;
            int cnt = 0;
            chaos_data::CDataWrapper p;
            p.setSerializedJsonData(args);
            if (p.hasKey("clearuid"))
            {
                uid = p.getInt32Value("clearuid");
            }
            if ((uid > 0) && (query_cursor_map.find(uid) != query_cursor_map.end()))
            {
                DBGET << "removing query " << uid;
                query_cursor_map.erase(query_cursor_map.find(uid));
            }

            res << "[";
            for (query_cursor_map_t::iterator i = query_cursor_map.begin(); i != query_cursor_map.end(); i++, cnt++)
            {
                if ((cnt + 1) < query_cursor_map.size())
                {
                    res << i->first << ",";
                }
                else
                {
                    res << i->first;
                }
            }
            res << "]";

            json_buf = res.str();
            return CHAOS_DEV_OK;
        }
        else if (cmd == "queryhstnext")
        {
            chaos::common::io::QueryCursor *query_cursor = NULL;
            chaos_data::CDataWrapper p;
            boost::shared_ptr<CDataWrapper> data;
            p.setSerializedJsonData(args);
            std::stringstream res;
            bool clear_req = false;
            std::string var_name;
            int cnt = 0;
            uint32_t uid = 0;
            bundle_state.append_error("OLD STYLE NEXT PAGED QUERIES NOT ANYMORE SUPPORTED");
            json_buf = bundle_state.getData()->getCompliantJSONString();
            CALC_EXEC_TIME;
            return CHAOS_DEV_CMD;
            if (p.hasKey("uid"))
            {
                uid = p.getInt32Value("uid");
            }
            else
            {
                bundle_state.append_error("must specify a valid page uid " + getPath());
                json_buf = bundle_state.getData()->getCompliantJSONString();
                CALC_EXEC_TIME;
                return CHAOS_DEV_CMD;
            }

            if (p.hasKey("clear"))
            {
                clear_req = p.getBoolValue("clear");
            }

            if (p.hasKey("var"))
            {
                var_name = p.getStringValue("var");
            }
            DBGET << "querynext uid:" << uid << " clear:" << clear_req;
            if (query_cursor_map.find(uid) != query_cursor_map.end())
            {
                query_cursor = query_cursor_map[uid].qc;
                query_cursor_map[uid].qt = reqtime / 1000;
                if (query_cursor)
                {
                    cnt = 0;
                    uint32_t page = query_cursor->getPageLen();
                    res << "{\"data\":[";

                    while ((query_cursor->hasNext()) && (cnt < page))
                    {
                        ChaosSharedPtr<CDataWrapper> q_result(query_cursor->next());
                        data = normalizeToJson(q_result.get(), binaryToTranslate);
                        if (var_name.size() && data->hasKey(var_name))
                        {
                            res << dataset2Var(data.get(), var_name);
                        }
                        else
                        {
                            res << data->getCompliantJSONString();
                        }
                        cnt++;
                        if ((query_cursor->hasNext()) && (cnt < page))
                        {
                            res << ",";
                        }
                    }
                    res << "]";
                    if ((!query_cursor->hasNext()) || clear_req)
                    {
                        int32_t err;
                        if ((err = query_cursor->getError()))
                        {
                            releaseQuery(query_cursor);
                            query_cursor_map.erase(query_cursor_map.find(uid));
                            bundle_state.append_error(CHAOS_FORMAT("error during query '%1%' with uid:%2% api error: %3%", % getPath() % uid % err));
                            json_buf = bundle_state.getData()->getCompliantJSONString();
                            CALC_EXEC_TIME;
                            //init(path, timeo);

                            return CHAOS_DEV_CMD;
                        }
                        res << ",\"uid\":0}";
                        DBGET << "queryhstnext no more pages items:" << cnt << " with uid:" << uid;
                        releaseQuery(query_cursor);
                        query_cursor_map.erase(query_cursor_map.find(uid));
                    }
                    else
                    {
                        res << ",\"uid\":" << uid << "}";
                        DBGET << "some page still missing:" << cnt << " with uid:" << uid;
                    }

                    json_buf = res.str();
                    return CHAOS_DEV_OK;
                }
                bundle_state.append_error("cannot perform specified query, no data? " + getPath());
                json_buf = bundle_state.getData()->getCompliantJSONString();
                CALC_EXEC_TIME;
                return CHAOS_DEV_CMD;
            }
            else
            {
                std::stringstream ss;
                ss << "the uid " << uid << " does not exists " + getPath();
                bundle_state.append_error(ss.str());
                CALC_EXEC_TIME;
                return CHAOS_DEV_CMD;
            }
            if (query_cursor)
            {
                uint32_t exported = 0;
                if (query_cursor->hasNext())
                {
                    ChaosSharedPtr<CDataWrapper> q_result(query_cursor->next());
                    json_buf = q_result->getCompliantJSONString();
                    return CHAOS_DEV_OK;
                }
                else
                {
                    releaseQuery(query_cursor);
                    query_cursor = NULL;
                }
            }
            json_buf = "{}";
            return CHAOS_DEV_OK;
        }
        else if (cmd == "save" && (args != 0))
        {
            int ret;
            chaos_data::CDataWrapper p;
            std::string snapname = args;
            try
            {
                p.setSerializedJsonData(args);
                if (p.hasKey("snapname"))
                {
                    snapname = p.getStringValue("snapname");
                }
            }
            catch (std::exception ee)
            {
            }
            json_buf = "{}";

            std::vector<std::string> other;
            ret = createNewSnapshot(snapname, other);
            DBGET << "creating snapshot " << snapname << " ret:" << ret;

            if (ret == 0)
            {
                DBGET << "SAVE snapshot " << snapname << " ret:" << ret;
                CALC_EXEC_TIME
                return CHAOS_DEV_OK;
            }
            bundle_state.append_error("error saving snapshot " + getPath() + " snapname:" + snapname);
            CALC_EXEC_TIME;
            return CHAOS_DEV_CMD;
        }
        else if (cmd == "delete" && (args != 0))
        {
            int ret;
            chaos_data::CDataWrapper p;
            std::string snapname = args;
            try
            {
                p.setSerializedJsonData(args);
                if (p.hasKey("snapname"))
                {
                    snapname = p.getStringValue("snapname");
                }
            }
            catch (std::exception ee)
            {
            }
            ret = deleteSnapshot(snapname);
            if (ret == 0)
            {
                DBGET << "DELETE snapshot " << snapname << " ret:" << ret;
                return CHAOS_DEV_OK;
            }
            bundle_state.append_error("error deleting snapshot " + getPath() + " key:" + std::string(args));
            json_buf = bundle_state.getData()->getCompliantJSONString();
            CALC_EXEC_TIME;
            return CHAOS_DEV_CMD;
        }
        else if (cmd == "load" && (args != 0))
        {
            chaos_data::CDataWrapper res;
            chaos_data::CDataWrapper p;
            std::string snapname = args;

            try
            {
                p.setSerializedJsonData(args);
                if (p.hasKey("snapname"))
                {
                    snapname = p.getStringValue("snapname");
                }
            }
            catch (std::exception ee)
            {
            }
            int retc = 0;
            DBGET << "LOADING snapshot " << snapname;
            if (mdsChannel->loadSnapshotNodeDataset(snapname, getPath(), res, MDS_TIMEOUT) == 0)
            {
                DBGET << "load snapshot name:\"" << snapname << "\": CU:" << getPath();
                json_buf = res.getCompliantJSONString();
                return CHAOS_DEV_OK;
            }
            bundle_state.append_error("error making load snapshot " + getPath() + " snap name:" + snapname);
            json_buf = bundle_state.getData()->getCompliantJSONString();
            CALC_EXEC_TIME;
            return CHAOS_DEV_CMD;
        }
        else if (cmd == "list")
        {
            ChaosStringVector snaps;
            int ret;
            ret = getSnapshotList(snaps);
            std::stringstream ss;

            DBGET << "list " << snaps.size() << " snapshots err:" << ret;
            if (ret == 0)
            {
                ss << "[";
                for (ChaosStringVector::iterator i = snaps.begin(); i != snaps.end(); i++)
                {
                    if ((i + 1) == snaps.end())
                    {
                        ss << "\"" << *i << "\"";
                    }
                    else
                    {
                        ss << "\"" << *i << "\",";
                    }
                }
                ss << "]";
                json_buf = ss.str();
                return CHAOS_DEV_OK;
            }
            else
            {
                bundle_state.append_error("error making listing snapshot for:" + getPath());
                json_buf = bundle_state.getData()->getCompliantJSONString();
                return CHAOS_DEV_CMD;
            }
        } else if(cmd=="buildInfo"){
            chaos::common::data::CDWUniquePtr mdsinfo,webuinfo,webuiproc,mdsproc;
            chaos::common::data::CDataWrapper infos;

            mdsChannel->getBuildInfo(mdsinfo);
            mdsChannel->getProcessInfo(mdsproc);
            mdsinfo->addCSDataValue("process",*mdsproc.get());
            infos.addCSDataValue("mds",*mdsinfo.get());
            
            DBGET << "BUILD INFO:" << infos.getCompliantJSONString();
            chaos::common::data::CDWUniquePtr webui=getBuildProcessInfo("","webui",true);
            infos.addCSDataValue("webui",*webui.get());

            chaos::common::data::CDWUniquePtr agent=getBuildProcessInfo("","agent",true);
            infos.addCSDataValue("agent",*agent.get());
            chaos::common::data::CDWUniquePtr us=getBuildProcessInfo("","us",true);
            infos.addCSDataValue("us",*us.get());
            chaos::common::data::CDWUniquePtr mds=getBuildProcessInfo("","mds",true);
            infos.addCSDataValue("mdss",*mds.get());
            
            json_buf=infos.getCompliantJSONString();
            return CHAOS_DEV_OK;

            
        }
        else if (cmd == "attr" && (args != 0))
        {

            chaos::common::data::CDataWrapper data;
            data.setSerializedJsonData(args);
            std::vector<std::string> attrs;
            data.getAllKey(attrs);
            for (std::vector<std::string>::iterator i = attrs.begin(); i != attrs.end(); i++)
            {
                char param[1024];
                std::string check;
                check.assign(data.getCStringValue(*i));
                if (check.compare(0, 2, "0x") == 0)
                {
                    sprintf(param, "%lld", strtoull(data.getCStringValue(*i), 0, 0));
                    DBGET << "converted parameter:" << param;
                }
                else
                {
                    strncpy(param, data.getCStringValue(*i), sizeof(param));
                }
                DBGET << "applying \"" << i->c_str() << "\"=" << param;
                bundle_state.append_log("send attr:\"" + cmd + "\" args: \"" + std::string(param) + "\" to device:\"" + path + "\"");
                err = setAttributeToValue(i->c_str(), param);
                if (err != 0)
                {
                    if (init(path, timeo) == 0)
                    {
                        DBGET << "error reapplying ...";
                        err = setAttributeToValue(i->c_str(), param);
                    }
                }
                if (err != 0)
                {

                    bundle_state.append_error("error setting attribute:" + path + "/" + *i + "\"=" + data.getCStringValue(*i));

                    //					init(path, timeo);
                    json_buf = bundle_state.getData()->getCompliantJSONString();
                    CALC_EXEC_TIME;
                    return CHAOS_DEV_CMD;
                }
            }
            json_buf = bundle_state.getData()->getCompliantJSONString();
            DBGET << "attribute applied:" << json_buf;
            return CHAOS_DEV_OK;
        }
        else if (cmd == "recover")
        {
            bundle_state.append_log("send recover from error:\"" + path);
            err = recoverDeviceFromError();
            if (err != 0)
            {
                bundle_state.append_error("error recovering from error " + path);
                //init(path, timeo);
                json_buf = bundle_state.getData()->getCompliantJSONString();
                CALC_EXEC_TIME;
                return CHAOS_DEV_CMD;
            }
            json_buf = bundle_state.getData()->getCompliantJSONString();
            return CHAOS_DEV_OK;
        }
        else if (cmd == "restore" && (args != 0))
        {
            bundle_state.append_log("send restore on \"" + path + "\" tag:\"" + std::string(args) + "\"");
            err = restoreDeviceToTag(args);
            if (err != 0)
            {
                bundle_state.append_error("error setting restoring:\"" + path + "\" with tag:\"" + std::string(args) + "\"");
                //				init(path, timeo);
                json_buf = bundle_state.getData()->getCompliantJSONString();
                CALC_EXEC_TIME;
                return CHAOS_DEV_CMD;
            }
            json_buf = bundle_state.getData()->getCompliantJSONString();
            return CHAOS_DEV_OK;
        }
        else if (cmd != "status")
        {
            bundle_state.append_log("send cmd:\"" + cmd + "\" args: \"" + std::string(args) + "\" to device:" + path);
            command_t command = prepareCommand(cmd);
            if (*args != 0)
            {
                command->param.setSerializedJsonData(args);
            }
            command->priority = prio;
            command->sub_rule = (submission_mode == 1) ? chaos::common::batch_command::SubmissionRuleType::SUBMIT_AND_KILL : chaos::common::batch_command::SubmissionRuleType::SUBMIT_NORMAL;
#ifndef CMD_BY_MDS           
            err = sendCmd(command, false);
#else
            err = sendMDSCmd(command);
#endif  
            if (err != 0)
            {
                init(path, timeo);
#ifndef CMD_BY_MDS           
                err = sendCmd(command, false);
#else                
                err = sendMDSCmd(command);
#endif
                if (err != 0)
                {
                    std::stringstream ss;
                    ss << "error sending command:'" << cmd << "' args:'" << std::string(args) << "' to:'" << path << "' err:" << err;
                    bundle_state.append_error(ss.str());
                    json_buf = bundle_state.getData()->getCompliantJSONString();
                    CALC_EXEC_TIME;
                    return CHAOS_DEV_CMD;
                }
            }
            bundle_state.reset();

            bundle_state.status(state);
            json_buf = bundle_state.getData()->getCompliantJSONString();
            return CHAOS_DEV_OK;
        }

        //	chaos::common::data::CDataWrapper*data = fetch((chaos::metadata_service_client::node_controller::DatasetDomain)atoi((char*) args));
        //json_buf = data->getCompliantJSONString();
        json_buf = fetchJson(atoi((char *)args));
        return CHAOS_DEV_OK;
    }
    catch (chaos::CException e)
    {
        bundle_state.append_error("error sending \"" + cmd + "\" args:\"" + std::string(args) + "\" to:" + path + " err:" + e.what());
        json_buf = bundle_state.getData()->getCompliantJSONString();
        return CHAOS_DEV_UNX;
    }
    catch (std::exception ee)
    {
        bundle_state.append_error("unexpected error sending \"" + cmd + "\" args:\"" + std::string(args) + "\" to:" + path + " err:" + ee.what());
        json_buf = bundle_state.getData()->getCompliantJSONString();
        return CHAOS_DEV_CMD;
    }
}

uint64_t ChaosController::checkHB()
{
    uint64_t h = 0;
    h = getState(state);
    //DBGET <<" HB timestamp:"<<h<<" state:"<<state;
    if (h == 0)
    {
        //bundle_state.append_error("cannot access to HB");
        wostate = 1;
        state = chaos::CUStateKey::START;
        return 0;
    }
    if ((heart > 0) && ((h - heart) == 0))
    {
        std::stringstream ss;
        ss << "device is dead, last HB " << h << "us , removing";
        bundle_state.append_error(ss.str());
        return 0;
    }
    heart = h;
    return h;
}
int ChaosController::updateState()
{
    uint64_t h;
    last_state = state;
    h = getState(state);
    //DBGET <<" HB timestamp:"<<h<<" state:"<<state;
    if (h == 0)
    {
        //bundle_state.append_error("cannot access to HB");
        wostate = 1;
        state = chaos::CUStateKey::START;
        return -1;
    }
    bundle_state.reset();

    bundle_state.status(state);

    return (int)state;
}

boost::shared_ptr<chaos::common::data::CDataWrapper> ChaosController::normalizeToJson(chaos::common::data::CDataWrapper *src, std::map<std::string, int> &list)
{
    boost::shared_ptr<chaos::common::data::CDataWrapper> data_res(new CDataWrapper());

    if (list.empty())
    {
        data_res->appendAllElement(*src);
        return data_res;
    }

    std::vector<std::string> contained_key;
    std::map<std::string, int>::iterator rkey;
    src->getAllKey(contained_key);

    for (std::vector<std::string>::iterator k = contained_key.begin(); k != contained_key.end(); k++)
    {
        if ((rkey = list.find(*k)) != list.end())
        {
            if (rkey->second == chaos::DataType::SUB_TYPE_DOUBLE)
            {
                //        CUIServerLDBG_ << " replace data key:"<<rkey->first;
                int cnt;
                double *data = (double *)src->getRawValuePtr(rkey->first);
                int size = src->getValueSize(rkey->first);
                int elems = size / sizeof(double);
                for (cnt = 0; cnt < elems; cnt++)
                {
                    //  if(data[cnt]<1.0e-308)data[cnt]=1.0e-208;
                    //  else
                    //    if(data[cnt]>1.0e+308)data[cnt]=1.0e+308;
                    data_res->appendDoubleToArray(data[cnt]);
                }
                data_res->finalizeArrayForKey(rkey->first);
            }
            else if (rkey->second == chaos::DataType::SUB_TYPE_INT32)
            {
                int cnt;
                int32_t *data = (int32_t *)src->getRawValuePtr(rkey->first);
                int size = src->getValueSize(rkey->first);
                int elems = size / sizeof(int32_t);
                for (cnt = 0; cnt < elems; cnt++)
                {
                    data_res->appendInt32ToArray(data[cnt]);
                }
                data_res->finalizeArrayForKey(rkey->first);
            }
            else if (rkey->second == chaos::DataType::SUB_TYPE_INT64)
            {
                int cnt;
                int64_t *data = (int64_t *)src->getRawValuePtr(rkey->first);
                int size = src->getValueSize(rkey->first);
                int elems = size / sizeof(int64_t);
                for (cnt = 0; cnt < elems; cnt++)
                {
                    data_res->appendInt64ToArray(data[cnt]);
                    //  data_res->appendDoubleToArray(data[cnt]);
                }
                data_res->finalizeArrayForKey(rkey->first);
            } /*else if(rkey->second ==chaos::DataType::TYPE_INT64) {
          data_res->addDoubleValue(rkey->first,(double)src->getInt64Value(rkey->first));
          }*/
            else
            {
                // LDBG_ << "adding not translated key:"<<*k<<" json:"<<src->getCSDataValue(*k)->getCompliantJSONString();
                data_res->appendAllElement(*src->getCSDataValue(*k));
                src->copyKeyTo(*k, *data_res.get());
            }
        }
        else
        {
            //LDBG_ << "adding normal key:"<<*k<<" json:"<<src->getCSDataValue(*k)->getCompliantJSONString();
            src->copyKeyTo(*k, *data_res.get());
        }
    }
    return data_res;
}

ChaosController::dev_info_status::dev_info_status()
{
    reset();
}

void ChaosController::dev_info_status::status(chaos::CUStateKey::ControlUnitState deviceState)
{
    dev_status.str("");
    if (deviceState == chaos::CUStateKey::DEINIT)
    {
     dev_status<<   "deinit";
        
    }
    else if (deviceState == chaos::CUStateKey::INIT)
    {
        dev_status<< "init";
    }
    else if (deviceState == chaos::CUStateKey::START)
    {
        dev_status<<"start";
    }
    else if (deviceState == chaos::CUStateKey::STOP)
    {
        dev_status<< "stop";
    }
    else if (deviceState == chaos::CUStateKey::FATAL_ERROR)
    {
        dev_status<< "fatal error";
    }
    else if (deviceState == chaos::CUStateKey::RECOVERABLE_ERROR)
    {
        dev_status<< "recoverable error";
    }
    else
    {
        dev_status<< "uknown";
    }
}

void ChaosController::dev_info_status::append_log(const std::string& log)
{
    CTRLDBG_ << log;
    log_status<<log;
}

void ChaosController::dev_info_status::reset()
{
    dev_status.str("");
    error_status.str("");
    log_status.str("");
}

void ChaosController::dev_info_status::append_error(const std::string& log)
{
    CTRLERR_ << log;
    error_status<<log;
}
int ChaosController::getSnapshotsofCU(const std::string &cuname, std::map<uint64_t, std::string> &res)
{
    std::vector<std::string> node_found;
    if (mdsChannel->searchSnapshotForNode(cuname, node_found, MDS_TIMEOUT) == 0)
    {
        for (std::vector<std::string>::iterator i = node_found.begin(); i != node_found.end(); i++)
        {
            DBGET << "snapshot for node '" << cuname << "' found:" << *i;
            if (mdsChannel->searchSnapshot(*i, res, MDS_TIMEOUT) != 0)
            {
                CTRLERR_ << "get internal snapshot " << *i << " NOT found";

                return -2;
            }
        }
    }
    else
    {
        CTRLERR_ << " No snapshot for node '" << cuname;
        return -1;
    }
    return 0;
}

chaos::common::data::CDataWrapper *ChaosController::dev_info_status::getData()
{
    data_wrapper.reset();
    data_wrapper.addStringValue("dev_status", dev_status.str());
    data_wrapper.addStringValue("log_status", log_status.str());
    data_wrapper.addStringValue("error_status", error_status.str());
    dev_status.str("");
    log_status.str("");
    error_status.str("");
    return &data_wrapper;
}
chaos::NodeType::NodeSearchType ChaosController::human2NodeType(const std::string& what){
        chaos::NodeType::NodeSearchType node_type=chaos::NodeType::NodeSearchType::node_type_cu;


            if (what == "agent")
                node_type = chaos::NodeType::NodeSearchType::node_type_agent;
            if (what == "us")
                node_type = chaos::NodeType::NodeSearchType::node_type_us;
            if (what == "webui")
                node_type = chaos::NodeType::NodeSearchType::node_type_wan;
            if (what == "mds")
                node_type = chaos::NodeType::NodeSearchType::node_type_cds;
            if (what == "variable")
                node_type = chaos::NodeType::NodeSearchType::node_type_variable;
            if (what == "tag")
                node_type = chaos::NodeType::NodeSearchType::node_type_tag;
            if (what == "server")
                node_type = chaos::NodeType::NodeSearchType::node_type_all_server;
            if (what == "root")
                node_type = chaos::NodeType::NodeSearchType::node_type_root;

            
    return node_type;
}
chaos::common::data::VectorCDWUniquePtr ChaosController::getNodeInfo(const std::string& search,const std::string& what,bool alive){
    ChaosStringVector node_found;
    int reti;
    chaos::common::data::VectorCDWUniquePtr ret;
        chaos::NodeType::NodeSearchType node_type=human2NodeType(what);

      DBGET<< "search "<<what<<"("<<node_type<<") with key:"<<search;
      if (mdsChannel->searchNode(search, node_type, alive, 0, MAX_QUERY_ELEMENTS, node_found, MDS_TIMEOUT) != 0){
           CTRLERR_ <<"Nothing found for search \""<<search<<" type:" <<what;
                
          return ret;
    }
    for(ChaosStringVector::iterator i=node_found.begin();i!=node_found.end();i++){
        EXECUTE_CHAOS_RET_API(reti,api_proxy::node::GetNodeDescription, MDS_TIMEOUT, *i);
        if(reti==0){
            ret.push_back(apires->detachResult());
        }
    }
    return ret;

}
chaos::common::data::CDWUniquePtr ChaosController::sendRPCMsg(const std::string& uid,const std::string& domain,const std::string&rpcmsg,CDWUniquePtr& data_pack){

    chaos::common::data::VectorCDWUniquePtr node=getNodeInfo(uid,"cu",false);
    auto message_channel=NetworkBroker::getInstance()->getRawMessageChannel();
    for (chaos::common::data::VectorCDWUniquePtr::iterator i=node.begin();i!=node.end();i++){
         if((*i)->hasKey(chaos::NodeDefinitionKey::NODE_RPC_ADDR)){

         
            std::string remote_host=(*i)->getStringValue("ndk_rpc_addr");
            std::string node_id=(*i)->getStringValue("ndk_uid");

            DBGET << "sending domain:"<<domain<<"action:"<< rpcmsg<<" to:" << remote_host<<" uid:"<<node_id;
  
            ChaosUniquePtr<MessageRequestFuture>  fut=message_channel->sendRequestWithFuture(remote_host,
                                                                                domain,
                                                                                 rpcmsg,
                                                                                 MOVE(data_pack));
                if(rpcmsg==chaos::NodeDomainAndActionRPC::ACTION_NODE_SHUTDOWN){
                    DBGET << "SENT IMMEDIATE \""<< rpcmsg<<"\" to:" << remote_host<<" uid:"<<node_id;

                    return chaos::common::data::CDWUniquePtr();
                }
                fut->wait(MDS_TIMEOUT);
                if(fut->getError()==0){
                    return fut->detachResult();
                } else {
                    DBGET << "Error sending command \""<< rpcmsg<<"\" to:" << remote_host<<" uid:"<<node_id << " error:"<<fut->getError();

                }
                


            }
    }
    return chaos::common::data::CDWUniquePtr();

}

chaos::common::data::CDWUniquePtr ChaosController::sendRPCMsg(const std::string& search,const std::string&rpcmsg,CDWUniquePtr data_pack,const std::string& what,bool alive){
    chaos::common::data::CDWUniquePtr infos(new CDataWrapper());

    chaos::common::data::VectorCDWUniquePtr node=getNodeInfo(search,what,alive);
    auto message_channel=NetworkBroker::getInstance()->getRawMessageChannel();
    for (chaos::common::data::VectorCDWUniquePtr::iterator i=node.begin();i!=node.end();i++){
         DBGET << what << " INFO:" << (*i)->getCompliantJSONString();
         if((*i)->hasKey(chaos::NodeDefinitionKey::NODE_RPC_ADDR)){

         
            std::string remote_host=(*i)->getStringValue("ndk_rpc_addr");
            std::string node_id=(*i)->getStringValue("ndk_uid");

               
            ChaosUniquePtr<MessageRequestFuture>  fut=message_channel->sendRequestWithFuture(remote_host,
                                                                                chaos::NodeDomainAndActionRPC::RPC_DOMAIN,
                                                                                 rpcmsg,
                                                                                 MOVE(data_pack));
                if(rpcmsg==chaos::NodeDomainAndActionRPC::ACTION_NODE_SHUTDOWN){
                    DBGET << "SENT IMMEDIATE \""<< rpcmsg<<"\" to:" << remote_host<<" uid:"<<node_id;

                    return infos;
                }
                fut->wait(MDS_TIMEOUT);
                if(fut->getError()==0){
                    (*i)->addCSDataValue(node_id,*fut->detachResult().get());
                } else {
                    DBGET << "Error sending command \""<< rpcmsg<<"\" to:" << remote_host<<" uid:"<<node_id << " error:"<<fut->getError();

                }
                

                infos->appendCDataWrapperToArray(*i->get());

            }
    }
    infos->finalizeArrayForKey("info");
    NetworkBroker::getInstance()->disposeMessageChannel(message_channel);
        
    return infos;
}

chaos::common::data::CDWUniquePtr ChaosController::getBuildProcessInfo(const std::string& search,const std::string& what,bool alive){
    chaos::common::data::CDWUniquePtr infos(new CDataWrapper());
    chaos::common::data::VectorCDWUniquePtr agent=getNodeInfo(search,what,alive);
    auto message_channel=NetworkBroker::getInstance()->getRawMessageChannel();

    for (chaos::common::data::VectorCDWUniquePtr::iterator i=agent.begin();i!=agent.end();i++){

        CDataWrapper agent;
         DBGET << what << " INFO:" << (*i)->getCompliantJSONString();
         if((*i)->hasKey(chaos::NodeDefinitionKey::NODE_RPC_ADDR)){

         
            std::string remote_host=(*i)->getStringValue("ndk_rpc_addr");
            std::string node_id=(*i)->getStringValue("ndk_uid");

            CDWUniquePtr data_pack;
               
            ChaosUniquePtr<MessageRequestFuture>  fut=message_channel->sendRequestWithFuture(remote_host,
                                                                                chaos::NodeDomainAndActionRPC::RPC_DOMAIN,
                                                                                 chaos::NodeDomainAndActionRPC::ACTION_GET_BUILD_INFO,
                                                                                 MOVE(data_pack));
                fut->wait(MDS_TIMEOUT);
                if(fut->getError()==0){
                    (*i)->addCSDataValue("build",*fut->detachResult().get());
                } else {
                    DBGET << "Error sending command to:" << remote_host<<" uid:"<<node_id << " error:"<<fut->getError();

                }
                ChaosUniquePtr<MessageRequestFuture>  fut2=message_channel->sendRequestWithFuture(remote_host,
                                                                                chaos::NodeDomainAndActionRPC::RPC_DOMAIN,
                                                                                 chaos::NodeDomainAndActionRPC::ACTION_GET_PROCESS_INFO,
                                                                                 MOVE(data_pack));
                fut2->wait(MDS_TIMEOUT);
                if(fut2->getError()==0){
                    (*i)->addCSDataValue("process",*fut2->detachResult().get());
                } else {
                    DBGET << "Error sending command to:" << remote_host<<" uid:"<<node_id << " error:"<<fut2->getError();

                }


                infos->appendCDataWrapperToArray(*i->get());

            }
    }
    infos->finalizeArrayForKey("info");
    NetworkBroker::getInstance()->disposeMessageChannel(message_channel);
        
    return infos;
}

