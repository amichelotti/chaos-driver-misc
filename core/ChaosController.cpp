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

using namespace ::driver::misc;

#define CALC_EXEC_TIME \
		tot_us +=(reqtime -boost::posix_time::microsec_clock::local_time().time_of_day().total_microseconds());\
		if(naccess%500 ==0){\
			refresh=tot_us/500;\
			tot_us=0;\
			CTRLDBG_ << " Profiling: N accesses:"<<naccess<<" response time:"<<refresh<<" us";}

void ChaosController::setTimeout(uint64_t timeo_us){
	controller->setRequestTimeWaith(timeo_us/1000);
	timeo=timeo_us;
}

int ChaosController::forceState(int dstState){
	int currState=-100,oldstate;
	boost::posix_time::ptime start;
	int retry=10;
	
	do{
		oldstate=currState;
		currState=getState();

		CTRLDBG_ << "Current state ["<<getPath()<<"]:"<<currState<<" destination state:"<<dstState;
		if(currState==dstState){
		  return 0;
		}
		if(currState!=oldstate){
			start=boost::posix_time::microsec_clock::local_time();
		}

		if(currState<0)
			return currState;


		switch(currState){
		case chaos::CUStateKey::DEINIT:
		  CTRLDBG_ << "[deinit] apply \"init\" to :"<<getPath();
		  controller->initDevice();
		  break;

		case chaos::CUStateKey::INIT:
		  switch(dstState){
		  case chaos::CUStateKey::DEINIT:
		    CTRLDBG_ << "[init] apply \"deinit\" to :"<<getPath();
		    controller->deinitDevice();
		    break;
		  case chaos::CUStateKey::START:
		  case chaos::CUStateKey::STOP:
		    CTRLDBG_ << "[init] apply \"start\" to :"<<getPath();
		    controller->startDevice();
		    break;

		  }

		  break;

		case chaos::CUStateKey::START:
		  CTRLDBG_ << "[start] apply \"stop\" to :"<<getPath();
		  controller->stopDevice();
		  break; 


		case chaos::CUStateKey::STOP:
		  switch(dstState){
		  case chaos::CUStateKey::DEINIT:
		  case chaos::CUStateKey::INIT:
		    CTRLDBG_ << "[stop] apply \"deinit\" to :"<<getPath();
		    controller->deinitDevice();
		    break;
		  case chaos::CUStateKey::START:
		    CTRLDBG_ << "[stop] apply \"start\" to :"<<getPath();
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
		if((boost::posix_time::microsec_clock::local_time() - start).total_microseconds()> timeo){
			retry --;
			CTRLERR_ <<"["<< getPath()<<"] Timeout of "<<timeo <<" us elapsed:"<<(boost::posix_time::microsec_clock::local_time() - start).total_microseconds()<< "  Retry:"<<retry;
			if(init(path,timeo)!=0){
				CTRLERR_<<"cannot retrive controller for:"<<path;
				return -1;

			}
			start=boost::posix_time::microsec_clock::local_time();

		}
	} while((currState!=dstState)&& (retry>0));


	if(retry==0){
		CTRLERR_ <<"["<< getPath()<<"]"<< " Not Responding";
		return -100;

	}

	return 0;

}

int  ChaosController::init(int force){

	if(force){

		return forceState(chaos::CUStateKey::INIT);
	}
	return controller->initDevice();
}
int  ChaosController::stop(int force){
	if(force){
		return forceState(chaos::CUStateKey::STOP);
	}
	return controller->stopDevice();
}
int  ChaosController::start(int force){
	if(force){
		return forceState(chaos::CUStateKey::START);
	}
	return controller->startDevice();
}
int  ChaosController::deinit(int force){
	if(force){
		return forceState(chaos::CUStateKey::DEINIT);
	}
	return controller->deinitDevice();
}

int  ChaosController::getState(){

	if(controller->getState(state)>0){
		return state;
	}
	return -1;
}

uint64_t  ChaosController::getTimeStamp(){
	uint64_t ret;
	controller->getTimeStamp(ret);
	return ret;
}
ChaosController::command_t  ChaosController::prepareCommand(std::string alias){
	ChaosController::command_t cmd=boost::shared_ptr<command>(new command());
	cmd->alias = alias;
	return cmd;
}
int ChaosController::setSchedule(uint64_t us){

	schedule=us;
	return controller->setScheduleDelay(us);
}
std::string  ChaosController::getJsonState(){
	std::string ret;
	ret=bundle_state.getData()->getJSONString();
	return ret;
}

int ChaosController::init(std::string p,uint64_t timeo_)  {
	path=p;
	state= chaos::CUStateKey::UNDEFINED;
	schedule=0;
	bundle_state.reset();
	bundle_state.status(state);
	CTRLDBG_ << "init CU NAME:\""<<path<<"\""<<" timeo:"<<timeo_;
	/* CTRLDBG_<<" UI CONF:"<<chaos::ui::ChaosUIToolkit::getInstance()->getGlobalConfigurationInstance()->getConfiguration()->getJSONString();
    CTRLDBG_<<" CU CONF:"<<chaos::cu::ChaosCUToolkit::getInstance()->getGlobalConfigurationInstance()->getConfiguration()->getJSONString();
    CTRLDBG_<<" CU STATE:"<<chaos::cu::ChaosCUToolkit::getInstance()->getServiceState();
    CTRLDBG_<<" UI STATE:"<<chaos::ui::ChaosUIToolkit::getInstance()->getServiceState();
	 */

	//chaos::common::utility::InizializableService::initImplementation(chaos::common::async_central::AsyncCentralManager::getInstance(), 0, "AsyncCentralManager", __PRETTY_FUNCTION__);

	//    chaos::ui::LLRpcApi::getInstance()->init();
	//chaos::ui::ChaosUIToolkit::getInstance()->init(NULL);

	if(controller!=NULL){

		CTRLDBG_<<" removing existing controller";
		chaos::ui::HLDataApi::getInstance()->disposeDeviceControllerPtr(controller);
	}

	try {
		controller= chaos::ui::HLDataApi::getInstance()->getControllerForDeviceID(path, timeo_/1000);
	} catch (chaos::CException &e){
		std::stringstream ss;
		ss<<"Exception during get controller for device:\""<<path<<"\" ex: \""<<e.what()<<"\"";
		bundle_state.append_error(ss.str());
		CTRLERR_<<"Exception during get controller for device:"<<e.what();
		return -3;
	}
	if(controller==NULL){
		CTRLERR_<<"bad handle for controller "<<path;
		return -1;

	}
	controller->setRequestTimeWaith(timeo_/1000);
	controller->setupTracking();
	timeo=timeo_;
	wostate=0;
	heart=0;
	tot_us=0;
	refresh=0;
	naccess=0;
	if(getState()<0){
		CTRLERR_<<"during getting state for device:"<<path;
		return -2;
	}
	std::vector<chaos::common::data::RangeValueInfo> vi=controller->getDeviceValuesInfo();
	for(std::vector<chaos::common::data::RangeValueInfo>::iterator i=vi.begin();i!=vi.end();i++){
		//CTRLDBG_<<"attr_name:"<< i->name<<"type:"<<i->valueType;
		if((i->valueType==chaos::DataType::TYPE_BYTEARRAY) && (i->binType!=chaos::DataType::SUB_TYPE_NONE)){
			binaryToTranslate.insert(std::make_pair(i->name,i->binType));
			CTRLDBG_ << i->name<<" is binary of type:"<<i->binType;
		}
	}
	chaos::common::data::CDataWrapper *  dataWrapper = controller->fetchCurrentDatatasetFromDomain(chaos::ui::DatasetDomainSystem);
	if(dataWrapper){
		json_dataset=dataWrapper->getJSONString();
	} else {
		CTRLERR_<<"cannot retrieve system dataset from "<<path;
		return -3;
	}
	CTRLDBG_<<"initalization ok handle:"<<(void*)controller;
	last_access =boost::posix_time::microsec_clock::local_time().time_of_day().total_microseconds();
	return 0;
}

int ChaosController::waitCmd(){
	return waitCmd(last_cmd);
}
int ChaosController::waitCmd(command_t&cmd){
	int ret;
	boost::posix_time::ptime start= boost::posix_time::microsec_clock::local_time();
	chaos::common::batch_command::CommandState command_state;
	if(cmd==NULL)
		return -200;
	command_state.command_id=cmd->command_id;
	do {
		if((ret=controller->getCommandState(command_state))!=0){
			return ret;
		}

	} while((command_state.last_event!=chaos::common::batch_command::BatchCommandEventType::EVT_COMPLETED) && (command_state.last_event!=chaos::common::batch_command::BatchCommandEventType::EVT_RUNNING)  && ((boost::posix_time::microsec_clock::local_time()-start).total_microseconds()<timeo));

	CTRLDBG_ <<" Command state last event:"<<command_state.last_event;
	if((command_state.last_event==chaos::common::batch_command::BatchCommandEventType::EVT_COMPLETED)||(command_state.last_event==chaos::common::batch_command::BatchCommandEventType::EVT_RUNNING)){
		return 0;
	}

	return -100;
}
int ChaosController::sendCmd(command_t& cmd,bool wait,uint64_t perform_at,uint64_t wait_for){
	int err=0;
	if(cmd==NULL)
		return -2;
	if(perform_at){
		cmd->param.addInt64Value("perform_at",perform_at);
		CTRLDBG_ << "command will be performed at "<< perform_at;

	} else if(wait_for){
		cmd->param.addInt64Value("wait_for",wait_for);
		CTRLDBG_ << "command will be performed in "<< wait_for<<" us";


	}
	CTRLAPP_ << "sending command \""<<cmd->alias <<"\" params:"<<cmd->param.getJSONString();
	if(controller->submitSlowControlCommand(cmd->alias,cmd->sub_rule,cmd->priority,cmd->command_id,0,cmd->scheduler_steps_delay,cmd->submission_checker_steps_delay,&cmd->param)!=0){
		CTRLERR_<<"error submitting";
		return -1;
	}

	chaos::common::batch_command::CommandState command_state;
	command_state.command_id=cmd->command_id;

	err+=controller->getCommandState(command_state);

	//    LAPP_ << "command after:" << ss.str().c_str();
	return err;

}
int ChaosController::executeCmd(command_t& cmd,bool wait,uint64_t perform_at,uint64_t wait_for){
	int ret=sendCmd(cmd,wait,perform_at,wait_for);
	if(ret!=0){
		// retry to update channel
		CTRLERR_<<"error sending command to:"<<path<<" update controller";

		if(init(path,timeo)==0){
			ret=sendCmd(cmd,wait,perform_at,wait_for);
		} else {
			CTRLERR_<<"cannot reinitialize controller:"<<path;
		}
		return ret;
	}
	last_cmd=cmd;
	if(wait){
		CTRLDBG_ << "waiting command id:"<<cmd->command_id;
		if((ret=waitCmd(cmd))!=0){
			CTRLERR_<<"error waiting ret:"<<ret;

			return ret;
		}
		CTRLDBG_ << "command performed";
	}
	return ret;
}

ChaosController::ChaosController(std::string p,uint32_t timeo_):cassandra(common::misc::data::DBCassandra::getInstance("chaos"))  {
	int ret;
	controller = NULL;

	if((ret=init(p,timeo_))!=0){
		throw chaos::CException(ret, "cannot allocate controller for:"+ path + " check if exists",__FUNCTION__);

	}
		cassandra.addDBServer("127.0.0.1");
		if(cassandra.connect()==0){
				DPRINT("connected to cassandra");
		} else {
				ERR("cannot connect to cassandra");
		}
}

ChaosController::ChaosController():cassandra(common::misc::data::DBCassandra::getInstance("chaos")) {
	controller=NULL;
	state= chaos::CUStateKey::UNDEFINED;
	//cassandra = ;
	cassandra.addDBServer("127.0.0.1");
	if(cassandra.connect()==0){
			DPRINT("connected to cassandra");
	} else {
			ERR("cannot connect to cassandra");
	}



}
ChaosController::~ChaosController() {
	if(controller){
		delete controller;
	}
	controller=NULL;


}
chaos::common::data::CDataWrapper* ChaosController::fetch(int channel){
	chaos::common::data::CDataWrapper*data=NULL;
	try {
		if(channel==-1){
			uint64_t time_stamp=boost::posix_time::microsec_clock::local_time().time_of_day().total_milliseconds();
			chaos::common::data::CDataWrapper* idata = NULL,*odata=NULL;
			chaos::common::data::CDataWrapper resdata;
			std::stringstream out;
			uint64_t ts=0;
			idata=controller->fetchCurrentDatatasetFromDomain(chaos::ui::DatasetDomainInput);
			odata=controller->fetchCurrentDatatasetFromDomain(chaos::ui::DatasetDomainOutput);
			if(odata==NULL){
				std::stringstream ss;

				ss<<"error fetching data from output channel ";
				bundle_state.append_error(ss.str());
				return bundle_state.getData();
			}
			//out<<"{\"name\":\""<<getPath()<<"\",\"timestamp\":"<<odata->getInt64Value(chaos::DataPackCommonKey::DPCK_TIMESTAMP);
			resdata.addStringValue("name",getPath());
			resdata.addInt64Value("timestamp",time_stamp);
			if(idata){
				data=normalizeToJson(idata,binaryToTranslate);
				//out<<",\"input\":"<<data->getJSONString();
				resdata.addCSDataValue("input",*data);
			}

			if(odata){
			//	out<<",\"output\":";
				data=normalizeToJson(odata,binaryToTranslate);
				//out<<data->getJSONString();
				resdata.addCSDataValue("output",*data);

			}
//			out<<"}";
			data->reset();
			//CTRLDBG_<<"channel "<<channel<<" :"<<out.str();

	//		data->setSerializedJsonData(out.str().c_str());
			data->appendAllElement(resdata);
			data->appendAllElement(*bundle_state.getData());
			CTRLDBG_<<"channel "<<channel<<" :"<<odata->getJSONString();
			return data;

		} else {
			data=controller->fetchCurrentDatatasetFromDomain((chaos::ui::DatasetDomain)channel);
			if(data==NULL){
				std::stringstream ss;
				ss<<"error fetching data from channel "<<channel;
				bundle_state.append_error(ss.str());
				return bundle_state.getData();
			}
		}

		data=normalizeToJson(data,binaryToTranslate);

		data->appendAllElement(*bundle_state.getData());

		CTRLDBG_<<"channel "<<channel<<" :"<<data->getJSONString();

	} catch (chaos::CException& e) {
		std::stringstream ss;
		ss<<"exception fetching data from channel "<<channel<<" \""<<e.what()<<"\"";
		bundle_state.append_error(ss.str());
		return bundle_state.getData();
	}



	return data;
}

ChaosController::chaos_controller_error_t ChaosController::get(const std::string&  cmd,char* args,int timeout, int prio,int sched,int submission_mode,int channel, std::string &json_buf){
	int err;
	last_access= reqtime;
	reqtime=boost::posix_time::microsec_clock::local_time().time_of_day().total_microseconds();
	naccess++;
	bundle_state.reset();
	bundle_state.status(state);
	CTRLDBG_<<"cmd:"<<cmd<< " last access:" <<reqtime - last_access<<" us ago"<< " timeo:"<<timeo;
	try {
	if (wostate == 0) {
		std::stringstream ss;

		if(((reqtime - last_access) > (timeo))|| ((next_state > 0)&&(state != next_state))){
			if(updateState()==0){
				ss<<" ["<<path<<"] HB expired" <<(reqtime - last_access)<<" us greater than "<<timeo<<" us, removing device";
				init(path,timeo);
				bundle_state.append_error(ss.str());
				json_buf=bundle_state.getData()->getJSONString();
				CALC_EXEC_TIME;
				return CHAOS_DEV_HB_TIMEOUT;
			}
			last_access = reqtime;
			if((state==chaos::CUStateKey::RECOVERABLE_ERROR)||(state==chaos::CUStateKey::FATAL_ERROR)){
				chaos::common::data::CDataWrapper*data=fetch(chaos::ui::DatasetDomainHealth);
				std::string ll;

				if(data->hasKey("nh_led")){
					ll=std::string("domain:")+data->getCStringValue("nh_led");
				}
				if(data->hasKey("nh_lem")){
					ll= ll +std::string(" msg:")+data->getCStringValue("nh_lem");
				}
				bundle_state.append_error(ll);
				json_buf=bundle_state.getData()->getJSONString();
				CALC_EXEC_TIME;


				return (state==chaos::CUStateKey::RECOVERABLE_ERROR)?CHAOS_DEV_RECOVERABLE_ERROR:CHAOS_DEV_FATAL_ERROR;
			}

		}

	} else if (cmd == "status") {
		DPRINT("fetch dataset of %s (stateless)\n", path.c_str());
		bundle_state.status(chaos::CUStateKey::START);
		state = chaos::CUStateKey::START;
		bundle_state.append_log("stateless device");
		chaos::common::data::CDataWrapper* data=fetch(chaos::ui::DatasetDomainOutput);
		json_buf=data->getJSONString();
		CALC_EXEC_TIME;
		return CHAOS_DEV_OK;
	}

	if (cmd == "init") {
		wostate = 0;

		if(updateState()==0){
			init(path,timeo);
			json_buf=bundle_state.getData()->getJSONString();
			CALC_EXEC_TIME;
			return CHAOS_DEV_HB_TIMEOUT;
		}

		if(state!=chaos::CUStateKey::INIT){
			bundle_state.append_log("init device:" + path);
			err = controller->initDevice();
			if(err!=0){
				bundle_state.append_error("initializing device:" + path);
				init(path,timeo);
				json_buf=bundle_state.getData()->getJSONString();
				CALC_EXEC_TIME;
				return CHAOS_DEV_INIT;
			}
			next_state = chaos::CUStateKey::INIT;
		} else {
			bundle_state.append_log("device:" + path+ " already initialized");
		}
		chaos::common::data::CDataWrapper* data=fetch(chaos::ui::DatasetDomainOutput);
		json_buf=data->getJSONString();
		return CHAOS_DEV_OK;
	} else if (cmd == "start") {
		wostate = 0;

		if(updateState()==0){
			init(path,timeo);
			json_buf=bundle_state.getData()->getJSONString();
			CALC_EXEC_TIME;
			return CHAOS_DEV_HB_TIMEOUT;
		}

		if(state!=chaos::CUStateKey::START){
			bundle_state.append_log("start device:" + path);
			err = controller->startDevice();
			if(err!=0){
				bundle_state.append_error("starting device:" + path);
				init(path,timeo);
				json_buf=bundle_state.getData()->getJSONString();
				CALC_EXEC_TIME;
				return CHAOS_DEV_START;
			}
			next_state = chaos::CUStateKey::START;
		} else {
			bundle_state.append_log("device:" + path+ " already started");
		}

		chaos::common::data::CDataWrapper* data=fetch(chaos::ui::DatasetDomainOutput);
		json_buf=data->getJSONString();
		return CHAOS_DEV_OK;
	} else if (cmd == "stop") {
		wostate = 0;

		if(updateState()==0){
			init(path,timeo);
			json_buf=bundle_state.getData()->getJSONString();
			CALC_EXEC_TIME;
			return CHAOS_DEV_HB_TIMEOUT;
		}

		if(state!=chaos::CUStateKey::STOP){
			bundle_state.append_log("stop device:" + path);
			err = controller->stopDevice();
			if(err!=0){
				bundle_state.append_error("stopping device:" + path);
				init(path,timeo);
				json_buf=bundle_state.getData()->getJSONString();
				CALC_EXEC_TIME;
				return CHAOS_DEV_STOP;
			}
			next_state = chaos::CUStateKey::STOP;
		} else {
			bundle_state.append_log("device:" + path+ " already stopped");
		}

		chaos::common::data::CDataWrapper* data=fetch(chaos::ui::DatasetDomainOutput);
		json_buf=data->getJSONString();
		return CHAOS_DEV_OK;
	} else if (cmd == "deinit") {
		wostate = 0;

		if(updateState()==0){
			init(path,timeo);
			json_buf=bundle_state.getData()->getJSONString();
			CALC_EXEC_TIME;
			return CHAOS_DEV_HB_TIMEOUT;
		}

		if(state!=chaos::CUStateKey::DEINIT){
			bundle_state.append_log("deinitializing device:" + path);
			err = controller->deinitDevice();
			if(err!=0){
				bundle_state.append_error("deinitializing device:" + path);
				init(path,timeo);
				json_buf=bundle_state.getData()->getJSONString();
				CALC_EXEC_TIME;
				return CHAOS_DEV_DEINIT;
			}
			next_state = chaos::CUStateKey::DEINIT;
		} else {
			bundle_state.append_log("device:" + path+ " already deinitialized");
		}
		chaos::common::data::CDataWrapper* data=fetch(chaos::ui::DatasetDomainOutput);
		json_buf=data->getJSONString();
		return CHAOS_DEV_OK;
	} else if (cmd == "sched" && (args!=0)) {
		bundle_state.append_log("sched device:" + path);
		err = controller->setScheduleDelay(atol((char*) args));
		if(err!=0){
			bundle_state.append_error("error set scheduling:"+path);
			init(path,timeo);
			json_buf=bundle_state.getData()->getJSONString();
			CALC_EXEC_TIME;
			return CHAOS_DEV_CMD;
		}
		chaos::common::data::CDataWrapper* data=fetch(chaos::ui::DatasetDomainOutput);
				json_buf=data->getJSONString();
		return CHAOS_DEV_OK;
	} else if (cmd == "channel" &&  (args!=0)) {
		// bundle_state.append_log("return channel :" + parm);
		chaos::common::data::CDataWrapper*data=fetch((chaos::ui::DatasetDomain)atoi((char*) args));
		json_buf=data->getJSONString();
		return CHAOS_DEV_OK;

	} else if (cmd == "save" &&  (args!=0)) {
		// bundle_state.append_log("return channel :" + parm);
		chaos::common::data::CDataWrapper*data=fetch((chaos::ui::DatasetDomain)-1);
		json_buf=data->getJSONString();
		std::replace(path.begin(),path.end(),'/','_');
		std::string key = path + "_"+std::string(args);
		DPRINT("saving dataset %s :%s",key.c_str(),json_buf.c_str());
		cassandra.pushData("snapshot",key,json_buf);
		return CHAOS_DEV_OK;

	} else if (cmd == "load" &&  (args!=0)) {
		::common::misc::data::blobRecord_t ret;

		// bundle_state.append_log("return channel :" + parm);
		//chaos::common::data::CDataWrapper*data=fetch((chaos::ui::DatasetDomain)-1));
		//json_buf=data->getJSONString();
		std::replace(path.begin(),path.end(),'/','_');
		std::string key = path + "_"+std::string(args);
		cassandra.queryData("snapshot",key,ret);
		if(ret.size()){
			json_buf=ret[ret.size()-1].data;
			DPRINT("retriving dataset %s : [%lld] %s",key.c_str(),ret[ret.size()-1].timestamp,json_buf.c_str());

		}
		return CHAOS_DEV_OK;

	} else if (cmd == "list" ) {
		::common::misc::data::blobRecord_t ret;

		// bundle_state.append_log("return channel :" + parm);
		//chaos::common::data::CDataWrapper*data=fetch((chaos::ui::DatasetDomain)-1));
		//json_buf=data->getJSONString();
	  //		std::replace(path.begin(),path.end(),'/','_');
	  //	std::string key = path + "_"+std::string(args);
		cassandra.queryData("snapshot","",ret);
		if(ret.size()){
		  int cnt;
		  std::stringstream ss;
		  ss<<"{[";
		  for(cnt=0;cnt<ret.size();cnt++){
		    ss<<ret[cnt].key;
		    if((cnt+1)<ret.size())
		      ss<<",";
		  }
		  ss<<"]}";
		  json_buf=ss.str();
		  DPRINT("retriving key list:%s",ss.str().c_str());

		}
		return CHAOS_DEV_OK;

	} else if (cmd == "attr" && (args!=0)) {

		chaos::common::data::CDataWrapper data;
		data.setSerializedJsonData(args);
		std::vector<std::string> attrs;
		data.getAllKey(attrs);
		for(std::vector<std::string>::iterator i=attrs.begin();i!=attrs.end();i++){
			char param[1024];
			std::string check;
			check.assign(data.getCStringValue(*i));
			if(check.compare(0,2,"0x")==0){
				sprintf(param,"%lld",strtoull(data.getCStringValue(*i),0,0));
				CTRLDBG_<<"converted parameter:"<<param;

			} else {
				strncpy(param,data.getCStringValue(*i),sizeof(param));
			}
			CTRLDBG_<<"applying \""<<i->c_str()<<"\"="<<param;
			bundle_state.append_log("send attr:\"" + cmd + "\" args: \"" + std::string(param) + "\" to device:\"" + path+"\"");
			err = controller->setAttributeToValue(i->c_str(), param, false);
			if(err!=0){
				bundle_state.append_error("error setting attribute:"+path+"/"+*i+"\"="+data.getCStringValue(*i));
				init(path,timeo);
				json_buf=bundle_state.getData()->getJSONString();
				CALC_EXEC_TIME;
				return CHAOS_DEV_CMD;
			}
		}
		json_buf=bundle_state.getData()->getJSONString();
		CTRLDBG_<<"attribute applied:"<<json_buf;
		return CHAOS_DEV_OK;
	} else if (cmd == "recover") {
		bundle_state.append_log("send recover from error:\"" + path);
		err = controller->recoverDeviceFromError();
		if(err!=0){
			bundle_state.append_error("error recovering from error "+path);
			init(path,timeo);
			json_buf=bundle_state.getData()->getJSONString();
			CALC_EXEC_TIME;
			return CHAOS_DEV_CMD;
		}
		json_buf=bundle_state.getData()->getJSONString();
		return CHAOS_DEV_OK;
	} else if (cmd == "restore"  && (args!=0)) {
		bundle_state.append_log("send restore on \"" + path+ "\" tag:\""+std::string(args)+"\"");
		err = controller->restoreDeviceToTag(args);
		if(err!=0){
			bundle_state.append_error("error setting restoring:\""+path+"\" with tag:\""+std::string(args)+"\"");
			init(path,timeo);
			json_buf=bundle_state.getData()->getJSONString();
			CALC_EXEC_TIME;
			return CHAOS_DEV_CMD;
		}
		json_buf=bundle_state.getData()->getJSONString();
		return CHAOS_DEV_OK;
	} else if (cmd != "status") {
		bundle_state.append_log("send cmd:\"" + cmd + "\" args: \"" + std::string(args) + "\" to device:" + path);
		command_t command = prepareCommand(cmd);
		command->param.setSerializedJsonData(args);

		err = sendCmd(command,false);
		if(err!=0){
			init(path,timeo);
			err = sendCmd(command,false);
			if(err!=0){
				bundle_state.append_error("error sending command:"+cmd+" "+std::string(args)+ " to:"+path);
				json_buf=bundle_state.getData()->getJSONString();
				CALC_EXEC_TIME;
				return CHAOS_DEV_CMD;
			}


		}
		bundle_state.status(state);
		json_buf=bundle_state.getData()->getJSONString();
		return CHAOS_DEV_OK;
	}




	chaos::common::data::CDataWrapper*data=fetch((chaos::ui::DatasetDomain)atoi((char*) args));
	json_buf=data->getJSONString();
	return CHAOS_DEV_OK;
	} catch (chaos::CException e){
		bundle_state.append_error("error sending \""+cmd+"\" "+ " to:"+path +" err:"+e.what());
		json_buf=bundle_state.getData()->getJSONString();
		return CHAOS_DEV_UNX;
	} catch (std::exception ee){
		bundle_state.append_error("unexpected error sending \""+cmd+"\" "+ " to:"+path + " err:"+ee.what());
		json_buf=bundle_state.getData()->getJSONString();
		return CHAOS_DEV_UKN;
	}

}




int ChaosController::updateState(){
	uint64_t h;
	last_state=state;
	h= controller->getState(state);
	if (h==0){
		//bundle_state.append_error("cannot access to HB");
		wostate=1;
		state=chaos::CUStateKey::START;
		return -1;
	}
	if((heart>0) &&((h - heart)>HEART_BEAT_MAX)){
		std::stringstream ss;
		ss<<"device is dead "<< (h - heart)<<" ms of inactivity, removing";
		bundle_state.append_error(ss.str());
		bundle_state.status(state);
		init(path,timeo);
		return 0;
	}
	heart= h;
	bundle_state.status(state);

	return (int)state;
}

chaos::common::data::CDataWrapper* ChaosController::normalizeToJson(chaos::common::data::CDataWrapper*src,std::map<std::string,int>& list){
	if(list.empty())
		return src;
	std::vector<std::string> contained_key;
	std::map<std::string,int>::iterator rkey;
	src->getAllKey(contained_key);
	data_out.reset();
	for(std::vector<std::string>::iterator k=contained_key.begin();k!=contained_key.end();k++){
		if((rkey=list.find(*k))!=list.end()){
			if(rkey->second == chaos::DataType::SUB_TYPE_DOUBLE){
				//        CUIServerLDBG_ << " replace data key:"<<rkey->first;
				int cnt;
				double *data=(double*)src->getRawValuePtr(rkey->first);
				int size=src->getValueSize(rkey->first);
				int elems=size/sizeof(double);
				for(cnt=0;cnt<elems;cnt++){
				//  if(data[cnt]<1.0e-308)data[cnt]=1.0e-208;
				//  else
				//    if(data[cnt]>1.0e+308)data[cnt]=1.0e+308;
					data_out.appendDoubleToArray(data[cnt]);
				}
				data_out.finalizeArrayForKey(rkey->first);

			} else if(rkey->second == chaos::DataType::SUB_TYPE_INT32){
				int cnt;
				int32_t *data=(int32_t*)src->getRawValuePtr(rkey->first);
				int size=src->getValueSize(rkey->first);
				int elems=size/sizeof(int32_t);
				for(cnt=0;cnt<elems;cnt++){
					data_out.appendInt32ToArray(data[cnt]);
				}
				data_out.finalizeArrayForKey(rkey->first);
			} else if(rkey->second == chaos::DataType::SUB_TYPE_INT64){
				int cnt;
				int64_t *data=(int64_t*)src->getRawValuePtr(rkey->first);
				int size=src->getValueSize(rkey->first);
				int elems=size/sizeof(int64_t);
				for(cnt=0;cnt<elems;cnt++){
					data_out.appendInt64ToArray(data[cnt]);
				}
				data_out.finalizeArrayForKey(rkey->first);
			} else {
				// LDBG_ << "adding not translated key:"<<*k<<" json:"<<src->getCSDataValue(*k)->getJSONString();
				data_out.appendAllElement(*src->getCSDataValue(*k));
				src->copyKeyTo(*k,data_out);
			}
		} else {
			//LDBG_ << "adding normal key:"<<*k<<" json:"<<src->getCSDataValue(*k)->getJSONString();
			src->copyKeyTo(*k,data_out);

		}
	}
	return &data_out;
}

ChaosController::dev_info_status::dev_info_status (){
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
	}  else if (deviceState == chaos::CUStateKey::RECOVERABLE_ERROR) {
		strcpy(dev_status, "recoverable error");
	} else {
		strcpy(dev_status, "uknown");
	}
}

void ChaosController::dev_info_status::append_log(std::string log) {
	CTRLDBG_ << log;
	snprintf(log_status, sizeof (log_status), "%s%s;", log_status, log.c_str());

}
void ChaosController::dev_info_status::reset(){
	*dev_status = 0;
	*error_status = 0;
	*log_status = 0;
}

void ChaosController::dev_info_status::append_error(std::string log) {
	CTRLERR_ << log;
	snprintf(error_status, sizeof (error_status), "%s%s;", error_status, log.c_str());

}


chaos::common::data::CDataWrapper * ChaosController::dev_info_status::getData(){
	data_wrapper.reset();
	data_wrapper.addStringValue("dev_status",std::string(dev_status));
	data_wrapper.addStringValue("log_status",std::string(log_status));
	data_wrapper.addStringValue("error_status",std::string(error_status));

	return &data_wrapper;
}



