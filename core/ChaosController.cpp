/*
 * File:   ChaosController.cpp
 * Author: michelo
 *
 * Created on September 2, 2015, 5:29 PM
 */

#include "ChaosController.h"
#include <chaos/common/exception/CException.h>
#include <chaos/ui_toolkit/ChaosUIToolkit.h>
#include <chaos/cu_toolkit/ChaosCUToolkit.h>
#include <chaos/ui_toolkit/LowLevelApi/LLRpcApi.h>
#include <common/debug/core/debug.h>
#include <ctype.h>
#include <json/json.h>
#include <json/reader.h>
#include <json/writer.h>
#include <json/value.h>
#include <chaos/ui_toolkit/LowLevelApi/LLRpcApi.h>
using namespace chaos::ui;


using namespace ::driver::misc;
#define DBGET CTRLDBG_<<"["<<getPath()<<"]"

#define CALC_EXEC_TIME \
tot_us +=(reqtime -boost::posix_time::microsec_clock::local_time().time_of_day().total_microseconds());\
if(naccess%500 ==0){\
refresh=tot_us/500;\
tot_us=0;\
CTRLDBG_ << " Profiling: N accesses:"<<naccess<<" response time:"<<refresh<<" us";}

void ChaosController::setTimeout(uint64_t timeo_us) {
    controller->setRequestTimeWaith(timeo_us / 1000);
    timeo = timeo_us;
}

int ChaosController::forceState(int dstState) {
    int currState = -100, oldstate;
    boost::posix_time::ptime start;
    int retry = 10;

    do {
        oldstate = currState;
        currState = getState();

        DBGET << "Current state :" << currState << " destination state:" << dstState;
        if (currState == dstState) {
            return 0;
        }
        if (currState != oldstate) {
            start = boost::posix_time::microsec_clock::local_time();
        }

        if (currState < 0)
            return currState;


        switch (currState) {
            case chaos::CUStateKey::DEINIT:
                DBGET << "[deinit] apply \"init\"";
                controller->initDevice();
                break;

            case chaos::CUStateKey::INIT:
                switch (dstState) {
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
                switch (dstState) {
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
        if ((boost::posix_time::microsec_clock::local_time() - start).total_microseconds() > timeo) {
            retry--;
            CTRLERR_ << "[" << getPath() << "] Timeout of " << timeo << " us elapsed:" << (boost::posix_time::microsec_clock::local_time() - start).total_microseconds() << "  Retry:" << retry;
            if (init(path, timeo) != 0) {
                CTRLERR_ << "cannot retrive controller for:" << path;
                return -1;

            }
            start = boost::posix_time::microsec_clock::local_time();

        }
    } while ((currState != dstState)&& (retry > 0));


    if (retry == 0) {
        CTRLERR_ << "[" << getPath() << "]" << " Not Responding";
        return -100;

    }

    return 0;

}

int ChaosController::init(int force) {

    if (force) {

        return forceState(chaos::CUStateKey::INIT);
    }
    return controller->initDevice();
}

int ChaosController::stop(int force) {
    if (force) {
        return forceState(chaos::CUStateKey::STOP);
    }
    return controller->stopDevice();
}

int ChaosController::start(int force) {
    if (force) {
        return forceState(chaos::CUStateKey::START);
    }
    return controller->startDevice();
}

int ChaosController::deinit(int force) {
    if (force) {
        return forceState(chaos::CUStateKey::DEINIT);
    }
    return controller->deinitDevice();
}

int ChaosController::getState() {

    if (controller->getState(state) > 0) {
        return state;
    }
    state = chaos::CUStateKey::UNDEFINED;
    ;
    return -1;
}

uint64_t ChaosController::getTimeStamp() {
    uint64_t ret;
    controller->getTimeStamp(ret);
    return ret;
}

ChaosController::command_t ChaosController::prepareCommand(std::string alias) {
    ChaosController::command_t cmd = boost::shared_ptr<command>(new command());
    cmd->alias = alias;
    return cmd;
}

int ChaosController::setSchedule(uint64_t us) {

    schedule = us;
    return controller->setScheduleDelay(us);
}

std::string ChaosController::getJsonState() {
    std::string ret;
    ret = bundle_state.getData()->getJSONString();
    return ret;
}

int ChaosController::init(std::string p, uint64_t timeo_) {
    path = p;
    state = chaos::CUStateKey::UNDEFINED;
    schedule = 0;
    bundle_state.reset();
    bundle_state.status(state);
    DBGET << "init CU NAME:\"" << path << "\"" << " timeo:" << timeo_;
    /* DBGET<<" UI CONF:"<<chaos::ui::ChaosUIToolkit::getInstance()->getGlobalConfigurationInstance()->getConfiguration()->getJSONString();
     DBGET<<" CU CONF:"<<chaos::cu::ChaosCUToolkit::getInstance()->getGlobalConfigurationInstance()->getConfiguration()->getJSONString();
     DBGET<<" CU STATE:"<<chaos::cu::ChaosCUToolkit::getInstance()->getServiceState();
     DBGET<<" UI STATE:"<<chaos::ui::ChaosUIToolkit::getInstance()->getServiceState();
     */

    //chaos::common::utility::InizializableService::initImplementation(chaos::common::async_central::AsyncCentralManager::getInstance(), 0, "AsyncCentralManager", __PRETTY_FUNCTION__);

    //    chaos::ui::LLRpcApi::getInstance()->init();
    //chaos::ui::ChaosUIToolkit::getInstance()->init(NULL);

    if (controller != NULL) {

        DBGET << " removing existing controller";
        chaos::ui::HLDataApi::getInstance()->disposeDeviceControllerPtr(controller);
    }

    try {
        controller = chaos::ui::HLDataApi::getInstance()->getControllerForDeviceID(path, timeo_ / 1000);
    } catch (chaos::CException &e) {
        std::stringstream ss;
        ss << "Exception during get controller for device:\"" << path << "\" ex: \"" << e.what() << "\"";
        bundle_state.append_error(ss.str());
        CTRLERR_ << "Exception during get controller for device:" << e.what();
        return -3;
    }
    if (controller == NULL) {
        CTRLERR_ << "bad handle for controller " << path;
        return -1;

    }
    controller->setRequestTimeWaith(timeo_ / 1000);
    controller->setupTracking();
    timeo = timeo_;
    wostate = 0;
    heart = 0;
    tot_us = 0;
    refresh = 0;
    naccess = 0;
    if (getState() < 0) {
        DBGET << "Uknown state for device assuming wostate";
        wostate = 1;
    }
    std::vector<chaos::common::data::RangeValueInfo> vi = controller->getDeviceValuesInfo();
    for (std::vector<chaos::common::data::RangeValueInfo>::iterator i = vi.begin(); i != vi.end(); i++) {
        DBGET << "attr_name:" << i->name << "type:" << i->valueType;
        if ((i->valueType == chaos::DataType::TYPE_BYTEARRAY) && (i->binType != chaos::DataType::SUB_TYPE_NONE)) {
            binaryToTranslate.insert(std::make_pair(i->name, i->binType));
            DBGET << i->name << " is binary of type:" << i->binType;
        } /*else if((i->valueType==chaos::DataType::TYPE_INT64)){
	binaryToTranslate.insert(std::make_pair(i->name,i->valueType));
	DBGET << i->name<<" is of type64 :"<<i->valueType;
	}*/
    }
    chaos::common::data::CDataWrapper * dataWrapper;
    if (wostate) {
        dataWrapper = controller->fetchCurrentDatatasetFromDomain(chaos::ui::DatasetDomainOutput);

    } else {
        dataWrapper = controller->fetchCurrentDatatasetFromDomain(chaos::ui::DatasetDomainSystem);
    }

    if (dataWrapper) {
        json_dataset = dataWrapper->getJSONString();
    } else {
        CTRLERR_ << "cannot retrieve system dataset from " << path;
        return -3;
    }
    DBGET << "initalization ok handle:" << (void*) controller;
    last_access = boost::posix_time::microsec_clock::local_time().time_of_day().total_microseconds();
    return 0;
}

int ChaosController::waitCmd() {
    return waitCmd(last_cmd);
}

int ChaosController::waitCmd(command_t&cmd) {
    int ret;
    boost::posix_time::ptime start = boost::posix_time::microsec_clock::local_time();
    chaos::common::batch_command::CommandState command_state;
    if (cmd == NULL)
        return -200;
    command_state.command_id = cmd->command_id;
    do {
        if ((ret = controller->getCommandState(command_state)) != 0) {
            return ret;
        }

    } while ((command_state.last_event != chaos::common::batch_command::BatchCommandEventType::EVT_COMPLETED) && (command_state.last_event != chaos::common::batch_command::BatchCommandEventType::EVT_RUNNING) && ((boost::posix_time::microsec_clock::local_time() - start).total_microseconds() < timeo));

    DBGET << " Command state last event:" << command_state.last_event;
    if ((command_state.last_event == chaos::common::batch_command::BatchCommandEventType::EVT_COMPLETED) || (command_state.last_event == chaos::common::batch_command::BatchCommandEventType::EVT_RUNNING)) {
        return 0;
    }

    return -100;
}

int ChaosController::sendCmd(command_t& cmd, bool wait, uint64_t perform_at, uint64_t wait_for) {
    int err = 0;
    if (cmd == NULL)
        return -2;
    if (perform_at) {
        cmd->param.addInt64Value("perform_at", perform_at);
        DBGET << "command will be performed at " << perform_at;

    } else if (wait_for) {
        cmd->param.addInt64Value("wait_for", wait_for);
        DBGET << "command will be performed in " << wait_for << " us";


    }
    CTRLAPP_ << "sending command \"" << cmd->alias << "\" params:" << cmd->param.getJSONString();
    if (controller->submitSlowControlCommand(cmd->alias, cmd->sub_rule, cmd->priority, cmd->command_id, 0, cmd->scheduler_steps_delay, cmd->submission_checker_steps_delay, &cmd->param) != 0) {
        CTRLERR_ << "error submitting";
        return -1;
    }

    chaos::common::batch_command::CommandState command_state;
    command_state.command_id = cmd->command_id;

    err += controller->getCommandState(command_state);

    //    LAPP_ << "command after:" << ss.str().c_str();
    return err;

}

int ChaosController::executeCmd(command_t& cmd, bool wait, uint64_t perform_at, uint64_t wait_for) {
    int ret = sendCmd(cmd, wait, perform_at, wait_for);
    if (ret != 0) {
        // retry to update channel
        CTRLERR_ << "error sending command to:" << path << " update controller";

        if (init(path, timeo) == 0) {
            ret = sendCmd(cmd, wait, perform_at, wait_for);
        } else {
            CTRLERR_ << "cannot reinitialize controller:" << path;
        }
        return ret;
    }
    last_cmd = cmd;
    if (wait) {
        DBGET << "waiting command id:" << cmd->command_id;
        if ((ret = waitCmd(cmd)) != 0) {
            CTRLERR_ << "error waiting ret:" << ret;

            return ret;
        }
        DBGET << "command performed";
    }
    return ret;
}

ChaosController::ChaosController(std::string p, uint32_t timeo_) {
    int ret;
    controller = NULL;
    mdsChannel = LLRpcApi::getInstance()->getNewMetadataServerChannel();
    if (!mdsChannel) throw chaos::CException(-1, "No MDS Channel created", "ChaosController()");

    if ((ret = init(p, timeo_)) != 0) {
        throw chaos::CException(ret, "cannot allocate controller for:" + path + " check if exists", __FUNCTION__);

    }
    /*db = ::common::misc::data::DBbaseFactory::getInstance(DEFAULT_DBTYPE,DEFAULT_DBNAME);
     db->setDBParameters("replication",DEFAULT_DBREPLICATION);
     
     db->addDBServer("127.0.0.1");
     if(db->connect()==0){
     DPRINT("connected to cassandra");
     } else {
     ERR("cannot connect to cassandra");
     }*/

}

ChaosController::ChaosController() {
    controller = NULL;
    queryuid = 0;
    state = chaos::CUStateKey::UNDEFINED;
    mdsChannel = LLRpcApi::getInstance()->getNewMetadataServerChannel();
    if (!mdsChannel) throw chaos::CException(-1, "No MDS Channel created", "ChaosController()");

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

}

ChaosController::~ChaosController() {
    /*if(db){
     db->disconnect();
     }
     */
    if (mdsChannel) {
        LLRpcApi::getInstance()->deleteMessageChannel(mdsChannel);
    }
    if (controller) {
        chaos::ui::HLDataApi::getInstance()->disposeDeviceControllerPtr(controller);

    }
    controller = NULL;


}

static const char* chaosDomainToString(int domain) {
    switch (domain) {
        case 0:
            return "output";
        case 1:
            return "input";
        case 2:
            return "custom";
        case 3:
            return "system";
        case 4:
            return "health";
        case 5:
            return "alarms";
        default:
            return "uknown";
    }
}

chaos::common::data::CDataWrapper*ChaosController::combineDataSets(std::map<int, chaos::common::data::CDataWrapper*>& set) {
    std::map<int, chaos::common::data::CDataWrapper*>::iterator i;
    chaos::common::data::CDataWrapper*data;
    chaos::common::data::CDataWrapper resdata;
    uint64_t time_stamp = boost::posix_time::microsec_clock::local_time().time_of_day().total_milliseconds();
    resdata.addStringValue("name", getPath());
    resdata.addInt64Value("timestamp", time_stamp);

    for (i = set.begin(); i != set.end(); i++) {
        if (i->second) {
            data = normalizeToJson(i->second, binaryToTranslate);
            //out<<",\"input\":"<<data->getJSONString();
            resdata.addCSDataValue(chaosDomainToString(i->first), *data);
        } else {

            chaos::common::data::CDataWrapper empty;
            resdata.addCSDataValue(chaosDomainToString(i->first), empty);
            //	  ss<<"error fetching data from \""<<chaosDomainToString(i->first)<<"\" channel ";
            //	  bundle_state.append_error(ss.str());
            //	  return bundle_state.getData();
        }
    }
    data->reset();
    data->appendAllElement(resdata);
    data->appendAllElement(*bundle_state.getData());
    //	DBGET<<"channel "<<channel<<" :"<<odata->getJSONString();
    return data;

}

chaos::common::data::CDataWrapper* ChaosController::fetch(int channel) {
    chaos::common::data::CDataWrapper*data = NULL;
    try {
        if (channel == -1) {
            chaos::common::data::CDataWrapper* idata = NULL, *odata = NULL;
            chaos::common::data::CDataWrapper resdata;
            std::stringstream out;
            uint64_t ts = 0;
            std::map<int, chaos::common::data::CDataWrapper*> set;
            set[chaos::ui::DatasetDomainInput] = controller->fetchCurrentDatatasetFromDomain(chaos::ui::DatasetDomainInput);
            set[chaos::ui::DatasetDomainOutput] = controller->fetchCurrentDatatasetFromDomain(chaos::ui::DatasetDomainOutput);
            set[chaos::ui::DatasetDomainHealth] = controller->fetchCurrentDatatasetFromDomain(chaos::ui::DatasetDomainHealth);
            set[chaos::ui::DatasetDomainSystem] = controller->fetchCurrentDatatasetFromDomain(chaos::ui::DatasetDomainSystem);
            set[chaos::ui::DatasetDomainCustom] = controller->fetchCurrentDatatasetFromDomain(chaos::ui::DatasetDomainCustom);
            set[chaos::ui::DatasetDomainAlarm] = controller->fetchCurrentDatatasetFromDomain(chaos::ui::DatasetDomainAlarm);
            return combineDataSets(set);

        } else {
            data = controller->fetchCurrentDatatasetFromDomain((chaos::ui::DatasetDomain)channel);
            if (data == NULL) {
                std::stringstream ss;
                ss << "error fetching data from channel " << channel;
                bundle_state.append_error(ss.str());
                return bundle_state.getData();
            }
        }

        data = normalizeToJson(data, binaryToTranslate);

        data->appendAllElement(*bundle_state.getData());

        //        DBGET<<"channel "<<channel<<" :"<<data->getJSONString();

    } catch (chaos::CException& e) {
        std::stringstream ss;
        ss << "exception fetching data from channel " << channel << " \"" << e.what() << "\"";
        bundle_state.append_error(ss.str());
        return bundle_state.getData();
    }



    return data;
}

std::string ChaosController::vector2Json(ChaosStringVector& node_found) {
    std::stringstream ss;
    ss << "[";
    for (ChaosStringVector::iterator i = node_found.begin(); i != node_found.end(); i++) {
        if (i + 1 != node_found.end()) {
            ss << *i << ",";
        } else {
            ss << *i;
        }
    }
    ss << "]";
    return ss.str();
}

ChaosController::chaos_controller_error_t ChaosController::get(const std::string& cmd, char* args, int timeout, int prio, int sched, int submission_mode, int channel, std::string &json_buf) {
    int err;
    last_access = reqtime;
    reqtime = boost::posix_time::microsec_clock::local_time().time_of_day().total_microseconds();
    naccess++;
    bundle_state.reset();
    bundle_state.status(state);
    DBGET << "cmd:" << cmd << " args:" << args << " last access:" << reqtime - last_access << " us ago" << " timeo:" << timeo;
    json_buf = "[]";

    try {
        // global commands
        if (cmd == "search") {
            std::stringstream res;
            std::string name = "";
            std::string obj = "cu";
            bool alive = true;

            if (args != NULL) {
                chaos_data::CDataWrapper p;
                p.setSerializedJsonData(args);
                if (p.hasKey("alive")) {
                    alive = p.getBoolValue("alive");
                }
                if (p.hasKey("name")) {
                    name = p.getCStringValue("name");
                }

                if (p.hasKey("what")) {
                    obj = p.getCStringValue("what");
                }
            }


            ChaosStringVector node_found;
            if (obj == "cu") {
                json_buf = "[]";

                if (mdsChannel->searchNode(name,
                        0,
                        alive,
                        0,
                        MAX_QUERY_ELEMENTS,
                        node_found,
                        MDS_TIMEOUT) == 0) {

                    json_buf = vector2Json(node_found);
                    CALC_EXEC_TIME;
                    return CHAOS_DEV_OK;
                }
            } else if (obj == "snapshots") {
                if (mdsChannel->searchSnapshot(name, node_found, MDS_TIMEOUT) == 0) {
                    json_buf = vector2Json(node_found);
                    CALC_EXEC_TIME;
                    return CHAOS_DEV_OK;

                }
            } else if (obj == "insnapshot") {
                json_buf = "[]";

                if (mdsChannel->searchNodeForSnapshot(name, node_found, MDS_TIMEOUT) == 0) {
                    json_buf = vector2Json(node_found);
                    CALC_EXEC_TIME;
                    return CHAOS_DEV_OK;

                }
            } else if (obj == "snapshotsof") {
                json_buf = "[]";

                if (mdsChannel->searchSnapshotForNode(name, node_found, MDS_TIMEOUT) == 0) {
                    json_buf = vector2Json(node_found);
                    CALC_EXEC_TIME;
                    return CHAOS_DEV_OK;

                }
            }  else if (obj == "desc") {
                 json_buf = "[]";
                 if(name.size()>0){
                     CDataWrapper* out;
                     if (mdsChannel->getLastDatasetForDevice(name, &out, MDS_TIMEOUT) == 0) {
                        json_buf =out->getJSONString();
                        CALC_EXEC_TIME;
                        return CHAOS_DEV_OK;
                    }
                 } else {
                   CTRLERR_ << "missing name " << cmd;

                 }
                
            }
            CTRLERR_ << "error performing command: " << cmd;
            return CHAOS_DEV_CMD;
        } else if (cmd == "snapshot") {
            std::stringstream res;
            std::string name = "";
            std::string obj = "";
            chaos_data::CDataWrapper p;
            bool alive = true;

            if (args != NULL) {
             
                p.setSerializedJsonData(args);

                if (p.hasKey("name")) {
                    name = p.getCStringValue("name");
                } else {
                    CTRLERR_ << "missing snapshot name" << cmd;

                    return CHAOS_DEV_CMD;
                }

                if (p.hasKey("what")) {
                    obj = p.getCStringValue("what");
                } else {
                    CTRLERR_ << "missing snapshot operation type" << cmd;

                    return CHAOS_DEV_CMD;
                }
            } else {
                CTRLERR_ << "no parameters given" << cmd;

                return CHAOS_DEV_CMD;
            }


            ChaosStringVector node_found;
            if (obj == "create") {
                ChaosStringVector node_found;

                if (p.hasKey("node_list")) {
                    std::auto_ptr<CMultiTypeDataArrayWrapper> nodes(p.getVectorValue("node_list"));
                    for (int idx = 0; idx < nodes->size(); idx++) {
                        const std::string domain = nodes->getStringElementAtIndex(idx);
                        node_found.push_back(domain);
                        DBGET << "adding \"" << domain << "\" to snapshot name:\"" << name << "\"";
                    }
                    if (mdsChannel->createNewSnapshot(name, node_found, MDS_TIMEOUT) == 0) {
                        DBGET << "Created snapshot name:\"" << name << "\"";

                        json_buf = vector2Json(node_found);
                        CALC_EXEC_TIME;
                        return CHAOS_DEV_OK;
                    }

                } else {
                    CTRLERR_ << "missing node list" << cmd;

                    return CHAOS_DEV_CMD;
                }
                CTRLERR_ << "error creating snapshot:\"" << name << "\"";

                return CHAOS_DEV_CMD;
            } else if (obj == "delete") {
                if (mdsChannel->deleteSnapshot(name, MDS_TIMEOUT) == 0) {
                   DBGET << "Deleted snapshot name:\"" << name << "\"";

                    CALC_EXEC_TIME;
                    return CHAOS_DEV_OK;

                }
            } else if (obj == "restore") {
                if (mdsChannel->restoreSnapshot(name, MDS_TIMEOUT) == 0) {
                    DBGET << "Restore snapshot name:\"" << name << "\"";

                    CALC_EXEC_TIME;
                    return CHAOS_DEV_OK;

                }
            }

            json_buf = "[]";
            CTRLERR_ << "error performing command: " << cmd;
            return CHAOS_DEV_OK;
        }

        if (controller == NULL) {
            CTRLERR_ << "no controller defined " << cmd;

            return CHAOS_DEV_CMD;
        }
        //Json::Reader rreader;
        //Json::Value vvalue;
        if (wostate == 0) {
            std::stringstream ss;

            if (((reqtime - last_access) > (timeo)) || ((next_state > 0)&&(state != next_state))) {
                if (updateState() == 0) {
                    ss << " [" << path << "] HB expired" << (reqtime - last_access) << " us greater than " << timeo << " us, removing device";
                    init(path, timeo);
                    bundle_state.append_error(ss.str());
                    json_buf = bundle_state.getData()->getJSONString();
                    CALC_EXEC_TIME;
                    return CHAOS_DEV_HB_TIMEOUT;
                }
                last_access = reqtime;
                if ((state == chaos::CUStateKey::RECOVERABLE_ERROR) || (state == chaos::CUStateKey::FATAL_ERROR)) {
                    chaos::common::data::CDataWrapper*data = fetch(chaos::ui::DatasetDomainHealth);
                    std::string ll;

                    if (data->hasKey("nh_led")) {
                        ll = std::string("domain:") + data->getCStringValue("nh_led");
                    }
                    if (data->hasKey("nh_lem")) {
                        ll = ll + std::string(" msg:") + data->getCStringValue("nh_lem");
                    }
                    bundle_state.append_error(ll);
                    json_buf = bundle_state.getData()->getJSONString();
                    CALC_EXEC_TIME;


                    return (state == chaos::CUStateKey::RECOVERABLE_ERROR) ? CHAOS_DEV_RECOVERABLE_ERROR : CHAOS_DEV_FATAL_ERROR;
                }

            }

        } else if (cmd == "status") {
            DPRINT("fetch dataset of %s (stateless)\n", path.c_str());
            bundle_state.status(chaos::CUStateKey::START);
            state = chaos::CUStateKey::START;
            bundle_state.append_log("stateless device");
            chaos::common::data::CDataWrapper* data = fetch(chaos::ui::DatasetDomainOutput);
            json_buf = data->getJSONString();
            CALC_EXEC_TIME;
            return CHAOS_DEV_OK;
        }

        if (cmd == "init") {
            wostate = 0;

            if (updateState() == 0) {
                init(path, timeo);
                json_buf = bundle_state.getData()->getJSONString();
                CALC_EXEC_TIME;
                return CHAOS_DEV_HB_TIMEOUT;
            }

            if (state != chaos::CUStateKey::INIT) {
                bundle_state.append_log("init device:" + path);
                err = controller->initDevice();
                if (err != 0) {
                    bundle_state.append_error("initializing device:" + path);
                    init(path, timeo);
                    json_buf = bundle_state.getData()->getJSONString();
                    CALC_EXEC_TIME;
                    return CHAOS_DEV_INIT;
                }
                next_state = chaos::CUStateKey::INIT;
            } else {
                bundle_state.append_log("device:" + path + " already initialized");
            }
            chaos::common::data::CDataWrapper* data = fetch(chaos::ui::DatasetDomainOutput);
            json_buf = data->getJSONString();
            return CHAOS_DEV_OK;
        } else if (cmd == "start") {
            wostate = 0;

            if (updateState() == 0) {
                init(path, timeo);
                json_buf = bundle_state.getData()->getJSONString();
                CALC_EXEC_TIME;
                return CHAOS_DEV_HB_TIMEOUT;
            }

            if (state != chaos::CUStateKey::START) {
                bundle_state.append_log("start device:" + path);
                err = controller->startDevice();
                if (err != 0) {
                    bundle_state.append_error("starting device:" + path);
                    init(path, timeo);
                    json_buf = bundle_state.getData()->getJSONString();
                    CALC_EXEC_TIME;
                    return CHAOS_DEV_START;
                }
                next_state = chaos::CUStateKey::START;
            } else {
                bundle_state.append_log("device:" + path + " already started");
            }

            chaos::common::data::CDataWrapper* data = fetch(chaos::ui::DatasetDomainOutput);
            json_buf = data->getJSONString();
            return CHAOS_DEV_OK;
        } else if (cmd == "stop") {
            wostate = 0;

            if (updateState() == 0) {
                init(path, timeo);
                json_buf = bundle_state.getData()->getJSONString();
                CALC_EXEC_TIME;
                return CHAOS_DEV_HB_TIMEOUT;
            }

            if (state != chaos::CUStateKey::STOP) {
                bundle_state.append_log("stop device:" + path);
                err = controller->stopDevice();
                if (err != 0) {
                    bundle_state.append_error("stopping device:" + path);
                    init(path, timeo);
                    json_buf = bundle_state.getData()->getJSONString();
                    CALC_EXEC_TIME;
                    return CHAOS_DEV_STOP;
                }
                next_state = chaos::CUStateKey::STOP;
            } else {
                bundle_state.append_log("device:" + path + " already stopped");
            }

            chaos::common::data::CDataWrapper* data = fetch(chaos::ui::DatasetDomainOutput);
            json_buf = data->getJSONString();
            return CHAOS_DEV_OK;
        } else if (cmd == "deinit") {
            wostate = 0;

            if (updateState() == 0) {
                init(path, timeo);
                json_buf = bundle_state.getData()->getJSONString();
                CALC_EXEC_TIME;
                return CHAOS_DEV_HB_TIMEOUT;
            }

            if (state != chaos::CUStateKey::DEINIT) {
                bundle_state.append_log("deinitializing device:" + path);
                err = controller->deinitDevice();
                if (err != 0) {
                    bundle_state.append_error("deinitializing device:" + path);
                    init(path, timeo);
                    json_buf = bundle_state.getData()->getJSONString();
                    CALC_EXEC_TIME;
                    return CHAOS_DEV_DEINIT;
                }
                next_state = chaos::CUStateKey::DEINIT;
            } else {
                bundle_state.append_log("device:" + path + " already deinitialized");
            }
            chaos::common::data::CDataWrapper* data = fetch(chaos::ui::DatasetDomainOutput);
            json_buf = data->getJSONString();
            return CHAOS_DEV_OK;
        } else if (cmd == "sched" && (args != 0)) {
            bundle_state.append_log("sched device:" + path);
            err = controller->setScheduleDelay(atol((char*) args));
            if (err != 0) {
                bundle_state.append_error("error set scheduling:" + path);
                init(path, timeo);
                json_buf = bundle_state.getData()->getJSONString();
                CALC_EXEC_TIME;
                return CHAOS_DEV_CMD;
            }
            chaos::common::data::CDataWrapper* data = fetch(chaos::ui::DatasetDomainOutput);
            json_buf = data->getJSONString();
            return CHAOS_DEV_OK;
        } else if (cmd == "channel" && (args != 0)) {
            // bundle_state.append_log("return channel :" + parm);
            chaos::common::data::CDataWrapper*data = fetch((chaos::ui::DatasetDomain)atoi((char*) args));
            json_buf = data->getJSONString();
            return CHAOS_DEV_OK;

        } else if (cmd == "queryhst" && (args != 0)) {
            chaos_data::CDataWrapper p;
            uint64_t start_ts = 0, end_ts = 0xffffffff;
            int32_t page = DEFAULT_PAGE;
            int paging = 0, cnt = 0;
            int limit = MAX_QUERY_ELEMENTS;
            uint32_t current_query = 0;
            std::stringstream res;
            chaos::common::io::QueryCursor *query_cursor = NULL;
            p.setSerializedJsonData(args);
            if (p.hasKey("start")) {
                start_ts = p.getInt64Value("start");
            }

            if (p.hasKey("end")) {
                end_ts = p.getInt64Value("end");
            }
            if (p.hasKey("page")) {
                page = p.getInt32Value("page");
                if (page > MAX_QUERY_ELEMENTS) {
                    page = MAX_QUERY_ELEMENTS;
                }
                paging = 1;
            }

            if (p.hasKey("limit")) {
                limit = p.getInt32Value("limit");
                if ((limit > 0)&&(limit <= page)) {
                    page = limit;
                    paging = 0;
                }
            }

            if (paging) {
                if (query_cursor_map.size() < MAX_CONCURRENT_QUERY) {
                    controller->executeTimeIntervallQuery(chaos::ui::DatasetDomainOutput, start_ts, end_ts, &query_cursor, page);
                    if (query_cursor) {
                        cnt = 0;
                        bool n = query_cursor->hasNext();
                        res << "{\"data\":[";
                        chaos::common::data::CDataWrapper*data;
                        DBGET << "paged query start:" << start_ts << " end:" << end_ts << " page uid " << queryuid << " has next:" << n;
                        current_query = queryuid;


                        while ((query_cursor->hasNext())&&(cnt < page)&&(cnt < limit)) {
                            boost::shared_ptr<CDataWrapper> q_result(query_cursor->next());
                            data = normalizeToJson(q_result.get(), binaryToTranslate);
                            res << data->getJSONString();
                            cnt++;
                            DBGET << "getting query page  " << cnt;
                            if ((query_cursor->hasNext())&&(cnt < page)&&(cnt < limit)) {
                                res << ",";
                            }
                        }
                        res << "]";
                        if ((query_cursor->hasNext()&&(cnt < limit))) {
                            query_cursor_map[++queryuid] = query_cursor;
                            res << ",\"uid\":" << queryuid << "}";
                            DBGET << "continue on UID:" << queryuid;
                        } else {
                            res << ",\"uid\":0}";
                            DBGET << "queryhst no more pages items:" << cnt << " \"" << res.str() << "\"";
                            controller->releaseQuery(query_cursor);
                        }

                    } else {
                        bundle_state.append_error("cannot perform specified query, no data? " + getPath());
                        CALC_EXEC_TIME;
                        return CHAOS_DEV_CMD;
                    }
                } else {
                    bundle_state.append_error("to many concurrent queries, please try later on " + getPath());
                    CALC_EXEC_TIME;
                    return CHAOS_DEV_CMD;
                }

                json_buf = res.str();
                return CHAOS_DEV_OK;
            } else {
                chaos::common::data::CDataWrapper*data;
                controller->executeTimeIntervallQuery(chaos::ui::DatasetDomainOutput, start_ts, end_ts, &query_cursor);
                bool n = query_cursor->hasNext();
                if (query_cursor) {
                    DBGET << "not paged query start:" << start_ts << " end:" << end_ts << " has next: " << (query_cursor->hasNext());
                    cnt = 0;
                    res << "{\"data\":[";

                    while ((query_cursor->hasNext())&& (cnt < limit)) {

                        boost::shared_ptr<CDataWrapper> q_result(query_cursor->next());
                        data = normalizeToJson(q_result.get(), binaryToTranslate);
                        res << data->getJSONString();
                        cnt++;
                        DBGET << "getting query page  " << cnt;
                        if ((query_cursor->hasNext())&&(cnt < limit)) {
                            res << ",";

                        }
                    }
                    res << "],\"uid\":0}";

                } else {
                    bundle_state.append_error("cannot perform specified query, no data? " + getPath());
                    CALC_EXEC_TIME;
                    return CHAOS_DEV_CMD;
                }
            }
            if (query_cursor && ((!query_cursor->hasNext()) || (cnt == limit))) {
                DBGET << "queryhst no more pages items:" << cnt;
                controller->releaseQuery(query_cursor);
                if (current_query) {
                    query_cursor_map.erase(query_cursor_map.find(current_query));
                }
            }
            json_buf = res.str();
            return CHAOS_DEV_OK;

        } else if (cmd == "queryinfo") {
            uint32_t uid = 0;
            std::stringstream res;
            int cnt = 0;
            chaos_data::CDataWrapper p;
            p.setSerializedJsonData(args);
            if (p.hasKey("clearuid")) {
                uid = p.getInt32Value("clearuid");
            }
            if ((uid > 0)&&(query_cursor_map.find(uid) != query_cursor_map.end())) {
                DBGET << "removing query " << uid;
                query_cursor_map.erase(query_cursor_map.find(uid));
            }

            res << "[";
            for (query_cursor_map_t::iterator i = query_cursor_map.begin(); i != query_cursor_map.end(); i++, cnt++) {
                if ((cnt + 1) < query_cursor_map.size()) {
                    res << i->first << ",";
                } else {
                    res << i->first;
                }
            }
            res << "]";

            json_buf = res.str();
            return CHAOS_DEV_OK;

        } else if (cmd == "queryhstnext") {
            chaos::common::io::QueryCursor *query_cursor = NULL;
            chaos_data::CDataWrapper p;
            chaos_data::CDataWrapper* data;
            p.setSerializedJsonData(args);
            std::stringstream res;
            bool clear_req = false;
            int cnt = 0;
            uint32_t uid = 0;
            if (p.hasKey("uid")) {
                uid = p.getInt32Value("uid");
            } else {
                bundle_state.append_error("must specify a valid page uid " + getPath());
                CALC_EXEC_TIME;
                return CHAOS_DEV_CMD;
            }

            if (p.hasKey("clear")) {
                clear_req = p.getBoolValue("clear");
            }
            DBGET << "querynext uid:" << uid << " clear:" << clear_req;
            if (query_cursor_map.find(uid) != query_cursor_map.end()) {
                query_cursor = query_cursor_map[uid];
                if (query_cursor) {
                    cnt = 0;
                    uint32_t page = query_cursor->getPageLen();
                    res << "{\"data\":[";

                    while ((query_cursor->hasNext())&&(cnt < page)) {
                        boost::shared_ptr<CDataWrapper> q_result(query_cursor->next());
                        data = normalizeToJson(q_result.get(), binaryToTranslate);
                        res << data->getJSONString();
                        cnt++;
                        if ((query_cursor->hasNext())&&(cnt < page)) {
                            res << ",";
                        }
                    }
                    res << "]";
                    if ((!query_cursor->hasNext()) || clear_req) {
                        res << ",\"uid\":0}";
                        DBGET << "queryhstnext no more pages items:" << cnt << "with uid:" << uid << " \"" << res.str() << "\"";
                        controller->releaseQuery(query_cursor);
                        query_cursor_map.erase(query_cursor_map.find(uid));

                    } else {
                        res << ",\"uid\":" << uid << "}";
                        DBGET << "queryhstnext some page missing still:" << cnt << "with uid:" << uid;
                    }

                    json_buf = res.str();
                    return CHAOS_DEV_OK;
                }
                bundle_state.append_error("cannot perform specified query, no data? " + getPath());
                CALC_EXEC_TIME;
                return CHAOS_DEV_CMD;

            } else {
                std::stringstream ss;
                ss << "the uid " << uid << " does not exists " + getPath();
                bundle_state.append_error(ss.str());
                CALC_EXEC_TIME;
                return CHAOS_DEV_CMD;
            }
            if (query_cursor) {
                uint32_t exported = 0;
                if (query_cursor->hasNext()) {
                    boost::shared_ptr<CDataWrapper> q_result(query_cursor->next());
                    json_buf = q_result->getJSONString();
                    return CHAOS_DEV_OK;

                } else {
                    controller->releaseQuery(query_cursor);
                    query_cursor = NULL;
                }
            }
            json_buf = "{}";
            return CHAOS_DEV_OK;

        } else if (cmd == "save" && (args != 0)) {
            int ret;
            chaos_data::CDataWrapper p;
            std::string snapname;
            p.setSerializedJsonData(args);
            if (p.hasKey("snapname")) {
                snapname = p.getStringValue("snapname");
                std::vector<std::string> other;
                ret = controller->createNewSnapshot(snapname, other);
                if (ret == 0) {
                    DBGET << "SAVE snapshot " << snapname << " ret:" << ret;
                } else {
                    bundle_state.append_error("error saving snapshot " + getPath() + " snapname:" + snapname);
                    CALC_EXEC_TIME;
                    return CHAOS_DEV_CMD;
                }
                return CHAOS_DEV_OK;
            }
            bundle_state.append_error("error bad arguments for save snapshot " + getPath() + " key:" + std::string(args));
            CALC_EXEC_TIME;
            return CHAOS_DEV_CMD;

        } else if (cmd == "delete" && (args != 0)) {
            int ret;
            chaos_data::CDataWrapper p;
            std::string snapname;
            p.setSerializedJsonData(args);
            if (p.hasKey("snapname")) {
                snapname = p.getStringValue("snapname");
                ret = controller->deleteSnapshot(snapname);
                if (ret == 0) {
                    DBGET << "DELETE snapshot " << snapname << " ret:" << ret;

                    return CHAOS_DEV_OK;
                }
                bundle_state.append_log("error deleting snapshot " + getPath() + " key:" + std::string(args));
                CALC_EXEC_TIME;
                return CHAOS_DEV_CMD;
            }

            bundle_state.append_log("error bad arguments for deleting snapshot " + getPath() + " key:" + std::string(args));
            CALC_EXEC_TIME;
            return CHAOS_DEV_CMD;


        } else if (cmd == "load" && (args != 0)) {
            chaos_data::CDataWrapper * io[2], *ret;
            chaos_data::CDataWrapper p;
            std::string snapname;
            io[0] = 0;
            io[1] = 0;
            p.setSerializedJsonData(args);
            if (p.hasKey("snapname")) {
                snapname = p.getStringValue("snapname");
                int retc = 0;

                retc += controller->loadDatasetTypeFromSnapshotTag(snapname, chaos::ui::DatasetDomainOutput, &io[0]);
                retc += controller->loadDatasetTypeFromSnapshotTag(snapname, chaos::ui::DatasetDomainInput, &io[1]);
                if (retc || io[0] == NULL || io[1] == NULL) {
                    bundle_state.append_error("error load snapshot " + getPath() + " snap name:" + snapname);
                    json_buf = bundle_state.getData()->getJSONString();
                    CALC_EXEC_TIME;
                    return CHAOS_DEV_CMD;
                }

                std::map<int, chaos::common::data::CDataWrapper *> set;
                set[0] = io[0];
                set[1] = io[1];
                ret = combineDataSets(set);
                if (ret) {
                    json_buf = ret->getJSONString();
                } else {
                    bundle_state.append_log("error making load snapshot " + getPath() + " snap name:" + snapname);
                    json_buf = bundle_state.getData()->getJSONString();
                    CALC_EXEC_TIME;
                    return CHAOS_DEV_CMD;
                }


                DBGET << "LOAD snapshot " << snapname << " ret:" << retc;
                return CHAOS_DEV_OK;

            } else {
                bundle_state.append_error("error bad arguments for load snapshot " + getPath() + " key:" + std::string(args));
                CALC_EXEC_TIME;
                return CHAOS_DEV_CMD;
            }



        } else if (cmd == "list") {
            ChaosStringVector snaps;
            int ret;
            ret = controller->getSnapshotList(snaps);
            std::stringstream ss;
            DBGET << "list snapshot err:" << ret;
            ss << "[";
            for (ChaosStringVector::iterator i = snaps.begin(); i != snaps.end(); i++) {
                if ((i + 1) == snaps.end()) {
                    ss << "\"" << *i << "\"";

                } else {
                    ss << "\"" << *i << "\",";

                }
            }
            ss << "]";
            json_buf = ss.str();
            return CHAOS_DEV_OK;

        } else if (cmd == "attr" && (args != 0)) {

            chaos::common::data::CDataWrapper data;
            data.setSerializedJsonData(args);
            std::vector<std::string> attrs;
            data.getAllKey(attrs);
            for (std::vector<std::string>::iterator i = attrs.begin(); i != attrs.end(); i++) {
                char param[1024];
                std::string check;
                check.assign(data.getCStringValue(*i));
                if (check.compare(0, 2, "0x") == 0) {
                    sprintf(param, "%lld", strtoull(data.getCStringValue(*i), 0, 0));
                    DBGET << "converted parameter:" << param;

                } else {
                    strncpy(param, data.getCStringValue(*i), sizeof (param));
                }
                DBGET << "applying \"" << i->c_str() << "\"=" << param;
                bundle_state.append_log("send attr:\"" + cmd + "\" args: \"" + std::string(param) + "\" to device:\"" + path + "\"");
                err = controller->setAttributeToValue(i->c_str(), param, false);
                if (err != 0) {
                    bundle_state.append_error("error setting attribute:" + path + "/" + *i + "\"=" + data.getCStringValue(*i));
                    init(path, timeo);
                    json_buf = bundle_state.getData()->getJSONString();
                    CALC_EXEC_TIME;
                    return CHAOS_DEV_CMD;
                }
            }
            json_buf = bundle_state.getData()->getJSONString();
            DBGET << "attribute applied:" << json_buf;
            return CHAOS_DEV_OK;
        } else if (cmd == "recover") {
            bundle_state.append_log("send recover from error:\"" + path);
            err = controller->recoverDeviceFromError();
            if (err != 0) {
                bundle_state.append_error("error recovering from error " + path);
                init(path, timeo);
                json_buf = bundle_state.getData()->getJSONString();
                CALC_EXEC_TIME;
                return CHAOS_DEV_CMD;
            }
            json_buf = bundle_state.getData()->getJSONString();
            return CHAOS_DEV_OK;
        } else if (cmd == "restore" && (args != 0)) {
            bundle_state.append_log("send restore on \"" + path + "\" tag:\"" + std::string(args) + "\"");
            err = controller->restoreDeviceToTag(args);
            if (err != 0) {
                bundle_state.append_error("error setting restoring:\"" + path + "\" with tag:\"" + std::string(args) + "\"");
                init(path, timeo);
                json_buf = bundle_state.getData()->getJSONString();
                CALC_EXEC_TIME;
                return CHAOS_DEV_CMD;
            }
            json_buf = bundle_state.getData()->getJSONString();
            return CHAOS_DEV_OK;
        } else if (cmd != "status") {
            bundle_state.append_log("send cmd:\"" + cmd + "\" args: \"" + std::string(args) + "\" to device:" + path);
            command_t command = prepareCommand(cmd);
            command->param.setSerializedJsonData(args);

            err = sendCmd(command, false);
            if (err != 0) {
                init(path, timeo);
                err = sendCmd(command, false);
                if (err != 0) {
                    bundle_state.append_error("error sending command:" + cmd + " " + std::string(args) + " to:" + path);
                    json_buf = bundle_state.getData()->getJSONString();
                    CALC_EXEC_TIME;
                    return CHAOS_DEV_CMD;
                }


            }
            bundle_state.status(state);
            json_buf = bundle_state.getData()->getJSONString();
            return CHAOS_DEV_OK;
        }




        chaos::common::data::CDataWrapper*data = fetch((chaos::ui::DatasetDomain)atoi((char*) args));
        json_buf = data->getJSONString();
        return CHAOS_DEV_OK;
    } catch (chaos::CException e) {
        bundle_state.append_error("error sending \"" + cmd + "\" args:\"" + std::string(args) + "\" to:" + path + " err:" + e.what());
        json_buf = bundle_state.getData()->getJSONString();
        return CHAOS_DEV_UNX;
    } catch (std::exception ee) {
        bundle_state.append_error("unexpected error sending \"" + cmd + "\" args:\"" + std::string(args) + "\" to:" + path + " err:" + ee.what());
        json_buf = bundle_state.getData()->getJSONString();
        return CHAOS_DEV_UKN;
    }

}

int ChaosController::updateState() {
    uint64_t h;
    last_state = state;
    h = controller->getState(state);
    if (h == 0) {
        //bundle_state.append_error("cannot access to HB");
        wostate = 1;
        state = chaos::CUStateKey::START;
        return -1;
    }
    if ((heart > 0) &&((h - heart) > HEART_BEAT_MAX)) {
        std::stringstream ss;
        ss << "device is dead " << (h - heart) << " ms of inactivity, removing";
        bundle_state.append_error(ss.str());
        bundle_state.status(state);
        init(path, timeo);
        return 0;
    }
    heart = h;
    bundle_state.status(state);

    return (int) state;
}

chaos::common::data::CDataWrapper* ChaosController::normalizeToJson(chaos::common::data::CDataWrapper*src, std::map<std::string, int>& list) {
    if (list.empty())
        return src;
    std::vector<std::string> contained_key;
    std::map<std::string, int>::iterator rkey;
    src->getAllKey(contained_key);
    data_out.reset();
    for (std::vector<std::string>::iterator k = contained_key.begin(); k != contained_key.end(); k++) {
        if ((rkey = list.find(*k)) != list.end()) {
            if (rkey->second == chaos::DataType::SUB_TYPE_DOUBLE) {
                //        CUIServerLDBG_ << " replace data key:"<<rkey->first;
                int cnt;
                double *data = (double*) src->getRawValuePtr(rkey->first);
                int size = src->getValueSize(rkey->first);
                int elems = size / sizeof (double);
                for (cnt = 0; cnt < elems; cnt++) {
                    //  if(data[cnt]<1.0e-308)data[cnt]=1.0e-208;
                    //  else
                    //    if(data[cnt]>1.0e+308)data[cnt]=1.0e+308;
                    data_out.appendDoubleToArray(data[cnt]);
                }
                data_out.finalizeArrayForKey(rkey->first);

            } else if (rkey->second == chaos::DataType::SUB_TYPE_INT32) {
                int cnt;
                int32_t *data = (int32_t*) src->getRawValuePtr(rkey->first);
                int size = src->getValueSize(rkey->first);
                int elems = size / sizeof (int32_t);
                for (cnt = 0; cnt < elems; cnt++) {
                    data_out.appendInt32ToArray(data[cnt]);
                }
                data_out.finalizeArrayForKey(rkey->first);
            } else if (rkey->second == chaos::DataType::SUB_TYPE_INT64) {
                int cnt;
                int64_t *data = (int64_t*) src->getRawValuePtr(rkey->first);
                int size = src->getValueSize(rkey->first);
                int elems = size / sizeof (int64_t);
                for (cnt = 0; cnt < elems; cnt++) {
                    data_out.appendInt64ToArray(data[cnt]);
                    //  data_out.appendDoubleToArray(data[cnt]);
                }
                data_out.finalizeArrayForKey(rkey->first);
            }/*else if(rkey->second ==chaos::DataType::TYPE_INT64) {
	      data_out.addDoubleValue(rkey->first,(double)src->getInt64Value(rkey->first));
	      }*/ else {
                // LDBG_ << "adding not translated key:"<<*k<<" json:"<<src->getCSDataValue(*k)->getJSONString();
                data_out.appendAllElement(*src->getCSDataValue(*k));
                src->copyKeyTo(*k, data_out);
            }
        } else {
            //LDBG_ << "adding normal key:"<<*k<<" json:"<<src->getCSDataValue(*k)->getJSONString();
            src->copyKeyTo(*k, data_out);

        }
    }
    return &data_out;
}

ChaosController::dev_info_status::dev_info_status() {
    reset();
}

void ChaosController::dev_info_status::status(chaos::CUStateKey::ControlUnitState deviceState) {
    if (deviceState == chaos::CUStateKey::DEINIT) {
        strcpy(dev_status, "deinit");
    } else if (deviceState == chaos::CUStateKey::INIT) {
        strcpy(dev_status, "init");
    } else if (deviceState == chaos::CUStateKey::START) {
        strcpy(dev_status, "start");
    } else if (deviceState == chaos::CUStateKey::STOP) {
        strcpy(dev_status, "stop");
    } else if (deviceState == chaos::CUStateKey::FATAL_ERROR) {
        strcpy(dev_status, "fatal error");
    } else if (deviceState == chaos::CUStateKey::RECOVERABLE_ERROR) {
        strcpy(dev_status, "recoverable error");
    } else {
        strcpy(dev_status, "uknown");
    }
}

void ChaosController::dev_info_status::append_log(std::string log) {
    CTRLDBG_ << log;
    snprintf(log_status, sizeof (log_status), "%s%s;", log_status, log.c_str());

}

void ChaosController::dev_info_status::reset() {
    *dev_status = 0;
    *error_status = 0;
    *log_status = 0;
}

void ChaosController::dev_info_status::append_error(std::string log) {
    CTRLERR_ << log;
    snprintf(error_status, sizeof (error_status), "%s%s;", error_status, log.c_str());

}

chaos::common::data::CDataWrapper * ChaosController::dev_info_status::getData() {
    data_wrapper.reset();
    data_wrapper.addStringValue("dev_status", std::string(dev_status));
    data_wrapper.addStringValue("log_status", std::string(log_status));
    data_wrapper.addStringValue("error_status", std::string(error_status));

    return &data_wrapper;
}



