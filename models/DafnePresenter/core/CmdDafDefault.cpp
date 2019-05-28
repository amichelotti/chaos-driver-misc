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

std::string getNameForDafneStatus(int32_t status)
{
	switch (status)
	{
		case 0: return "DAFNE: STDBY";
		case 1: return "DAFNE: E- INJECT";
		case 2: return "DAFNE: E+ INJECT";
		case 3: return "DAFNE: E- STORED";
		case 4: return "DAFNE: E+ STORED";
		case 5: return "DAFNE: FILLED";
		case 6: return "DAFNE: COLLIDING";
		case 7: return "DAFNE: BTF DELIVERING";
		case 8: return "DAFNE: BTF DELIVERING & COLLIDING";
		default: return "DAFNE: UNKNOWN";
	}
}

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
	beamFileElectronPathPointer=getAttributeCache()->getROPtr<char>(DOMAIN_CUSTOM,"beamFilePathE");
	beamFilePositronPathPointer=getAttributeCache()->getROPtr<char>(DOMAIN_CUSTOM,"beamFilePathP");
	dafnestatPathPointer= getAttributeCache()->getROPtr<char>(DOMAIN_CUSTOM,"newdafnepath");
	outfilePointer= getAttributeCache()->getROPtr<char>(DOMAIN_CUSTOM,"outFileName");
	vugNamePointer= getAttributeCache()->getROPtr<char>(DOMAIN_CUSTOM,"CUvugImportName");
	siddhartaPathPointer= getAttributeCache()->getROPtr<char>(DOMAIN_CUSTOM,"siddhartaPath");
	p_dafne_status_readable=getAttributeCache()->getRWPtr<char>(DOMAIN_OUTPUT,"dafne_status_string");
	CalLumiNamePointer= getAttributeCache()->getROPtr<char>(DOMAIN_CUSTOM,"CULuminometerCCALT");
	
	
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
	setFeatures(chaos_batch::features::FeaturesFlagTypes::FF_SET_SCHEDULER_DELAY, (uint64_t)15000000);
	DafneData::DafneDataToShow  DATO;
	//std::string where= dafnestatPathPointer;
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
		*p_dafne_status=DATO.dafne_status.innerValue;
		*p_nbunch_ele= DATO.nbunch_ele;
		*p_nbunch_pos=DATO.nbunch_pos;
		*p_fill_pattern_ele=DATO.fill_pattern_ele;
		*p_fill_pattern_pos=DATO.fill_pattern_pos;
		*p_lifetime_ele=DATO.lifetime_ele;
		*p_lifetime_pos=DATO.lifetime_pos;
		*p_rf=DATO.rf.innerValue;
		*p_ty_ele=DATO.Ty_ele.innerValue;
		*p_ty_pos=DATO.Ty_pos.innerValue;
		

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
		strncpy(p_dafne_status_readable,getNameForDafneStatus(DATO.dafne_status).c_str(),256);
		
	}
	VUGImporterName=vugNamePointer;
	
	VUGImporterDataset=VUGImporter->getLiveChannel(VUGImporterName,0);
	if ((VUGImporterDataset == NULL) || (VUGImporterDataset->isEmpty()) )
	{
		SCLERR_ << "VUGImporterDataset null";
		//metadataLogging(chaos::common::metadata_logging::StandardLoggingChannel::LogLevelError," cannot retrieve dataset VUGImporterDataset");
		setStateVariableSeverity(StateVariableTypeAlarmCU,"VUG_dataset_invalid_or_null",chaos::common::alarm::MultiSeverityAlarmLevelHigh);
	}
	else
	{
		try
		{
			
			setStateVariableSeverity(StateVariableTypeAlarmCU,"VUG_dataset_invalid_or_null",chaos::common::alarm::MultiSeverityAlarmLevelClear);
			*p_VUGEL102=DATO.VUGEL102.innerValue=VUGImporterDataset->getDoubleValue("VUGEL102_press");
			*p_VUGEL103=DATO.VUGEL103.innerValue=VUGImporterDataset->getDoubleValue("VUGEL103_press");
			*p_VUGEL202=DATO.VUGEL202.innerValue=VUGImporterDataset->getDoubleValue("VUGEL202_press");
			*p_VUGES102=DATO.VUGES102.innerValue=VUGImporterDataset->getDoubleValue("VUGES102_press");
			*p_VUGES103=DATO.VUGEL103.innerValue=VUGImporterDataset->getDoubleValue("VUGES103_press");
			*p_VUGES202=DATO.VUGES202.innerValue=VUGImporterDataset->getDoubleValue("VUGES202_press");

			*p_VUGPL102=DATO.VUGPL102.innerValue=VUGImporterDataset->getDoubleValue("VUGPL102_press");
			*p_VUGPL103=DATO.VUGPL103.innerValue=VUGImporterDataset->getDoubleValue("VUGPL103_press");
			*p_VUGPL202=DATO.VUGPL202.innerValue=VUGImporterDataset->getDoubleValue("VUGPL202_press");
			*p_VUGPS102=DATO.VUGPS102.innerValue=VUGImporterDataset->getDoubleValue("VUGPS102_press");
			*p_VUGPS103=DATO.VUGPS103.innerValue=VUGImporterDataset->getDoubleValue("VUGPS103_press");
			*p_VUGPS202=DATO.VUGPS202.innerValue=VUGImporterDataset->getDoubleValue("VUGPS202_press");

			*p_VUGPL101=DATO.VUGPL101.innerValue=VUGImporterDataset->getDoubleValue("VUGPL101_press");
			*p_VUGPS101=DATO.VUGPS101.innerValue=VUGImporterDataset->getDoubleValue("VUGPS101_press");
			*p_VUGPS201=DATO.VUGPS201.innerValue=VUGImporterDataset->getDoubleValue("VUGPS201_press");
			*p_VUGPS203=DATO.VUGPS203.innerValue=VUGImporterDataset->getDoubleValue("VUGPS203_press");
			*p_VUGPL201=DATO.VUGPL201.innerValue=VUGImporterDataset->getDoubleValue("VUGPL201_press");
			*p_VUGEL101=DATO.VUGEL101.innerValue=VUGImporterDataset->getDoubleValue("VUGEL101_press");
			*p_VUGES101=DATO.VUGES101.innerValue=VUGImporterDataset->getDoubleValue("VUGES101_press");
			*p_VUGES201=DATO.VUGES201.innerValue=VUGImporterDataset->getDoubleValue("VUGES201_press");
			*p_VUGES203=DATO.VUGES203.innerValue=VUGImporterDataset->getDoubleValue("VUGES203_press");
			*p_VUGEL201=DATO.VUGEL201.innerValue=VUGImporterDataset->getDoubleValue("VUGEL201_press");
			*p_VUGPL203=DATO.VUGPL203.innerValue=VUGImporterDataset->getDoubleValue("VUGPL203_press");
			*p_VUGEL203=DATO.VUGEL203.innerValue=VUGImporterDataset->getDoubleValue("VUGEL203_press");

			int64_t readTS=VUGImporterDataset->getInt64Value("dpck_ats");
			int64_t now=time(0);
			//SCLDBG_ << "read TS "<< readTS << " now "<< now ;
			readTS/=1000;
			if ((now - readTS) > 30)
			{
				setStateVariableSeverity(StateVariableTypeAlarmCU,"VUG_dataset_invalid_or_null",chaos::common::alarm::MultiSeverityAlarmLevelHigh);
			}

		}
		catch (chaos::CException)
		{
			setStateVariableSeverity(StateVariableTypeAlarmCU,"VUG_dataset_invalid_or_null",chaos::common::alarm::MultiSeverityAlarmLevelHigh);
		}
	}
	CCALTLumiDataset=CCALT->getLiveChannel(CalLumiNamePointer,0);
	if ((CCALTLumiDataset == NULL)  ||  CCALTLumiDataset->isEmpty())
	{
		setStateVariableSeverity(StateVariableTypeAlarmCU,"CCALT_data_not_retrieved",chaos::common::alarm::MultiSeverityAlarmLevelHigh);
	}
	else
	{
		setStateVariableSeverity(StateVariableTypeAlarmCU,"CCALT_data_not_retrieved",chaos::common::alarm::MultiSeverityAlarmLevelClear);
		try 
		{
			*p_R1C_ele=DATO.R1C_ele.innerValue=CCALTLumiDataset->getDoubleValue("R1C_ele");
			*p_R1C_pos=DATO.R1C_pos.innerValue=CCALTLumiDataset->getDoubleValue("R1C_pos");
			*p_R2_CCAL=DATO.R2_CCAL.innerValue=CCALTLumiDataset->getDoubleValue("R2_CCAL");
			*p_R2_BKG=DATO.R2_BKG.innerValue=CCALTLumiDataset->getDoubleValue("R2_BKG");
			*p_Dead_TC=DATO.Dead_TC.innerValue=CCALTLumiDataset->getDoubleValue("Dead_TC");
			*p_lum_CCAL=DATO.lum_CCAL.innerValue=CCALTLumiDataset->getDoubleValue("lum_CCAL");

			int64_t readTS=CCALTLumiDataset->getInt64Value("dpck_ats");
			int64_t now=time(0);
			//SCLDBG_ << "read TS "<< readTS << " now "<< now ;
			readTS/=1000;
			if ((now - readTS) > 30)
			{
				setStateVariableSeverity(StateVariableTypeAlarmCU,"CCALT_data_not_retrieved",chaos::common::alarm::MultiSeverityAlarmLevelHigh);
			}
		}
		catch (chaos::CException)
		{
			setStateVariableSeverity(StateVariableTypeAlarmCU,"CCALT_data_not_retrieved",chaos::common::alarm::MultiSeverityAlarmLevelHigh);
		}

	}
	int32_t sigmaret;
	setStateVariableSeverity(StateVariableTypeAlarmCU,"beam_file_not_found",chaos::common::alarm::MultiSeverityAlarmLevelClear);
	setStateVariableSeverity(StateVariableTypeAlarmDEV,"beam_file_not_updated",chaos::common::alarm::MultiSeverityAlarmLevelClear);
	sigmaret=DATO.ReadSigmas(beamFileElectronPathPointer,true);
	switch (sigmaret)
	{
		case -1 : setStateVariableSeverity(StateVariableTypeAlarmCU,"beam_file_not_found",chaos::common::alarm::MultiSeverityAlarmLevelHigh); break;
		case -2 : setStateVariableSeverity(StateVariableTypeAlarmDEV,"beam_file_not_updated",chaos::common::alarm::MultiSeverityAlarmLevelHigh);
		default : *p_sx_ele=DATO.sx_ele.innerValue;
				  *p_sy_ele=DATO.sy_ele.innerValue;

	}
	sigmaret=DATO.ReadSigmas(beamFilePositronPathPointer,false);
	switch (sigmaret)
	{
		case -1 : setStateVariableSeverity(StateVariableTypeAlarmCU,"beam_file_not_found",chaos::common::alarm::MultiSeverityAlarmLevelHigh); break;
		case -2 : setStateVariableSeverity(StateVariableTypeAlarmDEV,"beam_file_not_updated",chaos::common::alarm::MultiSeverityAlarmLevelHigh);
		default : *p_sx_pos=DATO.sx_pos.innerValue;
				  *p_sy_pos=DATO.sy_pos.innerValue;

	}

	kindOfPrint= getAttributeCache()->getROPtr<int32_t>(DOMAIN_INPUT, "printFile");
	ret=true;
	switch (*kindOfPrint)
	{
		case 0:
		default: ret= true; break;
		case 1 :  ret=DATO.PrintAsJson(outf,false); break;
		case 2 :  ret=DATO.PrintAsJson(outf,true); break;
		case 3 :  /*ret=DATO.PrintAsRawtxt(outf);*/ break;
		case 4 :  
		{
			std::string flname=outf+"1";
			ret &=DATO.PrintAsJson(flname,false);
			flname=outf+"2";
			ret &=DATO.PrintAsJson(flname,true);
			flname=outf+"3";
			//ret=DATO.PrintAsRawtxt(flname);

		}
	}
	ret=DATO.AppendSiddhartaFile(siddhartaPathPointer);
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



