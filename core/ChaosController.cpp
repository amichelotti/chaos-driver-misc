/*
 * File:   ChaosController.cpp
 * Author: michelo
 *
 * Created on September 2, 2015, 5:29 PM
 */

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
using namespace chaos::cu::data_manager;
using namespace chaos::common::data;
using namespace chaos::metadata_service_client;

using namespace ::driver::misc;

#define DBGET CTRLDBG_ << "[" << getPath() << "]"
#define DBGETERR CTRLERR_ << "[" << getPath() << "]"

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
    controller->setRequestTimeWaith(timeo_us / 1000);
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
            controller->initDevice();
            break;

        case chaos::CUStateKey::INIT:
            switch (dstState)
            {
            case chaos::CUStateKey::DEINIT:
                DBGET << "[init] apply \"deinit\" ";
                controller->deinitDevice();
                break;
            case chaos::CUStateKey::START:
            case chaos::CUStateKey::STOP:
                DBGET << "[init] apply \"start\"";
                ;
                controller->startDevice();
                break;
            }

            break;

        case chaos::CUStateKey::START:
            DBGET << "[start] apply \"stop\"";
            controller->stopDevice();
            break;

        case chaos::CUStateKey::STOP:
            switch (dstState)
            {
            case chaos::CUStateKey::DEINIT:
            case chaos::CUStateKey::INIT:
                DBGET << "[stop] apply \"deinit\"";
                controller->deinitDevice();
                break;
            case chaos::CUStateKey::START:
                DBGET << "[stop] apply \"start\"";
                controller->startDevice();
                break;
            }

            break;
        default:
            return 0;
            // controller->deinitDevice();
            // controller->initDevice();
            /*
                 switch(dstState){
                 case chaos::CUStateKey::DEINIT:
                 break;
                 case chaos::CUStateKey::INIT:
                 break;
                 case chaos::CUStateKey::START:
                 controller->startDevice();
                 break;
                 case chaos::CUStateKey::STOP:
                 controller->stopDevice();
                 break;
                 }*/
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
    return controller->initDevice();
}

int ChaosController::stop(int force)
{
    if (force)
    {
        return forceState(chaos::CUStateKey::STOP);
    }
    return controller->stopDevice();
}

int ChaosController::start(int force)
{
    if (force)
    {
        return forceState(chaos::CUStateKey::START);
    }
    return controller->startDevice();
}

int ChaosController::deinit(int force)
{
    if (force)
    {
        return forceState(chaos::CUStateKey::DEINIT);
    }
    return controller->deinitDevice();
}

uint64_t ChaosController::getState(chaos::CUStateKey::ControlUnitState &stat)
{
    uint64_t ret = 0;
    CDataWrapper *tmp = controller->getCurrentDatasetForDomain(KeyDataStorageDomainHealth).get();
    stat = chaos::CUStateKey::UNDEFINED;
    if (tmp && tmp->hasKey(chaos::NodeHealtDefinitionKey::NODE_HEALT_STATUS))
    {
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

uint64_t ChaosController::getTimeStamp()
{
    uint64_t ret;
    controller->getTimeStamp(ret);
    return ret;
}

ChaosController::command_t ChaosController::prepareCommand(std::string alias)
{
    ChaosController::command_t cmd = boost::shared_ptr<command>(new command());
    cmd->alias = alias;
    return cmd;
}

int ChaosController::setSchedule(uint64_t us)
{

    schedule = us;
    return controller->setScheduleDelay(us);
}

std::string ChaosController::getJsonState()
{
    std::string ret;
    ret = bundle_state.getData()->getCompliantJSONString();
    return ret;
}

int ChaosController::init(std::string p, uint64_t timeo_)
{

    path = p;
    state = chaos::CUStateKey::UNDEFINED;
    schedule = 0;

    bundle_state.reset();
    bundle_state.status(state);
    DBGET << "init CU NAME:\"" << path << "\""
          << " timeo:" << timeo_;
    last_access = reqtime = tot_us = naccess = refresh = 0;

    /*
     if (controller != NULL) {

        DBGET << " removing existing controller";

        ChaosMetadataServiceClient::getInstance()->deleteCUController(controller);
        controller = NULL;
    }
    */
    ChaosWriteLock ll(ioctrl);

    try
    {
        if (controller == NULL)
        {
            ChaosMetadataServiceClient::getInstance()->getNewCUController(path, &controller);
        }
        else {
            controller->updateChannel();
        }
        
    }
    catch (chaos::CException &e)
    {
        std::stringstream ss;
        ss << "Exception during get controller for device:\"" << path << "\" ex: \"" << e.what() << "\"";
        bundle_state.append_error(ss.str());
        CTRLERR_ << "Exception during get controller for device:" << e.what();
        return -3;
    }
    if (controller == NULL)
    {
        CTRLERR_ << "bad handle for controller " << path;
        return -1;
    }
    controller->setRequestTimeWaith(timeo_ / 1000);
    controller->setupTracking();
    timeo = timeo_;
    wostate = 0;

    setUid(path);

    controller->fetchCurrentDatatasetFromDomain(KeyDataStorageDomainInput);
    controller->fetchCurrentDatatasetFromDomain(KeyDataStorageDomainOutput);
    controller->fetchCurrentDatatasetFromDomain(KeyDataStorageDomainHealth);
    controller->fetchCurrentDatatasetFromDomain(KeyDataStorageDomainSystem);
    controller->fetchCurrentDatatasetFromDomain(KeyDataStorageDomainCustom);
    controller->fetchCurrentDatatasetFromDomain(KeyDataStorageDomainDevAlarm);
    controller->fetchCurrentDatatasetFromDomain(KeyDataStorageDomainCUAlarm);
    for (int cnt = 0; cnt <= DPCK_LAST_DATASET_INDEX; cnt++)
    {
        last_ts[cnt] = 0;
        last_pckid[cnt] = 0;
    }
    delta_update = 0;
    if (getState(state) == 0)
    {
        DBGET << "Uknown state for device assuming wostate";
        wostate = 1;
    }
    std::vector<chaos::common::data::RangeValueInfo> vi = controller->getDeviceValuesInfo();
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
    chaos::common::data::CDataWrapper *dataWrapper;
    if (wostate)
    {
        dataWrapper = controller->getCurrentDatasetForDomain(KeyDataStorageDomainOutput).get();
    }
    else
    {
        dataWrapper = controller->getCurrentDatasetForDomain(KeyDataStorageDomainSystem).get();
    }

    if (dataWrapper)
    {
        json_dataset = dataWrapper->getCompliantJSONString();
    }
    else
    {
        CTRLERR_ << "cannot retrieve system dataset from " << path;
        return -3;
    }
    DBGET << "initalization ok handle:" << (void *)controller;

    last_access = boost::posix_time::microsec_clock::local_time().time_of_day().total_microseconds();

    return 0;
}

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
int ChaosController::sendMDSCmd(command_t& cmd){
    CDWUniquePtr local_command_pack(new CDataWrapper());
    CDWUniquePtr result_data;
    
    local_command_pack->appendAllElement(cmd->param);
    
    
    // set the default slow command information
    local_command_pack->addStringValue(chaos::common::batch_command::BatchCommandAndParameterDescriptionkey::BC_ALIAS, cmd->alias);
    local_command_pack->addInt32Value(chaos::common::batch_command::BatchCommandSubmissionKey::SUBMISSION_RULE_UI32, (uint32_t) cmd->sub_rule);
    local_command_pack->addInt32Value(chaos::common::batch_command::BatchCommandSubmissionKey::SUBMISSION_PRIORITY_UI32, (uint32_t) cmd->priority);
    
    local_command_pack->addInt64Value(chaos::common::batch_command::BatchCommandSubmissionKey::SCHEDULER_STEP_TIME_INTERVALL_UI64, cmd->scheduler_steps_delay);
    local_command_pack->addInt32Value(chaos::common::batch_command::BatchCommandSubmissionKey::SUBMISSION_RETRY_DELAY_UI32,  cmd->submission_checker_steps_delay);
    
    //err = deviceChannel->setAttributeValue(local_command_pack, false, millisecToWait);
    local_command_pack->addStringValue(chaos::NodeDefinitionKey::NODE_UNIQUE_ID, path);
    chaos::metadata_service_client::api_proxy::ApiProxyResult apires = GET_CHAOS_API_PTR(chaos::metadata_service_client::api_proxy::node::CommandTemplateSubmit)->execute(local_command_pack); 
    apires->setTimeout(5000);                                                                                         
    apires->wait();                                                                                                   
    if (apires->getError() ){  
        return -1;                            
    }
    return 0;
}
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
    CTRLAPP_ << "sending command \"" << cmd->alias << "\" params:" << cmd->param.getCompliantJSONString();
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
                controller->releaseQuery(i->second.qc);
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
int ChaosController::executeCmd(command_t &cmd, bool wait, uint64_t perform_at, uint64_t wait_for)
{
    int ret = sendCmd(cmd, wait, perform_at, wait_for);
    if (ret != 0)
    {
        // retry to update channel
        CTRLERR_ << "error sending command to:" << path << " update controller";

        if (init(path, timeo) == 0)
        {
            ret = sendCmd(cmd, wait, perform_at, wait_for);
        }
        else
        {
            CTRLERR_ << "cannot reinitialize controller:" << path;
        }
        return ret;
    }
    last_cmd = cmd;
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
    return ret;
}

ChaosController::ChaosController(std::string p, uint32_t timeo_) : common::misc::scheduler::SchedTimeElem(p, 0), timeo(timeo_)
{
    int ret;
    controller = NULL;
    heart = 0;
    mdsChannel = NetworkBroker::getInstance()->getMetadataserverMessageChannel();
    if (!mdsChannel)
        throw chaos::CException(-1, "No MDS Channel created", "ChaosController()");

    if ((ret = init(p, timeo_)) != 0)
    {
        throw chaos::CException(ret, "cannot allocate controller for:" + path + " check if exists", __FUNCTION__);
    }
    cachedChannel_v.resize(DPCK_LAST_DATASET_INDEX + 1);

    /*db = ::common::misc::data::DBbaseFactory::getInstance(DEFAULT_DBTYPE,DEFAULT_DBNAME);
     db->setDBParameters("replication",DEFAULT_DBREPLICATION);

     db->addDBServer("127.0.0.1");
     if(db->connect()==0){
     DPRINT("connected to cassandra");
     } else {
     ERR("cannot connect to cassandra");
     }*/
    for (int cnt = -1; cnt <= DPCK_LAST_DATASET_INDEX; cnt++)
    {
        cachedJsonChannels[cnt] = "{}";
    }
    initializeClient();
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
    {
        ChaosReadLock ll(ioctrl);
        controller->fetchAllDataset();
    }
    ChaosSharedPtr<chaos::common::data::CDataWrapper> channels[DPCK_LAST_DATASET_INDEX + 1];
    delta_update = CU_HEALTH_UPDATE_US;
    for (int cnt = 0; cnt <= DPCK_LAST_DATASET_INDEX; cnt++)
    {
        ChaosReadLock ll(ioctrl);
        channels[cnt] = controller->getCurrentDatasetForDomain(static_cast<chaos::cu::data_manager::KeyDataStorageDomain>(cnt));

        uint64_t tss, pckid;
        if (channels[cnt]->hasKey("ndk_uid") && (channels[cnt]->getStringValue("ndk_uid") != path))
        {
            DBGETERR << "REALLIGN with CU RESTART because name mismatch \"" + channels[cnt]->getStringValue("ndk_uid") + "\" should be:\"" + path + "\"";
            init(path, timeo);
            continue;
        }
        /*if(channels[cnt]->hasKey(chaos::DataPackCommonKey::DPCK_SEQ_ID)){
            pckid=channels[cnt]->getInt64Value(chaos::DataPackCommonKey::DPCK_SEQ_ID);
            if(pckid<last_pckid[cnt]){
                DBGETERR<<"REALLIGN with CU RESTART because pckid:"<<pckid<<" less than last:"<<last_pckid[cnt];
                init(path,timeo);
                continue;
            }
            last_pckid[cnt]=pckid;
        }*/
        if (!channels[cnt]->hasKey(chaos::DataPackCommonKey::DPCK_TIMESTAMP))
        {
            continue;
        }
        ChaosWriteLock l(iomutex);

        //if(channels[cnt]->hasKey("ndk_uid")&&(channels[cnt]->getString("ndk_uid")!=controller->)
        if(channels[cnt].get()){
            std::string tmp= channels[cnt]->getCompliantJSONString();
            if(tmp.size()<2){
                cachedJsonChannels[cnt] ="{}";
            } else {
                cachedJsonChannels[cnt] =tmp;
                all.addCSDataValue(chaos::datasetTypeToHuman(cnt), *(channels[cnt].get()));

            }
            
        }
        if ((static_cast<chaos::cu::data_manager::KeyDataStorageDomain>(cnt) == KeyDataStorageDomainHealth) ||
            (static_cast<chaos::cu::data_manager::KeyDataStorageDomain>(cnt) == KeyDataStorageDomainSystem) ||
            (static_cast<chaos::cu::data_manager::KeyDataStorageDomain>(cnt) == KeyDataStorageDomainDevAlarm) ||
            (static_cast<chaos::cu::data_manager::KeyDataStorageDomain>(cnt) == KeyDataStorageDomainCUAlarm))
        {
            if(channels[cnt].get()){

                common.addCSDataValue(chaos::datasetTypeToHuman(cnt), *(channels[cnt].get()));
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
    float rate;
    if (channels[chaos::DataPackCommonKey::DPCK_DATASET_TYPE_HEALTH]->hasKey(chaos::ControlUnitHealtDefinitionValue::CU_HEALT_OUTPUT_DATASET_PUSH_RATE) &&
        ((rate = channels[chaos::DataPackCommonKey::DPCK_DATASET_TYPE_HEALTH]->getDoubleValue(chaos::ControlUnitHealtDefinitionValue::CU_HEALT_OUTPUT_DATASET_PUSH_RATE) > 0)))
    {
        delta_update = (1000 * 1000.0) / (2 * rate);
    }
    else if (channels[chaos::DataPackCommonKey::DPCK_DATASET_TYPE_SYSTEM]->hasKey(chaos::ControlUnitDatapackSystemKey::THREAD_SCHEDULE_DELAY))
    {
        delta_update = channels[chaos::DataPackCommonKey::DPCK_DATASET_TYPE_SYSTEM]->getDoubleValue(chaos::ControlUnitDatapackSystemKey::THREAD_SCHEDULE_DELAY) / 2.0;
    }
    delta_update = std::min(delta_update, (uint64_t)CU_HEALTH_UPDATE_US);

    //DBGET<<"SCHED STOP delta update:"<<delta_update;
    return delta_update;
}

ChaosSharedPtr<chaos::common::data::CDataWrapper> ChaosController::getLiveChannel(std::string &key, int domain)
{
    size_t value_len = 0;
    ChaosSharedPtr<chaos::common::data::CDataWrapper> ret;
    std::string lkey = key + chaos::datasetTypeToPostfix(domain);
    char *value = live_driver->retriveRawData(lkey, (size_t *)&value_len);
    if (value)
    {
        chaos::common::data::CDataWrapper *tmp = new CDataWrapper(value);
        ret.reset(tmp);
        free(value);
        return ret;
    }
    return ret;
}
chaos::common::data::VectorCDWShrdPtr ChaosController::getLiveChannel(std::vector<string> &keys, int domain)
{
    chaos::common::data::VectorCDWShrdPtr results;
    std::vector<std::string> channels;
    for (std::vector<std::string>::iterator i = keys.begin(); i != keys.end(); i++)
    {
        channels.push_back(*i + chaos::datasetTypeToPostfix(domain));
    }
    live_driver->retriveMultipleData(channels, results);
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
    live_driver->retriveMultipleData(channels, results);
    return results;
}

ChaosController::ChaosController() : common::misc::scheduler::SchedTimeElem("none", 0), controller(NULL), state(chaos::CUStateKey::UNDEFINED), queryuid(0), last_access(0), heart(0), reqtime(0), tot_us(0), naccess(0), refresh(0), timeo(DEFAULT_TIMEOUT_FOR_CONTROLLER)
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
     ChaosMetadataServiceClient::getInstance()->deleteCUController(controller);

    if (mdsChannel)
    {
        NetworkBroker::getInstance()->disposeMessageChannel(mdsChannel);
        mdsChannel = NULL;
    }
   
    
    controller = NULL;
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
            data = normalizeToJson(i->second, binaryToTranslate);
            //out<<",\"input\":"<<data->getCompliantJSONString();
            resdata.addCSDataValue(chaos::datasetTypeToHuman(i->first), *(data.get()));
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

boost::shared_ptr<chaos::common::data::CDataWrapper> ChaosController::fetch(int channel)
{
    //	boost::mutex::scoped_lock(iomutex);

    boost::shared_ptr<chaos::common::data::CDataWrapper> retdata;
    try
    {
        if (channel == -1)
        {
            chaos::common::data::CDataWrapper *idata = NULL, *odata = NULL;
            chaos::common::data::CDataWrapper resdata;
            std::stringstream out;
            uint64_t ts = 0;
            std::map<int, chaos::common::data::CDataWrapper *> set;
            CDataWrapper ch[7];
            controller->getCurrentDatasetForDomain(KeyDataStorageDomainInput, &ch[0]);
            controller->getCurrentDatasetForDomain(KeyDataStorageDomainOutput, &ch[1]);
            controller->getCurrentDatasetForDomain(KeyDataStorageDomainHealth, &ch[2]);
            controller->getCurrentDatasetForDomain(KeyDataStorageDomainSystem, &ch[3]);
            controller->getCurrentDatasetForDomain(KeyDataStorageDomainCustom, &ch[4]);
            controller->getCurrentDatasetForDomain(KeyDataStorageDomainDevAlarm, &ch[5]);
            controller->getCurrentDatasetForDomain(KeyDataStorageDomainCUAlarm, &ch[6]);
            set[KeyDataStorageDomainInput] = &ch[0];
            set[KeyDataStorageDomainOutput] = &ch[1];
            set[KeyDataStorageDomainHealth] = &ch[2];
            set[KeyDataStorageDomainSystem] = &ch[3];
            set[KeyDataStorageDomainCustom] = &ch[4];
            set[KeyDataStorageDomainDevAlarm] = &ch[5];
            set[KeyDataStorageDomainCUAlarm] = &ch[6];

            retdata = combineDataSets(set);
        }
        else if (channel == 255)
        {
            chaos::common::data::CDataWrapper *idata = NULL, *odata = NULL;
            chaos::common::data::CDataWrapper resdata;
            std::stringstream out;
            uint64_t ts = 0;
            std::map<int, chaos::common::data::CDataWrapper *> set;
            CDataWrapper ch[7];
            controller->getCurrentDatasetForDomain(KeyDataStorageDomainHealth, &ch[0]);
            controller->getCurrentDatasetForDomain(KeyDataStorageDomainSystem, &ch[1]);
            controller->getCurrentDatasetForDomain(KeyDataStorageDomainDevAlarm, &ch[2]);
            controller->getCurrentDatasetForDomain(KeyDataStorageDomainCUAlarm, &ch[3]);
            set[KeyDataStorageDomainHealth] = &ch[0];
            set[KeyDataStorageDomainSystem] = &ch[1];
            set[KeyDataStorageDomainDevAlarm] = &ch[2];
            set[KeyDataStorageDomainCUAlarm] = &ch[3];

            retdata = combineDataSets(set);
        }
        else
        {
            CDataWrapper data;

            if (controller->getCurrentDatasetForDomain((chaos::metadata_service_client::node_controller::DatasetDomain)channel, &data) != 0)
            {
                std::stringstream ss;
                retdata.reset(new CDataWrapper());
                ss << "error fetching data from channel " << channel;
                bundle_state.append_error(ss.str());
                retdata->appendAllElement(*bundle_state.getData());
                return retdata;
            }
            retdata = normalizeToJson(&data, binaryToTranslate);
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
    if (retdata.get())
    {
        retdata->appendAllElement(*bundle_state.getData());
    }
    else
    {
        retdata.reset(new CDataWrapper());
        retdata->appendAllElement(*bundle_state.getData());
    }
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

#define EXECUTE_CHAOS_API(api_name, time_out, ...)                                                                        \
    DBGET << " "                                                                                                          \
          << " Executing Api:\"" << #api_name << "\"";                                                                    \
    chaos::metadata_service_client::api_proxy::ApiProxyResult apires = GET_CHAOS_API_PTR(api_name)->execute(__VA_ARGS__); \
    apires->setTimeout(time_out);                                                                                         \
    apires->wait();json_buf = "{}";                                                                                                      \
    if (apires->getError() )                                                                                               \
    {                                                                                                                     \
        std::stringstream ss;                                                                                             \
        ss << " error in :" << __FUNCTION__ << "|" << __LINE__ << "|" << #api_name << " " << apires->getErrorMessage();   \
        bundle_state.append_error(ss.str());                                                                              \
        json_buf = bundle_state.getData()->getCompliantJSONString();                                                      \
    } else { chaos::common::data::CDataWrapper *r=apires->getResult();if((r!=NULL)) json_buf=r->getCompliantJSONString();}
uint64_t ChaosController::offsetToTimestamp(const std::string &off)
{
    boost::smatch what;
    boost::regex ts_ms("([0-9]{13})");

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
int32_t ChaosController::queryHistory(const std::string &start, const std::string &end, int channel, std::vector<boost::shared_ptr<CDataWrapper> > &res, int page){
    std::vector<std::string> tags;
    return queryHistory(start,end,tags, channel,res, page);
}
int32_t ChaosController::queryHistory(const std::string& start,const std::string& end,const std::vector<std::string>& tags, int channel,std::vector<boost::shared_ptr<chaos::common::data::CDataWrapper> >&res, int page){
    uint64_t start_ts = offsetToTimestamp(start);
    uint64_t end_ts = offsetToTimestamp(end);
    int32_t ret = 0, err = 0;
    chaos::common::io::QueryCursor *query_cursor = NULL;
    if (page == 0)
    {
        std::set<std::string> tagsv(tags.begin(),tags.end());
        controller->executeTimeIntervallQuery((chaos::metadata_service_client::node_controller::DatasetDomain)channel, start_ts, end_ts,tagsv, &query_cursor);
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
        controller->executeTimeIntervallQuery((chaos::metadata_service_client::node_controller::DatasetDomain)channel, start_ts, end_ts, &query_cursor, page);
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
        controller->releaseQuery(query_cursor);
        return -abs(err);
    }
    else
    {
        controller->releaseQuery(query_cursor);
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
                    controller->releaseQuery(query_cursor);
                    return -abs(err);
                }
                else
                {
                    boost::shared_ptr<CDataWrapper> cd = normalizeToJson(q_result.get(), binaryToTranslate);
                    res.push_back(cd);
                    if (!query_cursor->hasNext())
                    {
                        query_cursor_map.erase(query_cursor_map.find(uid));
                        controller->releaseQuery(query_cursor);
                        return 0;
                    }
                    return uid;
                }
            }
            else
            {
                query_cursor_map.erase(query_cursor_map.find(uid));
                controller->releaseQuery(query_cursor);
                return 0;
            }
        }
    }
    CTRLERR_ << " no cursor available with uid " << uid;

    return -1;
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
            if (what == "cu" || what == "us" || what == "agent")
            {
                json_buf = "[]";

                int node_type = 2;
                if (what == "agent")
                    node_type = 3;
                if (what == "us")
                    node_type = 1;

                if ((names.get()) && names->size())
                {
                    DBGET << "list nodes of type:" << node_type << "(" << what << ")";

                    for (int idx = 0; idx < names->size(); idx++)
                    {
                        ChaosStringVector node_tmp;

                        const std::string domain = names->getStringElementAtIndex(idx);
                        if (mdsChannel->searchNode(domain, node_type, alive, 0, MAX_QUERY_ELEMENTS, node_tmp, MDS_TIMEOUT) == 0)
                        {
                            node_found.insert(node_found.end(), node_tmp.begin(), node_tmp.end());
                        }
                    }
                    json_buf = vector2Json(node_found);
                    CALC_EXEC_TIME;
                    return CHAOS_DEV_OK;
                }
                else
                {
                    int err;
                    DBGET << "searching node \"" << name << "\" type:" << node_type << " (" << what << ")";

                    if ((err = mdsChannel->searchNode(name,
                                                      node_type,
                                                      alive,
                                                      0,
                                                      MAX_QUERY_ELEMENTS,
                                                      node_found,
                                                      MDS_TIMEOUT)) == 0)
                    {

                        json_buf = vector2Json(node_found);
                        CALC_EXEC_TIME;
                        return CHAOS_DEV_OK;
                    }
                    else
                    {
                        serr << "searching node: \"" << name << "\" err:" << err;
                    }
                }
            }
            else if (what == "zone")
            {
                json_buf = "[]";
                DBGET << "searching ZONE";

                ChaosStringVector dev_zone;
                if (mdsChannel->searchNode(name,
                                           2,
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
                        if (mdsChannel->searchNode(domain, 2, false, 0, MAX_QUERY_ELEMENTS, node_tmp, MDS_TIMEOUT) == 0)
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
                                               2,
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
                return CHAOS_DEV_OK;
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
                    return CHAOS_DEV_OK;
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
            if (node_type.empty())
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
                if (what == "health")
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

                    EXECUTE_CHAOS_API(api_proxy::node::GetNodeDescription, MDS_TIMEOUT, name);
                    res << json_buf;
                }
                else if (node_type == "us")
                {

                    if (what == "set")
                    {
                        EXECUTE_CHAOS_API(api_proxy::unit_server::GetSetFullUnitServer, MDS_TIMEOUT, name, 0, json_value.get());
                        res << json_buf;
                    }
                    else if (what == "del")
                    {
                        EXECUTE_CHAOS_API(api_proxy::unit_server::DeleteUS, MDS_TIMEOUT, name);
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
                                chaos::common::property::PropertyGroup pg(chaos::ControlUnitPropertyKey::GROUP_NAME);

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
                                          //  chaos::common::property::PropertyGroup pg(chaos::ControlUnitPropertyKey::GROUP_NAME);
                                            pg.addProperty(chaos::ControlUnitDatapackSystemKey::THREAD_SCHEDULE_DELAY, CDataVariant(static_cast<uint64_t>(prop->getInt64Value(chaos::ControlUnitDatapackSystemKey::THREAD_SCHEDULE_DELAY))));
                                        }
                                        if (prop.get() && prop->hasKey(chaos::ControlUnitPropertyKey::INIT_RESTORE_APPLY))
                                        {
                                            //chaos::common::property::PropertyGroup pg(chaos::ControlUnitPropertyKey::GROUP_NAME);
                                            pg.addProperty(chaos::ControlUnitPropertyKey::INIT_RESTORE_APPLY, CDataVariant(static_cast<bool>(prop->getBoolValue(chaos::ControlUnitPropertyKey::INIT_RESTORE_APPLY))));
                                        }
                                        if (prop.get() && prop->hasKey(chaos::ControlUnitPropertyKey::INIT_RESTORE_OPTION))
                                        {
                                            //chaos::common::property::PropertyGroup pg(chaos::ControlUnitPropertyKey::GROUP_NAME);
                                            pg.addProperty(chaos::ControlUnitPropertyKey::INIT_RESTORE_OPTION, CDataVariant(static_cast<int32_t>(prop->getInt32Value(chaos::ControlUnitPropertyKey::INIT_RESTORE_OPTION))));
                                        }
                                        if (prop.get() && prop->hasKey(chaos::DataServiceNodeDefinitionKey::DS_STORAGE_HISTORY_TIME))
                                        {
                                            //chaos::common::property::PropertyGroup pg(chaos::ControlUnitPropertyKey::GROUP_NAME);
                                            pg.addProperty(chaos::DataServiceNodeDefinitionKey::DS_STORAGE_HISTORY_TIME, CDataVariant(static_cast<uint64_t>(prop->getInt64Value(chaos::DataServiceNodeDefinitionKey::DS_STORAGE_HISTORY_TIME))));
                                        }

                                        if (prop.get() && prop->hasKey(chaos::DataServiceNodeDefinitionKey::DS_STORAGE_LIVE_TIME))
                                        {
                                           // chaos::common::property::PropertyGroup pg(chaos::ControlUnitPropertyKey::GROUP_NAME);
                                            pg.addProperty(chaos::DataServiceNodeDefinitionKey::DS_STORAGE_LIVE_TIME, CDataVariant(static_cast<uint64_t>(prop->getInt64Value(chaos::DataServiceNodeDefinitionKey::DS_STORAGE_LIVE_TIME))));
                                        }
                                        if (prop.get() && prop->hasKey(chaos::DataServiceNodeDefinitionKey::DS_STORAGE_TYPE))
                                        {
                                            //chaos::common::property::PropertyGroup pg(chaos::ControlUnitPropertyKey::GROUP_NAME);
                                            pg.addProperty(chaos::DataServiceNodeDefinitionKey::DS_STORAGE_TYPE, CDataVariant(static_cast<uint32_t>(prop->getInt32Value(chaos::DataServiceNodeDefinitionKey::DS_STORAGE_TYPE))));
                                        }
                                        if (prop.get() && prop->hasKey(chaos::DataServiceNodeDefinitionKey::DS_STORAGE_HISTORY_AGEING))
                                        {
                                           // chaos::common::property::PropertyGroup pg(chaos::ControlUnitPropertyKey::GROUP_NAME);
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
                            CHECK_PARENT;
                            {
                                if (json_value->hasKey("control_unit_implementation"))
                                {
                                    EXECUTE_CHAOS_API(api_proxy::unit_server::ManageCUType, MDS_TIMEOUT, parent, json_value->getStringValue("control_unit_implementation"), 0);
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

                        CHECK_PARENT;
                        {
                            EXECUTE_CHAOS_API(api_proxy::control_unit::DeleteInstance, MDS_TIMEOUT, parent, name);
                        }
                        {
                            EXECUTE_CHAOS_API(api_proxy::control_unit::Delete, MDS_TIMEOUT, name);
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
                        EXECUTE_CHAOS_API(chaos::metadata_service_client::api_proxy::agent::LoadNodeAssociation, MDS_TIMEOUT, name, parent);
                        chaos::common::data::CDataWrapper *r = apires->getResult();
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
                ret = CHAOS_DEV_OK;

            }
            json_buf = res.str();
            return ret;
        }
        else if (cmd == "log")
        {
            PARSE_QUERY_PARMS(args, true, true);
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
                chaos::common::data::CDataWrapper *r = apires->getResult();
                if (r)
                {
                    json_buf = r->getCompliantJSONString();
                }
                else
                {
                    json_buf = "[]";
                    CTRLERR_ << "an error arise during API:\"" << cmd << "\"";

                    return CHAOS_DEV_CMD;
                }
                return CHAOS_DEV_OK;
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
            else if (what == "load")
            {
                CHECK_VALUE_PARAM;
                CALL_CHAOS_API(chaos::metadata_service_client::api_proxy::script::LoadFullScript, MDS_TIMEOUT, json_value);
                chaos::common::data::CDataWrapper *r = apires->getResult();
                if (r)
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
                chaos::common::data::CDataWrapper *r = apires->getResult();
                if (r)
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

        if (controller == NULL)
        {
            CTRLERR_ << "no controller defined for cmd:\"" << cmd << "\"";

            return CHAOS_DEV_CMD;
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
            err = controller->initDevice();
            if (err != 0)
            {
                init(path, timeo);
                err = controller->initDevice();
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
            err = controller->startDevice();
            if (err != 0)
            {
                init(path, timeo);
                err = controller->startDevice();
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
            err = controller->stopDevice();
            if (err != 0)
            {
                init(path, timeo);
                err = controller->stopDevice();
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
            err = controller->deinitDevice();
            if (err != 0)
            {
                init(path, timeo);
                err = controller->deinitDevice();
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
            err = controller->setScheduleDelay(atoll((char *)args));
            if (err != 0)
            {
                init(path, timeo);
                err = controller->setScheduleDelay(atoll((char *)args));
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
            std::string ret = fetchJson(atoi((char *)args));
            json_buf = (ret.size() == 0) ? "{}" : ret;
            return CHAOS_DEV_OK;
        }
        else if (cmd == "readout" && (args != 0))
        {
            // bundle_state.append_log("return channel :" + parm);
            std::string var_name = args;
            boost::shared_ptr<chaos::common::data::CDataWrapper> data = fetch(KeyDataStorageDomainOutput);
            json_buf = dataset2Var(data.get(), var_name);
            return CHAOS_DEV_OK;
        }
        else if (cmd == "readin" && (args != 0))
        {
            // bundle_state.append_log("return channel :" + parm);
            std::string var_name = args;
            boost::shared_ptr<chaos::common::data::CDataWrapper> data = fetch(KeyDataStorageDomainInput);
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
                std::set<std::string> tags;
                if (p.hasKey("runid")){
                    runid = p.getInt64Value("runid");
                }
                if (p.hasKey("tags")){
                    if(p.isVector("tags")){
                        ChaosSharedPtr<CMultiTypeDataArrayWrapper> dw = p.getVectorValue("tags");
                        for(int cnt=0;cnt<dw->size();cnt++){
                            tags.insert(dw->getStringElementAtIndex(cnt));
                        }
                    } else if(p.isStringValue("tags")){
                        tags.insert(p.getStringValue("tags"));
                    }
                }
                DBGET << "START SEQ QUERY :" << std::dec << start_ts << " end:" << end_ts << "seq id " << seqid << " run id:" << runid << " page:" << page;

                controller->executeTimeIntervalQuery((chaos::metadata_service_client::node_controller::DatasetDomain)channel, start_ts, end_ts, seqid, runid, tags,&query_cursor, page);
                if (query_cursor)
                {
                    cnt = 0;
                    if(fmtType=="tgz"){
                        // all the elements into a tgz.
                    }
                    res << "{\"data\":[";
                    boost::shared_ptr<chaos::common::data::CDataWrapper> data;
                    if (query_cursor->hasNext())
                    {
                        uint32_t elems = query_cursor->size();
                        while (cnt < elems)
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
                            if (cnt < elems)
                            {
                                res << ",";
                            }
                        }
                    }
                    res << "]";

                    query_cursor->getIndexes(runid, seqid);
                    res << ",\"seqid\":" << seqid << ",\"runid\":" << runid << ",\"end\":" << ((query_cursor->hasNext()) ? 0 : 1) << "}";

                    controller->releaseQuery(query_cursor);
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

                    controller->executeTimeIntervallQuery((chaos::metadata_service_client::node_controller::DatasetDomain)channel, start_ts, end_ts, &query_cursor, page);
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
                                controller->releaseQuery(query_cursor);
                                bundle_state.append_error(CHAOS_FORMAT("error during query '%1' with  api error: %2%", % getPath() % err));
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
                            controller->releaseQuery(query_cursor);
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

                controller->executeTimeIntervallQuery((chaos::metadata_service_client::node_controller::DatasetDomain)channel, start_ts, end_ts, &query_cursor);
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
                controller->releaseQuery(query_cursor);
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
                            controller->releaseQuery(query_cursor);
                            query_cursor_map.erase(query_cursor_map.find(uid));
                            bundle_state.append_error(CHAOS_FORMAT("error during query '%1' with uid:%2% api error: %3%", % getPath() % uid % err));
                            json_buf = bundle_state.getData()->getCompliantJSONString();
                            CALC_EXEC_TIME;
                            //init(path, timeo);

                            return CHAOS_DEV_CMD;
                        }
                        res << ",\"uid\":0}";
                        DBGET << "queryhstnext no more pages items:" << cnt << " with uid:" << uid;
                        controller->releaseQuery(query_cursor);
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
                    controller->releaseQuery(query_cursor);
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
            ret = controller->createNewSnapshot(snapname, other);
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
            ret = controller->deleteSnapshot(snapname);
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
            controller->setRequestTimeWaith(50000);
            ret = controller->getSnapshotList(snaps);
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
                err = controller->setAttributeToValue(i->c_str(), param, false);
                if (err != 0)
                {
                    if (init(path, timeo) == 0)
                    {
                        DBGET << "error reapplying ...";
                        err = controller->setAttributeToValue(i->c_str(), param, false);
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
            err = controller->recoverDeviceFromError();
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
            err = controller->restoreDeviceToTag(args);
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
    if (deviceState == chaos::CUStateKey::DEINIT)
    {
        strcpy(dev_status, "deinit");
    }
    else if (deviceState == chaos::CUStateKey::INIT)
    {
        strcpy(dev_status, "init");
    }
    else if (deviceState == chaos::CUStateKey::START)
    {
        strcpy(dev_status, "start");
    }
    else if (deviceState == chaos::CUStateKey::STOP)
    {
        strcpy(dev_status, "stop");
    }
    else if (deviceState == chaos::CUStateKey::FATAL_ERROR)
    {
        strcpy(dev_status, "fatal error");
    }
    else if (deviceState == chaos::CUStateKey::RECOVERABLE_ERROR)
    {
        strcpy(dev_status, "recoverable error");
    }
    else
    {
        strcpy(dev_status, "uknown");
    }
}

void ChaosController::dev_info_status::append_log(const std::string& log)
{
    CTRLDBG_ << log;
    snprintf(log_status, sizeof(log_status), "%s%s;", log_status, log.c_str());
}

void ChaosController::dev_info_status::reset()
{
    *dev_status = 0;
    *error_status = 0;
    *log_status = 0;
}

void ChaosController::dev_info_status::append_error(const std::string& log)
{
    CTRLERR_ << log;
    snprintf(error_status, sizeof(error_status), "%s%s;", error_status, log.c_str());
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
    data_wrapper.addStringValue("dev_status", std::string(dev_status));
    data_wrapper.addStringValue("log_status", std::string(log_status));
    data_wrapper.addStringValue("error_status", std::string(error_status));

    return &data_wrapper;
}
