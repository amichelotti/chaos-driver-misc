/*
 *	CmdSync.h
 *	!CHAOS
 *	Created by Andrea Michelotti 13/10/2015
 *
 *    	Copyright 2013 INFN, National Institute of Nuclear Physics
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

#ifndef __CmdSync__
#define __CmdSync__

#include <chaos/cu_toolkit/ControlManager/slow_command/SlowCommand.h>
#include "remoteGroupAccessInterface.h"

namespace c_data = chaos::common::data;
namespace ccc_slow_command = chaos::cu::control_manager::slow_command;


namespace driver {
	namespace misc {
		class CmdSync : public ccc_slow_command::SlowCommand {
                

                   
                protected:
                    remoteGroupAccessInterface *driver;

                
			
		public:
			CmdSync();
			~CmdSync();
		
                	// return the implemented handler
                uint8_t implementedHandler();
			

			// Start the command execution
                void setHandler(c_data::CDataWrapper *data);
                };
		
	}
 }



#endif 
