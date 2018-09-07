//
//  ChaosMESS.cpp
//  ControlUnitTest
//


#include "ChaosMESS.h"
#include "DefaultCommand.h"
#include "CmdCalcTrxDelay.h"
#include "CmdCalcBandwidth.h"
#include "MessProfilePacketInfo.h"

#include <chaos/common/configuration/GlobalConfiguration.h>

#include <boost/format.hpp>

using namespace chaos::common::data;

using namespace chaos::cu::control_manager::slow_command;
using namespace chaos::cu::driver_manager::driver;
using namespace chaos::common::data::cache;
PUBLISHABLE_CONTROL_UNIT_IMPLEMENTATION(ChaosMESS);
/*
 Construct a new CU with an identifier
 */

ChaosMESS::ChaosMESS(const string& _control_unit_id,
		     const string& _control_unit_param,
		     const ControlUnitDriverList& _control_unit_drivers):
//call base constructor
  chaos::cu::control_manager::SCAbstractControlUnit(_control_unit_id,
						   _control_unit_param,
						   _control_unit_drivers){

}


ChaosMESS::~ChaosMESS() {}

/*
 Return the default configuration
 */
void ChaosMESS::unitDefineActionAndDataset() throw(CException) {
    //create the mess virtual device identifier
	//_deviceID = boost::str( boost::format("%1%_mess_monitor") % GlobalConfiguration::getInstance()->getLocalServerAddress());
	
    //add managed device di
	//setDeviceID(_deviceID);
        
    installCommand(BATCH_COMMAND_GET_DESCRIPTION(DefaultCommand), true);
    installCommand(BATCH_COMMAND_GET_DESCRIPTION(CmdCalcTrxDelay));
    installCommand(BATCH_COMMAND_GET_DESCRIPTION(CmdCalcBandwidth));
   	addActionDescritionInstance<ChaosMESS>(this,
										   &ChaosMESS::getLastTrxDelay,
										   "getLastTrxDelay",
										   "Return the last transmission delay");
	addAttributeToDataSet("trx_ts",
                          "Timestamp received form last transmission delay test",
                          DataType::TYPE_INT64,
                          DataType::Output);

	addAttributeToDataSet("trx_delay",
                          "Last command transmission delay in microseconds",
                          DataType::TYPE_INT64,
                          DataType::Output);
    
    addAttributeToDataSet("profile",
                          "profile info ",
                          DataType::TYPE_BYTEARRAY,
                          DataType::Output,
                          sizeof(MessProfilePacketInfo));

	addAttributeToDataSet("buffer",
                          "trial buffer to calculate perfomance",
                          DataType::TYPE_BYTEARRAY,
                          DataType::Output,
                          1024);
}

void ChaosMESS::defineSharedVariable() {
	//uint32_t quit = false;
	//here are defined the custom shared variable
    //addCustomSharedVariable("quit", 1, chaos::DataType::TYPE_BOOLEAN);
    //setVariableValue(chaos_batch::IOCAttributeSharedCache::SVD_CUSTOM, "quit", &quit, sizeof(bool));
}

void ChaosMESS::unitDefineCustomAttribute() {
	
}

// Abstract method for the initialization of the control unit
void ChaosMESS::unitInit() throw(CException) {
}

// Abstract method for the start of the control unit
void ChaosMESS::unitStart() throw(CException) {
	o_lct_ts = getAttributeCache()->getRWPtr<uint64_t>(DOMAIN_OUTPUT, "trx_ts");
	o_lct_delay = getAttributeCache()->getRWPtr<uint64_t>(DOMAIN_OUTPUT, "trx_delay");
	//o_lct_ts = getVariableValue(chaos_batch::IOCAttributeSharedCache::SVD_OUTPUT, "trx_ts")->getCurrentValue<uint64_t>();;
    //o_lct_delay = getVariableValue(chaos_batch::IOCAttributeSharedCache::SVD_OUTPUT, "trx_delay")->getCurrentValue<uint64_t>();
}

// Abstract method for the stop of the control unit
void ChaosMESS::unitStop() throw(CException) {
	
}

// Abstract method for the deinit of the control unit
void ChaosMESS::unitDeinit() throw(CException) {
	
}

CDWUniquePtr ChaosMESS::getLastTrxDelay(CDWUniquePtr actionParam) {
    if(!o_lct_delay) throw CException(-1, "o_lct_delay not allocated", __PRETTY_FUNCTION__);
	CreateNewDataWrapper(result, );
	result->addInt64Value("trx_delay", *o_lct_delay);
	return result;
}
