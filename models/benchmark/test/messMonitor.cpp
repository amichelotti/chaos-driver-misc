/*	
 *	ControlUnitTest.cpp
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


#include "ChaosMESS.h"
#include "DummyDriver.h"

#include <chaos/common/chaos_constants.h>
#include <chaos/cu_toolkit/ChaosCUToolkit.h>

#include <iostream>
#include <fstream>
#include <string>

#include <boost/shared_ptr.hpp>
#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
/*! 
 
 */
using namespace std;
using namespace chaos;
using namespace chaos::cu;

namespace common_plugin = chaos::common::plugin;
namespace common_utility = chaos::common::utility;
namespace cu_driver_manager = chaos::cu::driver_manager;

#define LOCK_FILE_NAME "/tmp/chaos_mess.lock"

int main (int argc, const char* argv[] ) {
    try {
		std::ofstream os;
		//check if we are the only instance
		os.open(LOCK_FILE_NAME, std::ios_base::out | std::ios_base::binary | std::ios_base::app);
		boost::interprocess::file_lock flock(LOCK_FILE_NAME);
		boost::interprocess::scoped_lock<boost::interprocess::file_lock> e_lock(flock, boost::interprocess::try_to_lock);
		if(e_lock) {
		  ChaosCUToolkit::getInstance()->init(argc, argv);
		  REGISTER_CU(ChaosMESS); 
		  REGISTER_DRIVER(,DummyDriver); 
		  chaos::cu::ChaosCUToolkit::getInstance()->start();
		} else {
			cerr<<"Another instance is already started, only one is permitted"<<endl;
		}
    } catch(boost::interprocess::interprocess_exception e) {
		cerr<<"Another instance is already started, only one is permitted"<<endl;
	} catch (CException& e) {
        cerr<<"Exception::"<<endl;
        std::cerr<< "in:"<<e.errorDomain << std::endl;
        std::cerr<< "cause:"<<e.errorMessage << std::endl;
    } catch (program_options::error &e){
        cerr << "Unable to parse command line: " << e.what() << endl;
    } catch (...){
        cerr << "unexpected exception caught.. " << endl;
    }

    return 0;
}
