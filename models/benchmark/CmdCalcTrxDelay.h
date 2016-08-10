//
//  CmdCalcTrxDelay.h
//  chaos-mess
//
//  Created by Claudio Bisegni on 27/02/14.
//  Copyright (c) 2014 INFN. All rights reserved.
//

#ifndef __chaos_mess__CmdCalcTrxDelay__
#define __chaos_mess__CmdCalcTrxDelay__
#include <chaos/cu_toolkit/control_manager/slow_command/SlowCommand.h>

using namespace chaos;

namespace c_data = chaos::common::data;
namespace ccc_slow_command = chaos::cu::control_manager::slow_command;


#define CmdCalcTrxDelay_CMD_ALIAS		"trx_delay"
#define CmdCalcTrxDelay_TS_PARAM_KEY	"sts"

class CmdCalcTrxDelay  : public ccc_slow_command::SlowCommand {
	uint64_t *o_lct_ts;
	uint64_t *o_lct_delay;
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
	CmdCalcTrxDelay();
	~CmdCalcTrxDelay();
};

#endif /* defined(__chaos_mess__CmdCalcTrxDelay__) */
