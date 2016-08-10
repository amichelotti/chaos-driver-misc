/*
 *	DefaultCommand.h
 *	!CHOAS
 *	Created by Bisegni Claudio.
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
#include "DefaultCommand.h"

#define CMDCU_ LAPP_ << "[DefaultCommand] - "


using namespace chaos;

using namespace chaos::common::data;

using namespace chaos::cu::control_manager::slow_command;
namespace chaos_batch = chaos::common::batch_command;

DefaultCommand::DefaultCommand():o_lct_delay(NULL) {
   // setFeatures(chaos_batch::features::FeaturesFlagTypes::FF_SET_SCHEDULER_DELAY, (uint64_t)000000);
}

DefaultCommand::~DefaultCommand() {
}

// return the implemented handler
uint8_t DefaultCommand::implementedHandler() {
    return  chaos_batch::HandlerType::HT_Set | chaos_batch::HandlerType::HT_Acquisition;
}

// Start the command execution
void DefaultCommand::setHandler(CDataWrapper *data) {
	o_lct_ts = getAttributeCache()->getRWPtr<uint64_t>(DOMAIN_OUTPUT, "trx_ts");
	o_lct_delay = getAttributeCache()->getRWPtr<uint64_t>(DOMAIN_OUTPUT, "trx_delay");
	//o_lct_ts = getVariableValue(chaos_batch::IOCAttributeSharedCache::SVD_OUTPUT, (chaos_batch::VariableIndexType)0)->getCurrentValue<uint64_t>();
	//o_lct_delay = getVariableValue(chaos_batch::IOCAttributeSharedCache::SVD_OUTPUT, (chaos_batch::VariableIndexType)1)->getCurrentValue<uint64_t>();
	*o_lct_delay = 0;
	*o_lct_ts = 0;
}

// Aquire the necessary data for the command
/*!
 The acquire handler has the purpose to get all necessary data need the by CC handler.
 \return the mask for the runnign state
 */
void DefaultCommand::acquireHandler() {
  // boost::posix_time::ptime time = boost::posix_time::microsec_clock::local_time();
  // boost::posix_time::time_duration duration( time.time_of_day() );
  //*o_lct_delay = duration.total_microseconds() - *o_lct_ts;
  //LAPP_<<"TS:"<<*o_lct_ts<<" delay:"<<*o_lct_delay;
  //getAttributeCache()->setOutputDomainAsChanged();

	
}

