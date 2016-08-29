//
//  CmdCalcTrxDelay.cpp
//  chaos-mess
//
//  Created by Claudio Bisegni on 27/02/14.
//  Copyright (c) 2014 INFN. All rights reserved.
//

#include "CmdCalcTrxDelay.h"

#include <boost/date_time/posix_time/posix_time_types.hpp>

using namespace chaos;

using namespace chaos::common::data;

using namespace chaos::cu::control_manager::slow_command;
namespace chaos_batch = chaos::common::batch_command;

BATCH_COMMAND_OPEN_DESCRIPTION_ALIAS(,CmdCalcTrxDelay,CmdCalcTrxDelay_CMD_ALIAS,
                                    "Start the trx delay (ping)",
                                    "adf2973c-35dd-11e5-155a-774defc0b6db")
BATCH_COMMAND_ADD_INT64_PARAM(CmdCalcTrxDelay_TS_PARAM_KEY, "timing tag", chaos::common::batch_command::BatchCommandAndParameterDescriptionkey::BC_PARAMETER_FLAG_MANDATORY)

BATCH_COMMAND_CLOSE_DESCRIPTION()


CmdCalcTrxDelay::CmdCalcTrxDelay():o_lct_ts(NULL),o_lct_delay(NULL)  {
    //setFeatures(chaos_batch::features::FeaturesFlagTypes::FF_SET_SCHEDULER_DELAY, (uint64_t)1);

}

CmdCalcTrxDelay::~CmdCalcTrxDelay() {
	
}

// return the implemented handler
uint8_t CmdCalcTrxDelay::implementedHandler() {
	return  chaos_batch::HandlerType::HT_Set| chaos_batch::HandlerType::HT_Acquisition;;
}

// Start the command execution
void CmdCalcTrxDelay::setHandler(CDataWrapper *data) {
	if(!data) {
            std::cout<<"## empty data"<<std::endl;
            return;
        }
        getAttributeCache()->setOutputAttributeNewSize("buffer",0);
	
	o_lct_ts = getAttributeCache()->getRWPtr<uint64_t>(DOMAIN_OUTPUT, "trx_ts");
	o_lct_delay = getAttributeCache()->getRWPtr<uint64_t>(DOMAIN_OUTPUT, "trx_delay");
	
	*o_lct_ts =  data->getUInt64Value(CmdCalcTrxDelay_TS_PARAM_KEY);
    //    std::cout << "cmd -"<<std::dec<<counter++<<"TS="<<*o_lct_ts<<std::endl;
        LDBG_<<"RT command TS::"<<*o_lct_ts;
}

void CmdCalcTrxDelay::acquireHandler() {
    boost::posix_time::ptime t = boost::posix_time::microsec_clock::local_time();
    *o_lct_delay = t.time_of_day().total_microseconds() - *o_lct_ts;
    LDBG_<<"RT delay TS::"<<*o_lct_delay;
    getAttributeCache()->setOutputDomainAsChanged();
    BC_END_RUNNIG_PROPERTY;

}