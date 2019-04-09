/*
CmdDafDefault.cpp
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
#include "CmdDafDefault.h"

#include "DafneDataPublisher.h"

#include <cmath>
#include  <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <sstream>
#define SCLAPP_ INFO_LOG(CmdDafDefault) << "[" << getDeviceID() << "] "
#define SCLDBG_ DBG_LOG(CmdDafDefault) << "[" << getDeviceID() << "] "
#define SCLERR_ ERR_LOG(CmdDafDefault) << "[" << getDeviceID() << "] "
namespace own = driver::dafnepresenter;
namespace c_data =  chaos::common::data;
namespace chaos_batch = chaos::common::batch_command;
using namespace chaos::cu::control_manager;
BATCH_COMMAND_OPEN_DESCRIPTION(driver::dafnepresenter::,CmdDafDefault,
	"Default command executed when no other commands in queue",
	"9a2582ee-0fc1-420f-bc3d-624640f9fa2a")
BATCH_COMMAND_CLOSE_DESCRIPTION()


// return the implemented handler
uint8_t own::CmdDafDefault::implementedHandler(){
	return      AbstractDafnePresenterCommand::implementedHandler()|chaos_batch::HandlerType::HT_Acquisition;
}
// empty set handler
void own::CmdDafDefault::setHandler(c_data::CDataWrapper *data) {
	AbstractDafnePresenterCommand::setHandler(data);
	clearFeatures(chaos_batch::features::FeaturesFlagTypes::FF_SET_COMMAND_TIMEOUT);
	setBusyFlag(false);
	SCLAPP_ << "Set Handler Default ";
	pastTimestamp=0;
	lastTimeUpdated=0;
	dafnestatPathPointer= getAttributeCache()->getROPtr<char>(DOMAIN_CUSTOM,"newdafnepath");
	outfilePointer= getAttributeCache()->getROPtr<char>(DOMAIN_CUSTOM,"outFileName");

	p_timestamp = getAttributeCache()->getRWPtr<uint64_t>(DOMAIN_OUTPUT, "timestamp");
	p_dafne_status = getAttributeCache()->getRWPtr<int32_t>(DOMAIN_OUTPUT, "dafne_status");
	p_i_ele = getAttributeCache()->getRWPtr<double>(DOMAIN_OUTPUT, "i_ele");
	p_i_pos = getAttributeCache()->getRWPtr<double>(DOMAIN_OUTPUT, "i_pos");
	p_nbunch_ele = getAttributeCache()->getRWPtr<int32_t>(DOMAIN_OUTPUT, "nbunch_ele");
	p_nbunch_pos = getAttributeCache()->getRWPtr<int32_t>(DOMAIN_OUTPUT, "nbunch_pos");
	p_fill_pattern_ele = getAttributeCache()->getRWPtr<int32_t>(DOMAIN_OUTPUT, "fill_pattern_ele");
	p_fill_pattern_pos = getAttributeCache()->getRWPtr<int32_t>(DOMAIN_OUTPUT, "fill_pattern_pos");
	p_lifetime_ele = getAttributeCache()->getRWPtr<int32_t>(DOMAIN_OUTPUT, "lifetime_ele");
	p_lifetime_pos = getAttributeCache()->getRWPtr<int32_t>(DOMAIN_OUTPUT, "lifetime_pos");
	p_sx_ele = getAttributeCache()->getRWPtr<double>(DOMAIN_OUTPUT, "sx_ele");
	p_sy_ele = getAttributeCache()->getRWPtr<double>(DOMAIN_OUTPUT, "sy_ele");
	p_sx_pos = getAttributeCache()->getRWPtr<double>(DOMAIN_OUTPUT, "sx_pos");
	p_sy_pos = getAttributeCache()->getRWPtr<double>(DOMAIN_OUTPUT, "sy_pos");
	p_th_ele = getAttributeCache()->getRWPtr<double>(DOMAIN_OUTPUT, "th_ele");
	p_th_pos = getAttributeCache()->getRWPtr<double>(DOMAIN_OUTPUT, "th_pos");
	p_rf = getAttributeCache()->getRWPtr<double>(DOMAIN_OUTPUT, "rf");

	p_VUGEL101 = getAttributeCache()->getRWPtr<double>(DOMAIN_OUTPUT, "VUGEL101");
	p_VUGEL102 = getAttributeCache()->getRWPtr<double>(DOMAIN_OUTPUT, "VUGEL102");
	p_VUGEL103 = getAttributeCache()->getRWPtr<double>(DOMAIN_OUTPUT, "VUGEL103");
	p_VUGEL201 = getAttributeCache()->getRWPtr<double>(DOMAIN_OUTPUT, "VUGEL201");
	p_VUGEL202 = getAttributeCache()->getRWPtr<double>(DOMAIN_OUTPUT, "VUGEL202");
	p_VUGEL203 = getAttributeCache()->getRWPtr<double>(DOMAIN_OUTPUT, "VUGEL203");

	p_VUGES101 = getAttributeCache()->getRWPtr<double>(DOMAIN_OUTPUT, "VUGES101");
	p_VUGES102 = getAttributeCache()->getRWPtr<double>(DOMAIN_OUTPUT, "VUGES102");
	p_VUGES103 = getAttributeCache()->getRWPtr<double>(DOMAIN_OUTPUT, "VUGES103");
	p_VUGES201 = getAttributeCache()->getRWPtr<double>(DOMAIN_OUTPUT, "VUGES201");
	p_VUGES202 = getAttributeCache()->getRWPtr<double>(DOMAIN_OUTPUT, "VUGES202");
	p_VUGES203 = getAttributeCache()->getRWPtr<double>(DOMAIN_OUTPUT, "VUGES203");

	p_VUGPL101 = getAttributeCache()->getRWPtr<double>(DOMAIN_OUTPUT, "VUGPL101");
	p_VUGPL102 = getAttributeCache()->getRWPtr<double>(DOMAIN_OUTPUT, "VUGPL102");
	p_VUGPL103 = getAttributeCache()->getRWPtr<double>(DOMAIN_OUTPUT, "VUGPL103");
	p_VUGPL201 = getAttributeCache()->getRWPtr<double>(DOMAIN_OUTPUT, "VUGPL201");
	p_VUGPL202 = getAttributeCache()->getRWPtr<double>(DOMAIN_OUTPUT, "VUGPL202");
	p_VUGPL203 = getAttributeCache()->getRWPtr<double>(DOMAIN_OUTPUT, "VUGPL203");

	p_VUGPS101 = getAttributeCache()->getRWPtr<double>(DOMAIN_OUTPUT, "VUGPS101");
	p_VUGPS102 = getAttributeCache()->getRWPtr<double>(DOMAIN_OUTPUT, "VUGPS102");
	p_VUGPS103 = getAttributeCache()->getRWPtr<double>(DOMAIN_OUTPUT, "VUGPS103");
	p_VUGPS201 = getAttributeCache()->getRWPtr<double>(DOMAIN_OUTPUT, "VUGPS201");
	p_VUGPS202 = getAttributeCache()->getRWPtr<double>(DOMAIN_OUTPUT, "VUGPS202");
	p_VUGPS203 = getAttributeCache()->getRWPtr<double>(DOMAIN_OUTPUT, "VUGPS203");

	p_ty_ele = getAttributeCache()->getRWPtr<double>(DOMAIN_OUTPUT, "ty_ele");
	p_ty_pos = getAttributeCache()->getRWPtr<double>(DOMAIN_OUTPUT, "ty_pos");

	p_R2_CCAL = getAttributeCache()->getRWPtr<double>(DOMAIN_OUTPUT, "R2_CCAL");
	p_R2_BKG = getAttributeCache()->getRWPtr<double>(DOMAIN_OUTPUT, "R2_BKG");

	p_Dead_TC = getAttributeCache()->getRWPtr<double>(DOMAIN_OUTPUT, "Dead_TC");
	p_R1C_ele = getAttributeCache()->getRWPtr<double>(DOMAIN_OUTPUT, "R1C_ele");

	p_R1C_pos = getAttributeCache()->getRWPtr<double>(DOMAIN_OUTPUT, "R1C_pos");
	p_lum_CCAL = getAttributeCache()->getRWPtr<double>(DOMAIN_OUTPUT, "lum_CCAL");

	BC_NORMAL_RUNNING_PROPERTY
}
// empty acquire handler
void own::CmdDafDefault::acquireHandler() {
	DafneData::DafneDataToShow  DATO;
	std::string where= dafnestatPathPointer;
	std::string outf=outfilePointer;
	bool ret= DATO.ReadFromNewDafne(dafnestatPathPointer);
	if (!ret)
	{
		setStateVariableSeverity(StateVariableTypeAlarmCU,"dafne_file_not_found",chaos::common::alarm::MultiSeverityAlarmLevelHigh);
	}
	else
	{
		*p_timestamp=DATO.timestamp.innerValue;
		*p_i_ele = DATO.i_ele.innerValue;
		*p_i_pos = DATO.i_pos.innerValue;
		*p_nbunch_ele= DATO.nbunch_ele;
		*p_nbunch_pos=DATO.nbunch_pos;
		*p_fill_pattern_ele=DATO.fill_pattern_ele;
		*p_fill_pattern_pos=DATO.fill_pattern_pos;
		*p_lifetime_ele=DATO.lifetime_ele;
		*p_lifetime_pos=DATO.lifetime_pos;
		*p_rf=DATO.rf;
		setStateVariableSeverity(StateVariableTypeAlarmCU,"dafne_file_not_found",chaos::common::alarm::MultiSeverityAlarmLevelClear);
		
		if (lastTimeUpdated==0)
			lastTimeUpdated=time(NULL);
		if (pastTimestamp == 0)
			pastTimestamp=*p_timestamp;
		uint64_t deltaT = (*p_timestamp -pastTimestamp);
		//SCLDBG_ << "deltaT = " << deltaT;
		if (deltaT != 0)
		{
			//si è aggiornato abbasso allarme
			setStateVariableSeverity(StateVariableTypeAlarmDEV,"dafne_file_not_updated",chaos::common::alarm::MultiSeverityAlarmLevelClear);
			lastTimeUpdated=time(NULL);
			pastTimestamp=*p_timestamp;

			
		}
		else
		{
			uint64_t deltaU =(time(NULL) - this->lastTimeUpdated );
			//SCLDBG_ << "deltaU = " << deltaU;
			if (deltaU > 30)
			{
				if ( deltaU < 60)
				setStateVariableSeverity(StateVariableTypeAlarmDEV,"dafne_file_not_updated",chaos::common::alarm::MultiSeverityAlarmLevelWarning);
			else
				setStateVariableSeverity(StateVariableTypeAlarmDEV,"dafne_file_not_updated",chaos::common::alarm::MultiSeverityAlarmLevelHigh);

			}
			
		}

		
	}
	kindOfPrint= getAttributeCache()->getROPtr<int32_t>(DOMAIN_INPUT, "printFile");
	ret=true;
	switch (*kindOfPrint)
	{
		case 0:
		default: ret= true; break;
		case 1 :  ret=DATO.PrintAsJson(outf,false); break;
		case 2 :  ret=DATO.PrintAsJson(outf,true); break;
		case 3 :  ret=DATO.PrintAsRawtxt(outf); break;
		case 4 :  
		{
			std::string flname=outf+"1";
			ret &=DATO.PrintAsJson(flname,false);
			flname=outf+"2";
			ret &=DATO.PrintAsJson(flname,true);
			flname=outf+"3";
			ret=DATO.PrintAsRawtxt(flname);

		}
	}
	if (!ret)
	{
		setStateVariableSeverity(StateVariableTypeAlarmCU,"failed_to_write_output_file",chaos::common::alarm::MultiSeverityAlarmLevelHigh);
	}
	else
	{
		setStateVariableSeverity(StateVariableTypeAlarmCU,"failed_to_write_output_file",chaos::common::alarm::MultiSeverityAlarmLevelClear);
	}
	
	
	getAttributeCache()->setOutputDomainAsChanged();
}
// empty correlation handler
void own::CmdDafDefault::ccHandler() {
}
// empty timeout handler
bool own::CmdDafDefault::timeoutHandler() {
	SCLDBG_ << "Timeout Handler Default "; 
	return false;
}



