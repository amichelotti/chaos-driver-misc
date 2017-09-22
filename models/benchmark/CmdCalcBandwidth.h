//
//  CmdCalcBandwidth.h
//  chaos-mess
//
//  Created by Andrea Michelotti
//  Copyright (c) 2014 INFN. All rights reserved.
//

#ifndef __chaos_mess__CmdCalcBandwidth__
#define __chaos_mess__CmdCalcBandwidth__
#include <chaos/cu_toolkit/control_manager/slow_command/SlowCommand.h>
#include "MessProfilePacketInfo.h"

using namespace chaos;

namespace c_data = chaos::common::data;
namespace ccc_slow_command = chaos::cu::control_manager::slow_command;


#define CmdCalcBandwidth_CMD_ALIAS		"calc_bandwidth"
#define CmdCalcBandwidth_BYTES_PARAM_KEY	"bytes"
#define CmdCalcBandwidth_REPEAT_PARAM_KEY	"repeat"

DEFINE_BATCH_COMMAND_CLASS(  CmdCalcBandwidth ,ccc_slow_command::SlowCommand) {
	uint8_t *buffer;
    int size;
    int repeat;
    int counter;
    int samples;
    int start;
    int64_t ts_tag;
    int64_t *repetition_stats;
       uint64_t oldtime;
        uint64_t avg_cycle;
        uint64_t avg_net_cycle;
        uint64_t st_time;
        uint64_t prep_time;
        uint64_t start_cycle;
        boost::posix_time::ptime st_prep,end_prep;
	boost::shared_ptr<chaos::common::data::cache::SharedCacheLockDomain> r_lock;
	MessProfilePacketInfo * hdr;
protected:
	// return the implemented handler
    uint8_t implementedHandler();
    
    // Start the command execution
	/*!
	 \param data need contains the "sts" key with transmission time in microseconds
	 */
    void setHandler(c_data::CDataWrapper *data);
    
    void acquireHandler();

public:
	CmdCalcBandwidth();
	~CmdCalcBandwidth();
};

#endif
