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

#ifndef __ControlUnitTest__DefaultCommand__
#define __ControlUnitTest__DefaultCommand__

#include <chaos/cu_toolkit/control_manager/slow_command/SlowCommand.h>

using namespace chaos;

namespace c_data = chaos::common::data;
namespace ccc_slow_command = chaos::cu::control_manager::slow_command;

DEFINE_BATCH_COMMAND_CLASS( DefaultCommand ,ccc_slow_command::SlowCommand) {
	uint64_t *o_lct_delay;
	uint64_t *o_lct_ts;
   protected:
    // return the implemented handler
    uint8_t implementedHandler();
    
    // Start the command execution
    void setHandler(c_data::CDataWrapper *data);
    
    // Aquire the necessary data for the command
    /*!
     The acquire handler has the purpose to get all necessary data need the by CC handler.
     \return the mask for the runnign state
     */
    void acquireHandler();

    
public:
    DefaultCommand();
    ~DefaultCommand();
};

#endif /* defined(__ControlUnitTest__DefaultCommand__) */
