/*
 *	MessClient.cpp
 *	!CHAOS
 *	Created by Andrea Michelotti
 *
 *    	Copyright 2012 INFN, National Institute of Nuclear Physics
 *
 *    	Licensed under the Apache License, Version 2.0 (the "License");
 *    	you may not use this file except in compliance with the License.
 *    	You may obtain a copy of the License at
 *
 *    	http://www.apache.org/licenses/LICENSE-2.0
 *
 *    	Unless required by applicable law or agreed to in writing, software
 *    	distributed under the License is distributed on an "AS IS" BASIS,
 *    	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    	See the License for the specific language governing permissions and
 *    	limitations under the License.
 */
#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <chaos/common/global.h>
#include <chaos/common/chaos_constants.h>
#include <chaos/common/network/CNodeNetworkAddress.h>
#include <chaos/ui_toolkit/ChaosUIToolkit.h>
#include <chaos/ui_toolkit/LowLevelApi/LLRpcApi.h>
#include <chaos/ui_toolkit/HighLevelApi/HLDataApi.h>
#include <stdio.h>
#include <chaos/common/bson/bson.h>
#include <boost/format.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/c_local_time_adjustor.hpp>
#include <fstream>
#include <functional>
#include <boost/shared_ptr.hpp>
#include "MessProfilePacketInfo.h"
#include <boost/crc.hpp>
#define RETRY_LOOP 1000000
#define BAND_REPETITION 100
#define MAX_BUFFER 1024*1024

using namespace std;
using namespace chaos;
using namespace chaos::ui;
using namespace chaos::common::data;
using namespace bson;
using namespace boost;
using namespace boost::posix_time;
using namespace boost::date_time;
namespace chaos_batch = chaos::common::batch_command;

#define OPT_MESS_DID                        "mess_device_id"
#define OPT_MESS_PHASES_START               "start"
#define OPT_MESS_PHASES_STOP                "stop"
#define OPT_MESS_PHASES_INIT                "init"
#define OPT_MESS_PHASES_DEINIT              "deinit"

#define OPT_PERFORM_TEST                    "report"

#define OPT_MAKE_TRX_TEST                   "trx_delay_test"
#define OPT_MAKE_ROUND_TRIP_TEST            "round_trip_test"
#define OPT_MAKE_ROUND_TRIP_TEST_ITERATION  "round_trip_test_iteration"
#define OPT_GET_TRX_TEST                    "bandwidth_test"
#define OPT_TIMEOUT                         "timeout"
#define OPT_SCHED_DELAY                     "scheduler_delay"

#define OPT_MAX                 "max"
#define OPT_INCREMENT                "increment"
#define OPT_START                    "start"

#define OPT_TEST_REPETITION               "test_repetition"

class perform_test {

protected:
	boost::shared_ptr<ofstream> fs;
	string test_name;
	string fradix_name;
	DeviceController* controller;
	string filename;


public:

	perform_test(string _test_name, string _fradix_name, DeviceController*ctrl) : test_name(_test_name), fradix_name(_fradix_name), controller(ctrl) {
		filename = _fradix_name + "_" + _test_name + ".csv";
		fs.reset(new ofstream(filename.c_str(), ios::out));
		LAPP_ << "opening "<<filename<<" for write";
		int err;
		if ( fs == NULL || !fs->good()) {
			throw CException(1, string(__FUNCTION__), "## cannot open " + filename);
		}

		err = controller->initDevice();
		if (err == ErrorCode::EC_TIMEOUT) throw CException(6,string(__FUNCTION__), "Set device to init state");
		sleep(1);
		err = controller->startDevice();
		if (err == ErrorCode::EC_TIMEOUT) throw CException(2, string(__FUNCTION__), "Set device to start state");
		sleep(1); //wait for init to complete.
	}

	~perform_test() {
		int err;
		fs->close();
		fs.reset();
		LAPP_ << "Wrote  " << filename<<endl;
		usleep(100000); // TODO: MAY NOT WORK IF THE DEVICE IS BUSY

		err = controller->stopDevice();
		if (err == ErrorCode::EC_TIMEOUT) {
			throw CException(2,  string(__FUNCTION__), "## Error Setting device to stop state");
		}
		sleep(1);
		err = controller->deinitDevice();


		if (err == ErrorCode::EC_TIMEOUT) {
			throw CException(2,  string(__FUNCTION__), "## Error Setting to deinit state");


		}
		sleep(1);

	}

	virtual int test(int i,uint64_t delay,int repetition) = 0;
};

class perform_bandwidth_test : public perform_test {
public:
	perform_bandwidth_test(string _fradix_name, DeviceController*ctrl) : perform_test(string("bandwidth_test"), _fradix_name, ctrl) {
		*fs << "Bytes,UI Acquire (us),UI Acquire min (us), UI Acquire max (us), CU cycle (us),CU cycle sampled (us), CU sigma (us), CU bandwidth (KB/s), ui packets lost,time shift(us), Cmd Latency (us), Cycles/s"<<endl;
	}

	int test(int bytes,uint64_t delay,int repetition) {
		CDataWrapper *wrapped_data = NULL;
		LAPP_ << "Performing  " << test_name<< "bytes :"<<bytes<< " scheduling :"<<delay<<" us"<<" repetitions:"<<repetition;
		//set scehdule delay to 500 micro seconds
		uint64_t command_id;
		controller->setScheduleDelay(delay);
		CDataWrapper test_data;

		boost::posix_time::ptime start_test = boost::posix_time::microsec_clock::local_time();
		int64_t start_test_us=start_test.time_of_day().total_microseconds();
		uint32_t counter =0;
		test_data.addInt32Value("bytes", bytes);
		test_data.addInt32Value("repeat", repetition);
		test_data.addInt64Value("ts_tag", start_test_us);
		int ui_cu_time_shift[repetition];

		uint64_t total_micro=0;
		uint64_t micro_min=-1;
		uint64_t micro_max=0;
		uint64_t number_of_packets=0;
		uint64_t ui_packets_lost=0;

		uint64_t cu_ui_timeshift=0;
		uint32_t ok_counter=0;
		uint32_t ui_cycles=0;
		uint32_t errors=0;
		int64_t command_latency=0;
		int64_t fetch_data_us=0;
		uint64_t live_fetch=0;
		int err;
		int retry=RETRY_LOOP;
		MessProfilePacketInfo *prof=0;
		err = controller->submitSlowControlCommand("calc_bandwidth",
				chaos_batch::SubmissionRuleType::SUBMIT_AND_Stack,
				100,
				command_id,
				0,
				delay,
				0,
				&test_data);

		//read answer
		if (err != ErrorCode::EC_NO_ERROR) throw CException(2, "Error", "executing command");
		do {
			boost::posix_time::ptime start_loop = boost::posix_time::microsec_clock::local_time();
			controller->fetchCurrentDeviceValue();
			wrapped_data = controller->getCurrentData();
			fetch_data_us =  boost::posix_time::microsec_clock::local_time().time_of_day().total_microseconds();
			if (wrapped_data) {
				int size;

				prof = (MessProfilePacketInfo *)wrapped_data->getBinaryValue("profile",size);
				//   cout<<"read:"<<duration0.total_microseconds()<<endl;
				if(prof && (size==sizeof(MessProfilePacketInfo))&&(prof->ts_tag == start_test_us)){
					LDBG_<<"ts_tag:"<<prof->ts_tag<<" cu count:"<<prof->uidx<<" crc:"<<prof->crc32<<" local counter:"<<counter<<" size "<<size;

					if(command_latency==0){
						command_latency = fetch_data_us - start_test_us;
				    }


					if(prof->uidx==counter){
						ui_cu_time_shift[ok_counter]=abs((long long)(fetch_data_us - prof->tprof.end_cycle_us));
						cu_ui_timeshift+=ui_cu_time_shift[ok_counter];
						ok_counter++;
						retry=RETRY_LOOP;
						counter++;
					} else if(prof->uidx>counter){
						int lost=(prof->uidx-counter);
						ui_packets_lost+=lost;
						LDBG_<<"%% UI packet lost CU:"<<prof->uidx<<" ui:"<<counter<<" total:"<<ui_packets_lost;
						counter= prof->uidx+1;
						retry=RETRY_LOOP;
					}
#ifdef CHECK_BUFFER		  
					// cout<<"received:"<<prof->uidx<<" exp:"<<counter<<" crc:"<<prof->crc32<<endl;
					if((prof->uidx==counter) && (prof->crc32!=0)){
						const char *buffer=0;
						buffer = wrapped_data->getBinaryValue("buffer",size);

						if(buffer && (size == bytes)){
							uint32_t crc;
							boost::crc_32_type crc_calc;
							crc_calc.process_bytes(buffer, size);
							crc =crc_calc.checksum();
							if(crc!=prof->crc32){
								LERR_<<"## size " << size<< "iteration "<< counter<<" bad CRC";
								errors++;
							}
						}
					}
#endif

					//controller->getTimeStamp(cu_push_time);

					//					if(prof->uidx==counter){
					//						cu_ui_timeshift+=abs((long long)(start_loop.time_of_day().total_microseconds() - prof->tprof.end_cycle_us));
					//						ok_counter++;
					//					}
					number_of_packets++;
					retry=RETRY_LOOP;
				}

			}
			boost::posix_time::ptime end_loop = boost::posix_time::microsec_clock::local_time();
			boost::posix_time::time_duration duration =(end_loop-start_loop);
			uint64_t packet_time=duration.total_microseconds();
			micro_max = std::max(micro_max,packet_time);
			micro_min = std::min(micro_min,packet_time);
			total_micro +=packet_time;

			ui_cycles++;

		} while ((counter<repetition) && (retry-->0));

		if(retry<0){
			if(prof==NULL){
				LERR_<<" no live data";
				throw CException(2, "No live data", "No live data");
			}
			LERR_ << "## CU TOO SLOW packet lost:"<<ui_packets_lost<<" received:"<<number_of_packets<<" ui cycles:"<<ui_cycles<<" ui counter:"<<counter<<" cu counter:"<<prof->uidx;
			throw CException(2, "CU too slow", "CU too slow");
		}
		LAPP_<<"* CU #:"<<prof->uidx<<" ui#:"<<counter -1<<" total lost:"<<ui_packets_lost;



		if(ok_counter<1){
			LERR_ << "## MANY out of sync PACKETS:"<<ui_packets_lost<<" received in sync:"<<ok_counter<<" ui cycles:"<<ui_cycles<<" ui counter:"<<counter<<" cu counter:"<<prof->uidx;

		}

		double  hz= 1000000.0/prof->tprof.net_cycle_us;
		double kbs=(bytes*1.0/1024.0)*hz;
		double cycle_us=1000000.0/prof->tprof.cycle_sec;
		double ui_shift_average=(double)cu_ui_timeshift*1.0/((ok_counter==0)?1:ok_counter);
		double ui_sigma=0;
		for(int cnt=0;cnt<ok_counter;cnt++){
			ui_sigma+=pow((ui_cu_time_shift[cnt]-ui_shift_average),2);
		}
		ui_sigma/=((ok_counter==0)?1:ok_counter);
		ui_sigma=sqrt(ui_sigma);
		*fs << bytes << "," << total_micro/ui_cycles << "," << micro_min << "," << micro_max<< ","<<cycle_us<<","<<prof->tprof.cycle_us<<","<<prof->tprof.cycle_sigma<<","<<kbs<<","<<ui_packets_lost<<","<<ui_shift_average<<","<<ui_sigma<<","<< (int64_t)prof->tprof.cmd_arrival_time_us - (int64_t)start_test.time_of_day().total_microseconds()<<","<<prof->tprof.cycle_sec<<endl;

		//sleep(1);
		return 0;


	}
};

class perform_rt_test : public perform_test {
public:
	perform_rt_test(string _fradix_name, DeviceController*ctrl) : perform_test(string("rt_test"), _fradix_name, ctrl) {
		*fs << "Cycle delay,mean rt(us), sigma rt (us), mean set(us),sigma set (us),#errors"<<endl;
	}

	int test(int ui_delay,uint64_t delay,int repetition) {

		int err;
		uint64_t got_ts = 0, got_delay = 0, cur_ts = 0, command_id = 0, calc_rt_delay = 0, got_max_rt_delay, got_min_rt_delay, got_mean_rt_delay, got_max_set_delay, got_min_set_delay, got_mean_set_delay;
		CDataWrapper *wrapped_data = NULL;
		LAPP_ << "Performing  " << test_name<< " schedule:"<<delay<<" ui delay :"<<ui_delay;

		int retry ;
		uint32_t samples=0,errors=0;
		double time_shift_avg=0,time_rt_avg=0;
		uint64_t time_shift_samples[repetition];
		uint64_t time_rt_samples[repetition];

		controller->setScheduleDelay(delay);


		for (int idx = 0; idx < repetition; idx++) {
			CDataWrapper test_delay_param_data;
			retry = RETRY_LOOP;
			boost::posix_time::ptime time = boost::posix_time::microsec_clock::local_time();
			boost::posix_time::time_duration duration(time.time_of_day());

			test_delay_param_data.addInt64Value("sts", cur_ts = duration.total_microseconds());
			err = controller->submitSlowControlCommand("trx_delay",
					//         					       chaos_batch::SubmissionRuleType::SUBMIT_AND_Stack,
					chaos_batch::SubmissionRuleType::SUBMIT_AND_Kill,
					100,
					command_id,
					0,
					delay, // delay
					0,
					&test_delay_param_data);

			//read answer
			if (err != ErrorCode::EC_NO_ERROR) throw CException(2, "Error", "executing commant");
			got_ts= 0;
			got_delay=0;
			//LAPP_<<"iteration :"<<idx<<" sending:"<<cur_ts;
			do {
				controller->fetchCurrentDeviceValue();
				wrapped_data = controller->getCurrentData();
				//    got_ts=0;
				if (wrapped_data) {
					got_ts = wrapped_data->getUInt64Value("trx_ts");
					got_delay = wrapped_data->getUInt64Value("trx_delay");

				}

			} while ((got_ts != cur_ts)&& (retry-->0));


			if(retry<0){
				LAPP_<<"%% timeout got_ts:"<<got_ts<<" expected:"<<cur_ts<<" diff:"<<cur_ts-got_ts;
				chaos_batch::CommandState command_state;
				command_state.command_id = command_id;
				err = controller->getCommandState(command_state);
				LAPP_ << "Device state start -------------------------------------------------------" << std::endl;
				LAPP_ << "Command";
				switch (command_state.last_event) {
				case chaos_batch::BatchCommandEventType::EVT_COMPLETED:
					LAPP_ << " has completed"<< std::endl;;
					break;
				case chaos_batch::BatchCommandEventType::EVT_FAULT:
					LAPP_ << " has fault";
					LAPP_ << "Error code		:"<<command_state.fault_description.code<< std::endl;
					LAPP_ << "Error domain		:"<<command_state.fault_description.domain<< std::endl;
					LAPP_ << "Error description	:"<<command_state.fault_description.description<< std::endl;
					break;
				case chaos_batch::BatchCommandEventType::EVT_KILLED:
					LAPP_ << " has been killed"<< std::endl;
					break;
				case chaos_batch::BatchCommandEventType::EVT_PAUSED:
					LAPP_ << " has been paused"<< std::endl;
					break;
				case chaos_batch::BatchCommandEventType::EVT_QUEUED:
					LAPP_ << " has been queued"<< std::endl;
					break;
				case chaos_batch::BatchCommandEventType::EVT_RUNNING:
					LAPP_ << " is running"<< std::endl;
					break;
				case chaos_batch::BatchCommandEventType::EVT_WAITING:
					LAPP_ << " is waiting"<< std::endl;
					break;
				}
				LAPP_ << "Device state end ---------------------------------------------------------" << std::endl;
				LERR_<<"iteraction: "<<idx<<" cur_ts:"<< cur_ts<<" got_ts:"<<got_ts<<" delay:"<<got_delay;
				errors++;
			}

			if (got_ts == cur_ts) {
				boost::posix_time::ptime time = boost::posix_time::microsec_clock::local_time();
				boost::posix_time::time_duration duration(time.time_of_day());
				time_rt_samples[samples] = duration.total_microseconds() - cur_ts;
				time_shift_samples[samples]=got_delay;
				time_rt_avg+=time_rt_samples[samples];
				time_shift_avg+=got_delay;
				samples++;
			} else {
				errors++;
			}

			usleep(ui_delay);
		}
		if(samples<1){
			LERR_<<"Too many errors: "<<errors;
			throw CException(errors,"Too many errors","too many errors");
		}
		time_rt_avg/=samples;
		time_shift_avg/=samples;
		double sigma_rt=0,sigma_shift=0;
		for(int idx=0;idx<samples;idx++){
			sigma_rt+=pow((time_rt_samples[idx] -time_rt_avg ),2);
			sigma_shift+=pow((time_shift_samples[idx] -time_shift_avg ),2);
		}
		sigma_rt/=samples;
		sigma_shift/=samples;
		sigma_rt=sqrt(sigma_rt);
		sigma_shift=sqrt(sigma_shift);
		*fs << ui_delay << "," << time_rt_avg << "," << sigma_rt << "," << time_shift_avg << "," << sigma_shift <<","<<errors <<endl;

		return 0;
	}

};


int main(int argc, char* argv[]) {
	try {
		int err = 0;
		uint32_t timeout = 0;
		uint32_t iteration = 0;
		uint64_t command_id = 0;
		string mess_device_id;
		uint64_t schedule_delay = 0;
		int32_t start=1;
		string report_name;
		std::string increment;
		uint32_t max;
		int increment_v=-1;
		CDataWrapper *wrapped_data = NULL;
		typedef std::vector<int>::iterator OpcodeSequenceIterator;
		std::vector<int> opcodes_sequence;
		CDeviceNetworkAddress deviceNetworkAddress;
		int repetition;
		ChaosUIToolkit::getInstance()->getGlobalConfigurationInstance()->addOption(OPT_MESS_DID, po::value<string>(&mess_device_id), "The host of the mess monitor");
		//ChaosUIToolkit::getInstance()->getGlobalConfigurationInstance()->addOption(OPT_MESS_PHASES_INIT, "Initialize the monitor");
		//ChaosUIToolkit::getInstance()->getGlobalConfigurationInstance()->addOption(OPT_MESS_PHASES_START, "Start the monitor");
		//ChaosUIToolkit::getInstance()->getGlobalConfigurationInstance()->addOption(OPT_MESS_PHASES_STOP, "Stop the monitor");
		ChaosUIToolkit::getInstance()->getGlobalConfigurationInstance()->addOption(OPT_PERFORM_TEST, po::value<string>(&report_name)->default_value("./test_"), "Test prefix for CSV <report> files");
		ChaosUIToolkit::getInstance()->getGlobalConfigurationInstance()->addOption(OPT_MESS_PHASES_DEINIT, "Deinitilize the monitor");
		ChaosUIToolkit::getInstance()->getGlobalConfigurationInstance()->addOption(OPT_MAKE_ROUND_TRIP_TEST, "Execute the round trip delay test");
		ChaosUIToolkit::getInstance()->getGlobalConfigurationInstance()->addOption(OPT_MAKE_ROUND_TRIP_TEST_ITERATION, po::value<uint32_t>(&iteration)->default_value(1), "Number of iteration of the roundtrip test");
		ChaosUIToolkit::getInstance()->getGlobalConfigurationInstance()->addOption(OPT_GET_TRX_TEST, "Execute the bandwidth test");
		ChaosUIToolkit::getInstance()->getGlobalConfigurationInstance()->addOption(OPT_TIMEOUT, po::value<uint32_t>(&timeout)->default_value(10000), "Timeout rpc in milliseconds");
		ChaosUIToolkit::getInstance()->getGlobalConfigurationInstance()->addOption(OPT_SCHED_DELAY, po::value<uint64_t>(&schedule_delay)->default_value(0), "Scheduler delay (us)");

		ChaosUIToolkit::getInstance()->getGlobalConfigurationInstance()->addOption(OPT_MAX, po::value<uint32_t>(&max)->default_value(MAX_BUFFER), "Max buffer (bandwidth test)/Max ui cyle (rt test)");

		ChaosUIToolkit::getInstance()->getGlobalConfigurationInstance()->addOption(OPT_INCREMENT, po::value<std::string>(&increment)->default_value("power2"), "Increment to be used in test: power2|<constant value>");
		ChaosUIToolkit::getInstance()->getGlobalConfigurationInstance()->addOption(OPT_START, po::value<int32_t>(&start)->default_value(1), "start quantity in test ");

		ChaosUIToolkit::getInstance()->getGlobalConfigurationInstance()->addOption(OPT_TEST_REPETITION, po::value<int>(&repetition)->default_value(BAND_REPETITION), "number of test repetions for better average results");


		ChaosUIToolkit::getInstance()->init(argc, argv);

		if(ChaosUIToolkit::getInstance()->getGlobalConfigurationInstance()->hasOption(OPT_INCREMENT)){
			if(increment != "power2"){
				increment_v = strtoul(increment.c_str(),0,0);
			}
		}
		if (!ChaosUIToolkit::getInstance()->getGlobalConfigurationInstance()->hasOption(OPT_MESS_DID)) throw CException(1, "invalid device identification string", "check param");
		if (mess_device_id.size() == 0) throw CException(1, "invalid device identification string", "check param");

		if (ChaosUIToolkit::getInstance()->getGlobalConfigurationInstance()->hasOption(OPT_MESS_PHASES_INIT)) {
			opcodes_sequence.push_back(1);
		}
		if (ChaosUIToolkit::getInstance()->getGlobalConfigurationInstance()->hasOption(OPT_MESS_PHASES_START)) {
			opcodes_sequence.push_back(2);
		}
		if (ChaosUIToolkit::getInstance()->getGlobalConfigurationInstance()->hasOption(OPT_SCHED_DELAY)) {
			opcodes_sequence.push_back(5);
		}



		DeviceController *controller = HLDataApi::getInstance()->getControllerForDeviceID(mess_device_id, timeout);

		if (!controller) throw CException(4, "Error allocating device controller", "device controller creation");
		controller->setRequestTimeWaith(timeout);
		if (report_name.length()) {
			int cnt;
			if(ChaosUIToolkit::getInstance()->getGlobalConfigurationInstance()->hasOption(OPT_GET_TRX_TEST)){
				perform_bandwidth_test bd_test(report_name,controller);
				if(max == increment_v){
					bd_test.test(max,schedule_delay,repetition);

				} else {
					for(cnt=start;cnt<=max;(increment_v<0)?(cnt<<=1):cnt+=increment_v){
						bd_test.test(cnt,schedule_delay,repetition);
						sleep(1);
					}
				}
			}
			if(ChaosUIToolkit::getInstance()->getGlobalConfigurationInstance()->hasOption(OPT_MAKE_ROUND_TRIP_TEST)){
				perform_rt_test rt_test(report_name,controller);
				if(max == increment_v){
					rt_test.test(max,schedule_delay,repetition);
				} else {

					for(cnt=start;cnt<=max;(increment_v<0)?(cnt<<=1):cnt+=increment_v){
						rt_test.test(cnt,schedule_delay,repetition);
					}
				}
			}
			return 0;
		}

		 HLDataApi::getInstance()->disposeDeviceControllerPtr(controller);


	} catch (CException& e) {
		std::cerr << e.errorCode << " - " << e.errorDomain << " - " << e.errorMessage << std::endl;
		return -1;
	}
	try {
		ChaosUIToolkit::getInstance()->deinit();
	} catch (CException& e) {
		std::cerr << e.errorCode << " - " << e.errorDomain << " - " << e.errorMessage << std::endl;
		return -2;
	}
	return 0;
}



