/*
SCDafnePresenterControlUnit.h
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
#ifndef __DafnePresenter__SCDafnePresenterControlUnit__
#define __DafnePresenter__SCDafnePresenterControlUnit__

#include <chaos/cu_toolkit/control_manager/SCAbstractControlUnit.h>
#include <driver/misc/models/DafnePresenter/core/ChaosDafnePresenterInterface.h>
using namespace chaos;
namespace driver {
	namespace dafnepresenter {
		class SCDafnePresenterControlUnit: public chaos::cu::control_manager::SCAbstractControlUnit {
			PUBLISHABLE_CONTROL_UNIT_INTERFACE(SCDafnePresenterControlUnit)
			std::string device_hw;
			chaos::driver::dafnepresenter::ChaosDafnePresenterInterface *dafnepresenter_drv;
			bool waitOnCommandID(uint64_t command_id);
		protected:
			/* Define the Control Unit Dataset and Actions */
			void unitDefineActionAndDataset();
			void unitDefineCustomAttribute();
			/*(Optional) Initialize the Control Unit and all driver, with received param from MetadataServer*/
			void unitInit();
			/*(Optional) Execute the work, this is called with a determined delay, it must be as fast as possible*/
			void unitStart();
			/*(Optional) The Control Unit will be stopped*/
			void unitStop();
			/*(Optional) The Control Unit will be deinitialized and disposed*/
			void unitDeinit();
			//!restore method
			bool unitRestoreToSnapshot(chaos::cu::control_manager::AbstractSharedDomainCache * const snapshot_cache);
			// handler declaration
			//end handler declaration
		public:
			/*Construct a new CU with an identifier*/
			SCDafnePresenterControlUnit(const std::string& _control_unit_id,const std::string& _control_unit_param,const ControlUnitDriverList& _control_unit_drivers);
			/*Base Destructor*/
			~SCDafnePresenterControlUnit();
			std::string loadedNewDafnePath;
			std::string loadedOutFile;
			std::string loadedVugName;
			std::string loadedCCALTLUMICUName;
			std::string loadedSiddPath;
			std::string loadedBeamElectronPath;
			std::string loadedBeamPositronPath;
		};
	}
}
#endif
