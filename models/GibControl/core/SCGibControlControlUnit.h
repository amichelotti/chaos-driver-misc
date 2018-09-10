/*
SCGibControlControlUnit.h
!CHAOS
Created by CUGenerator

Copyright 2013 INFN, National Institute of Nuclear Physics
Licensed under the Apache License, Version 2.0 (the "License")
you may not use this file except in compliance with the License.
      You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
#ifndef __GibControl__SCGibControlControlUnit__
#define __GibControl__SCGibControlControlUnit__

#include <chaos/cu_toolkit/control_manager/SCAbstractControlUnit.h>
#include <driver/misc/models/GibControl/core/ChaosGibControlInterface.h>
using namespace chaos;
namespace driver {
	namespace gibcontrol {
		class SCGibControlControlUnit: public chaos::cu::control_manager::SCAbstractControlUnit {
			PUBLISHABLE_CONTROL_UNIT_INTERFACE(SCGibControlControlUnit)
			 std::string device_hw;
			chaos::driver::gibcontrol::ChaosGibControlInterface *gibcontrol_drv;
			bool waitOnCommandID(uint64_t command_id);
		private:
		    int32_t numofchannels;
		protected:
			/* Define the Control Unit Dataset and Actions */
			void unitDefineActionAndDataset()throw(chaos::CException);
			void unitDefineCustomAttribute();
			/*(Optional) Initialize the Control Unit and all driver, with received param from MetadataServer*/
			void unitInit() throw(chaos::CException);
			/*(Optional) Execute the work, this is called with a determined delay, it must be as fast as possible*/
			void unitStart() throw(chaos::CException);
			/*(Optional) The Control Unit will be stopped*/
			void unitStop() throw(chaos::CException);
			/*(Optional) The Control Unit will be deinitialized and disposed*/
			void unitDeinit() throw(chaos::CException);
			//!restore method
			bool unitRestoreToSnapshot(chaos::cu::control_manager::AbstractSharedDomainCache * const snapshot_cache) throw(CException);
			// handler declaration
			bool myFunc(const std::string &name,double value,uint32_t size);
			//end handler declaration
		public:
			/*Construct a new CU with an identifier*/
			SCGibControlControlUnit(const std::string& _control_unit_id,const std::string& _control_unit_param,const ControlUnitDriverList& _control_unit_drivers);
			/*Base Destructor*/
			~SCGibControlControlUnit();
			/*handlers*/
			bool PowerOn(const std::string &name,int32_t,uint32_t size);
		};
	}
}
#endif
