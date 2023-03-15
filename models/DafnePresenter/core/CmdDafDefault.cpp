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
#include <common/misc/utility/HttpSender.h>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>
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
	//beamFileElectronPathPointer=getAttributeCache()->getROPtr<char>(DOMAIN_CUSTOM,"beamFilePathE");
	//beamFilePositronPathPointer=getAttributeCache()->getROPtr<char>(DOMAIN_CUSTOM,"beamFilePathP");
	dafnestatPathPointer= getAttributeCache()->getROPtr<char>(DOMAIN_CUSTOM,"newdafnepath");
	outfilePointer= getAttributeCache()->getROPtr<char>(DOMAIN_CUSTOM,"outFileName");
	vugNamePointer= getAttributeCache()->getROPtr<char>(DOMAIN_CUSTOM,"CUvugImportName");
	siddhartaPathPointer= getAttributeCache()->getROPtr<char>(DOMAIN_CUSTOM,"siddhartaPath");
	p_dafne_status_readable=getAttributeCache()->getRWPtr<char>(DOMAIN_OUTPUT,"dafne_status_string");
	CalLumiNamePointer= getAttributeCache()->getROPtr<char>(DOMAIN_CUSTOM,"CULuminometerCCALT");
	graphicServerAddressPointer= getAttributeCache()->getROPtr<char>(DOMAIN_CUSTOM, "GraphicsDataServerAddress");
	graphicsServerAnswer= getAttributeCache()->getRWPtr<int>(DOMAIN_OUTPUT, "GraphicsServer_http_answer");
	std::string toSplit(graphicServerAddressPointer);
	std::istringstream ss(toSplit);
	std::string splitted;
	this->GraphicsAddress = "";
	this->GraphicsPort = "";
	int cnt = 0;
	
	while (std::getline(ss, splitted, ':')) {
		if (cnt == 0)
			this->GraphicsAddress = splitted;
		if (cnt == 1)
			this->GraphicsPort = splitted;
		if (cnt == 2)
		{
			this->GraphicsAddress += ":" + this->GraphicsPort;
			this->GraphicsPort = splitted;
		}
		cnt++;
	}


	
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
	//setFeatures(chaos_batch::features::FeaturesFlagTypes::FF_SET_SCHEDULER_DELAY, (uint64_t)15000000);
	DafneData::DafneDataToShow  DATO;
	//std::string where= dafnestatPathPointer;
	std::string outf=outfilePointer;
	int count,retries=100;
	bool ret, validData;
	/*
	for ( count=0; count < retries; count++)
	{
		ret= DATO.ReadFromNewDafne(dafnestatPathPointer);
		if (!ret)
		{
			usleep(10000);
		}
		else 
		    break;
	}
	if (count > 1)
	{
		metadataLogging(chaos::common::metadata_logging::StandardLoggingChannel::LogLevelWarning, boost::str(boost::format("retried %1% times while reading newdafne file") % count));
	}
		
	validData=ret;
	if (!ret)
	{
		setStateVariableSeverity(StateVariableTypeAlarmCU,"dafne_file_not_found",chaos::common::alarm::MultiSeverityAlarmLevelHigh);
	}
	else
	{
		
		
		//*p_dafne_status=DATO.dafne_status.innerValue;
		//*p_nbunch_ele= DATO.nbunch_ele;
		//*p_nbunch_pos=DATO.nbunch_pos;
		//*p_fill_pattern_ele=DATO.fill_pattern_ele;
		//*p_fill_pattern_pos=DATO.fill_pattern_pos;
		//*p_lifetime_ele=DATO.lifetime_ele;
		//*p_lifetime_pos=DATO.lifetime_pos;
		//*p_rf=DATO.rf.innerValue;
		//*p_ty_ele=DATO.Ty_ele.innerValue;
		//*p_ty_pos=DATO.Ty_pos.innerValue;
		

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
		
		
	}*/
	VUGImporterName=vugNamePointer;
	DAFNESTATImporterDataset=VUGImporter->getLiveChannel(VUGImporterName,0);
	if ((DAFNESTATImporterDataset == NULL) || (DAFNESTATImporterDataset->isEmpty()) )
	{
		SCLERR_ << "DAFNESTATImporterDataset null";
		//metadataLogging(chaos::common::metadata_logging::StandardLoggingChannel::LogLevelError," cannot retrieve dataset DAFNESTATImporterDataset");
		setStateVariableSeverity(StateVariableTypeAlarmCU,"DAFNE_STAT_dataset_invalid_or_null",chaos::common::alarm::MultiSeverityAlarmLevelHigh);
	}
	else
	{
		try
		{
			
			
			
			*p_i_ele = DATO.i_ele.innerValue=DAFNESTATImporterDataset->getDoubleValue("e_current");
		    *p_i_pos = DATO.i_pos.innerValue=DAFNESTATImporterDataset->getDoubleValue("p_current");
			*p_sx_ele = DATO.sx_ele.innerValue = DAFNESTATImporterDataset->getDoubleValue("sigmax_e");
			*p_sy_ele = DATO.sy_ele.innerValue = DAFNESTATImporterDataset->getDoubleValue("sigmay_e");
			*p_sx_pos = DATO.sx_pos.innerValue = DAFNESTATImporterDataset->getDoubleValue("sigmax_p");
			*p_sy_pos = DATO.sy_pos.innerValue = DAFNESTATImporterDataset->getDoubleValue("sigmay_p");

			//Temperature camera Y per adesso non abbiamo modo di leggerle. Rimangono a zero.
			*p_ty_ele=DATO.Ty_ele.innerValue = 0.0;
		    *p_ty_pos=DATO.Ty_pos.innerValue = 0.0;

			//Fake values for retrocompatibility
			*p_fill_pattern_ele=0xDEADDEAD;
			*p_fill_pattern_pos=0xDEADDEAD;

			*p_VUGEL102=DATO.VUGEL102.innerValue/*=DAFNESTATImporterDataset->getDoubleValue("VUGEL102_press")*/ = -1;
			*p_VUGEL103=DATO.VUGEL103.innerValue/*=DAFNESTATImporterDataset->getDoubleValue("VUGEL103_press")*/ =-1;
			*p_VUGEL202=DATO.VUGEL202.innerValue/*=DAFNESTATImporterDataset->getDoubleValue("VUGEL202_press")*/=-1;
			*p_VUGES102=DATO.VUGES102.innerValue/*=DAFNESTATImporterDataset->getDoubleValue("VUGES102_press")*/ = -1;
			*p_VUGES103=DATO.VUGEL103.innerValue/*=DAFNESTATImporterDataset->getDoubleValue("VUGES103_press")*/ = -1;
			*p_VUGES202=DATO.VUGES202.innerValue/*=DAFNESTATImporterDataset->getDoubleValue("VUGES202_press")*/ = -1;

			*p_VUGPL102=DATO.VUGPL102.innerValue/*=DAFNESTATImporterDataset->getDoubleValue("VUGPL102_press")*/ = -1;
			*p_VUGPL103=DATO.VUGPL103.innerValue/*=DAFNESTATImporterDataset->getDoubleValue("VUGPL103_press")*/ = -1;
			*p_VUGPL202=DATO.VUGPL202.innerValue/*=DAFNESTATImporterDataset->getDoubleValue("VUGPL202_press") */= -1;
			*p_VUGPS102=DATO.VUGPS102.innerValue/*=DAFNESTATImporterDataset->getDoubleValue("VUGPS102_press")*/ = -1;
			*p_VUGPS103=DATO.VUGPS103.innerValue/*=DAFNESTATImporterDataset->getDoubleValue("VUGPS103_press")*/ = -1;
			*p_VUGPS202=DATO.VUGPS202.innerValue/*=DAFNESTATImporterDataset->getDoubleValue("VUGPS202_press")*/ = -1;

			*p_VUGPL101=DATO.VUGPL101.innerValue/*=DAFNESTATImporterDataset->getDoubleValue("VUGPL101_press")*/ = -1;
			*p_VUGPS101=DATO.VUGPS101.innerValue/*=DAFNESTATImporterDataset->getDoubleValue("VUGPS101_press")*/ = -1;
			*p_VUGPS201=DATO.VUGPS201.innerValue/*=DAFNESTATImporterDataset->getDoubleValue("VUGPS201_press")*/ = -1;
			*p_VUGPS203=DATO.VUGPS203.innerValue/*=DAFNESTATImporterDataset->getDoubleValue("VUGPS203_press")*/ = -1;
			*p_VUGPL201=DATO.VUGPL201.innerValue/*=DAFNESTATImporterDataset->getDoubleValue("VUGPL201_press")*/ = -1;
			*p_VUGEL101=DATO.VUGEL101.innerValue/*=DAFNESTATImporterDataset->getDoubleValue("VUGEL101_press")*/ = -1;
			*p_VUGES101=DATO.VUGES101.innerValue/*=DAFNESTATImporterDataset->getDoubleValue("VUGES101_press")*/ = -1;
			*p_VUGES201=DATO.VUGES201.innerValue/*=DAFNESTATImporterDataset->getDoubleValue("VUGES201_press")*/ = -1;
			*p_VUGES203=DATO.VUGES203.innerValue/*=DAFNESTATImporterDataset->getDoubleValue("VUGES203_press")*/ = -1;
			*p_VUGEL201=DATO.VUGEL201.innerValue/*=DAFNESTATImporterDataset->getDoubleValue("VUGEL201_press")*/ = -1;
			*p_VUGPL203=DATO.VUGPL203.innerValue/*=DAFNESTATImporterDataset->getDoubleValue("VUGPL203_press")*/ = -1;
			*p_VUGEL203=DATO.VUGEL203.innerValue/*=DAFNESTATImporterDataset->getDoubleValue("VUGEL203_press") */= -1;

			

			int64_t readTS=DAFNESTATImporterDataset->getInt64Value("dpck_ats");
			int64_t now=time(0);
			*p_timestamp=DATO.timestamp.innerValue=now;
			//SCLDBG_ << "read TS "<< readTS << " now "<< now ;
			readTS/=1000;
			if ((now - readTS) > 30)
			{
				setStateVariableSeverity(StateVariableTypeAlarmCU,"DAFNE_STAT_dataset_not_updated",chaos::common::alarm::MultiSeverityAlarmLevelHigh);
				SCLERR_ << "DAFNE_STAT_dataset old";
				validData=false;
			}
			else
			{
				setStateVariableSeverity(StateVariableTypeAlarmCU,"DAFNE_STAT_dataset_not_updated",chaos::common::alarm::MultiSeverityAlarmLevelClear);
				setStateVariableSeverity(StateVariableTypeAlarmCU,"DAFNE_STAT_dataset_invalid_or_null",chaos::common::alarm::MultiSeverityAlarmLevelClear);

			}
            
		}
		catch (chaos::CException)
		{
			setStateVariableSeverity(StateVariableTypeAlarmCU,"DAFNE_STAT_dataset_invalid_or_null",chaos::common::alarm::MultiSeverityAlarmLevelHigh);
			SCLERR_ << "DAFNE_STAT_dataset caught exception";
			validData=false;
		}
	}
	/*Lettura da DAFNE ELAB*/
	DAFNE_ELAB_Dataset=this->DAFNE_ELAB->getLiveChannel("DAFNE/ELAB/DAFNE_STATE",0);
	if ((DAFNE_ELAB_Dataset == NULL)   || (DAFNE_ELAB_Dataset->isEmpty()))
	{
		setStateVariableSeverity(StateVariableTypeAlarmCU,"dafne_elab_dataset_invalid_or_null",chaos::common::alarm::MultiSeverityAlarmLevelHigh);
		SCLERR_ << "DAFNE_ELAB_dataset invalid or null";
	}
	else
	{
		try 
		{
			int errs=0;
			if (DAFNE_ELAB_Dataset->hasKey("e_life"))
				*p_lifetime_ele=DATO.lifetime_ele=DAFNE_ELAB_Dataset->getDoubleValue("e_life");
			else errs++;
			if (DAFNE_ELAB_Dataset->hasKey("p_life"))
				*p_lifetime_pos=DATO.lifetime_pos=DAFNE_ELAB_Dataset->getDoubleValue("p_life");
			else errs++;
			if (DAFNE_ELAB_Dataset->hasKey("dafne_status"))
				*p_dafne_status=DATO.dafne_status=DAFNE_ELAB_Dataset->getInt32Value("dafne_status");
			else errs++;
			if (DAFNE_ELAB_Dataset->hasKey("p_nbunch"))
				*p_nbunch_pos=DATO.nbunch_pos=DAFNE_ELAB_Dataset->getInt32Value("p_nbunch");
			else errs++;
			if (DAFNE_ELAB_Dataset->hasKey("e_nbunch"))
				*p_nbunch_ele=DATO.nbunch_ele=DAFNE_ELAB_Dataset->getInt32Value("e_nbunch");
			else errs++;
			if (DAFNE_ELAB_Dataset->hasKey("rf"))
				*p_rf=DAFNE_ELAB_Dataset->getDoubleValue("rf");
			else errs++;
			int64_t readTS=DAFNE_ELAB_Dataset->getInt64Value("dpck_ats");
			int64_t now=time(0);
			readTS/=1000;
			if ((now - readTS) > 30)
			{
				setStateVariableSeverity(StateVariableTypeAlarmCU,"dafne_elab_dataset_invalid_or_null",chaos::common::alarm::MultiSeverityAlarmLevelHigh);
				SCLERR_ << "DAFNE_ELAB_dataset old";
			}
			if (errs >=5)
			{
				setStateVariableSeverity(StateVariableTypeAlarmCU,"dafne_elab_dataset_invalid_or_null",chaos::common::alarm::MultiSeverityAlarmLevelHigh);
				SCLERR_ << "DAFNE_ELAB_dataset more than 5 errors";
			}
			else if (errs> 0)
			{
				setStateVariableSeverity(StateVariableTypeAlarmCU,"dafne_elab_dataset_invalid_or_null",chaos::common::alarm::MultiSeverityAlarmLevelWarning);
				SCLERR_ << "DAFNE_ELAB_dataset has some error";
			}
			else if (errs == 0)
			{
				setStateVariableSeverity(StateVariableTypeAlarmCU,"dafne_elab_dataset_invalid_or_null",chaos::common::alarm::MultiSeverityAlarmLevelClear);
			}
		}
		catch (chaos::CException)
		{
			setStateVariableSeverity(StateVariableTypeAlarmCU,"dafne_elab_dataset_invalid_or_null",chaos::common::alarm::MultiSeverityAlarmLevelHigh);
			SCLERR_ << "DAFNE_ELAB_dataset generated a exception";
		}
	}
    //Fine lettura DAFNE_ELAB
	//Lettura temperature
	DAFNE_TEMPERATURE_Dataset=this->TEMPImporter->getLiveChannel("DAFNE/MAINRING/TEMP/ALL",0);
	if ((DAFNE_TEMPERATURE_Dataset == NULL)   || (DAFNE_TEMPERATURE_Dataset->isEmpty()))
	{
		setStateVariableSeverity(StateVariableTypeAlarmCU,"dafne_temperature_dataset_invalid_or_null",chaos::common::alarm::MultiSeverityAlarmLevelHigh);
		SCLERR_ << "DAFNE_TEMPERATURE_dataset invalid or null";
	}
	else
	{
		try 
		{
			int errs=0;
			if (DAFNE_TEMPERATURE_Dataset->hasKey("I1003"))
				*p_ty_pos=DATO.Ty_pos=DAFNE_TEMPERATURE_Dataset->getDoubleValue("I1003");
			else
				errs++;
			if (DAFNE_TEMPERATURE_Dataset->hasKey("I1004"))
				*p_ty_ele=DATO.Ty_ele=DAFNE_TEMPERATURE_Dataset->getDoubleValue("I1004");
			else
				errs++;
			int64_t readTS=DAFNE_TEMPERATURE_Dataset->getInt64Value("dpck_ats");
			int64_t now=time(0);
			readTS/=1000;
			if ((now - readTS) > 70)
			{
				setStateVariableSeverity(StateVariableTypeAlarmCU,"dafne_temperature_dataset_invalid_or_null",chaos::common::alarm::MultiSeverityAlarmLevelHigh);
				SCLERR_ << "DAFNE_TEMPERATURE_dataset old";
			}
			else if (errs >0)
			{
				setStateVariableSeverity(StateVariableTypeAlarmCU,"dafne_temperature_dataset_invalid_or_null",chaos::common::alarm::MultiSeverityAlarmLevelHigh);
				SCLERR_ << "DAFNE_ELAB_dataset  errors reading";
			}
			else
			{
				setStateVariableSeverity(StateVariableTypeAlarmCU,"dafne_temperature_dataset_invalid_or_null",chaos::common::alarm::MultiSeverityAlarmLevelClear);
				
			}


		}
		catch (chaos::CException)
		{
			setStateVariableSeverity(StateVariableTypeAlarmCU,"dafne_temperature_dataset_invalid_or_null",chaos::common::alarm::MultiSeverityAlarmLevelHigh);
			SCLERR_ << "DAFNE_TEMPERATURE_dataset generated a exception";
		}
	}



	strncpy(p_dafne_status_readable,getNameForDafneStatus(*p_dafne_status).c_str(),256);
	

	 
	CCALTLumiDataset=CCALT->getLiveChannel(CalLumiNamePointer,0);
	if ((CCALTLumiDataset == NULL)  ||  CCALTLumiDataset->isEmpty())
	{
		setStateVariableSeverity(StateVariableTypeAlarmCU,"CCALT_data_not_retrieved",chaos::common::alarm::MultiSeverityAlarmLevelHigh);
	}
	else
	{
		
		try 
		{
			int errs=0;
			if (CCALTLumiDataset->hasKey("Rate_C2"))
			   *p_R2_CCAL=DATO.R2_CCAL.innerValue=CCALTLumiDataset->getDoubleValue("Rate_C2");
			else errs++;
			if (CCALTLumiDataset->hasKey("DeadTimeFactor"))
			   *p_Dead_TC=DATO.Dead_TC.innerValue=CCALTLumiDataset->getDoubleValue("DeadTimeFactor");
			else errs++;
			if (CCALTLumiDataset->hasKey("R2SectSelLumi"))
			   *p_lum_CCAL=DATO.lum_CCAL.innerValue=CCALTLumiDataset->getDoubleValue("R2SectSelLumi");
			else errs++;
			if (CCALTLumiDataset->hasKey("R1C_ele"))
			   *p_R1C_ele=DATO.R1C_ele.innerValue=CCALTLumiDataset->getDoubleValue("R1C_ele");
			else errs++;
			if (CCALTLumiDataset->hasKey("R1C_pos"))
			   *p_R1C_pos=DATO.R1C_pos.innerValue=CCALTLumiDataset->getDoubleValue("R1C_pos");
			else errs++;
			if (CCALTLumiDataset->hasKey("R2_BKG"))
			   *p_R2_BKG=DATO.R2_BKG.innerValue=CCALTLumiDataset->getDoubleValue("R2_BKG");

			int64_t readTS=CCALTLumiDataset->getInt64Value("dpck_ats");
			int64_t now=time(0);
			//SCLDBG_ << "read TS "<< readTS << " now "<< now ;
			readTS/=1000;
			if ((now - readTS) > 30)
			{
				setStateVariableSeverity(StateVariableTypeAlarmCU,"CCALT_data_not_retrieved",chaos::common::alarm::MultiSeverityAlarmLevelHigh);
			}
			else if (errs > 0)
			{
				setStateVariableSeverity(StateVariableTypeAlarmCU,"CCALT_data_not_retrieved",chaos::common::alarm::MultiSeverityAlarmLevelWarning);
			}
			else
			{
				setStateVariableSeverity(StateVariableTypeAlarmCU, "CCALT_data_not_retrieved", chaos::common::alarm::MultiSeverityAlarmLevelClear);
			}
		}
		catch (chaos::CException)
		{
			setStateVariableSeverity(StateVariableTypeAlarmCU,"CCALT_data_not_retrieved",chaos::common::alarm::MultiSeverityAlarmLevelHigh);
		}

	}

	int32_t sigmaret;
	//setStateVariableSeverity(StateVariableTypeAlarmCU,"beam_file_not_found",chaos::common::alarm::MultiSeverityAlarmLevelClear);
	//setStateVariableSeverity(StateVariableTypeAlarmDEV,"beam_file_not_updated",chaos::common::alarm::MultiSeverityAlarmLevelClear);
	/*sigmaret=DATO.ReadSigmas(beamFileElectronPathPointer,true);
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
	*/
	/*not useful anymore*/
	/*::general::utility::HTTPResponse resp;
	
	::general::utility::HTTPClient   Sender(GraphicsAddress, GraphicsPort);
	resp=Sender.SendHttpPost("/dsdata/api/pushDafneData","application/json;",DATO.AsJsonStr());

	SCLDBG_ << "ALEDEBUG: Sending data to:"<<GraphicsAddress<<":"<<GraphicsPort<<"/dsdata/api/pushDafneData, for graphics returned " << resp.ReturnCode<<" errmsg:"<<resp.ReturnMessage;
    *graphicsServerAnswer = resp.ReturnCode;

	if (resp.ReturnCode != 201)
	{
		setStateVariableSeverity(StateVariableTypeAlarmCU,"push_data_graphics_failed",chaos::common::alarm::MultiSeverityAlarmLevelHigh);
	}
	else
	{
		setStateVariableSeverity(StateVariableTypeAlarmCU,"push_data_graphics_failed",chaos::common::alarm::MultiSeverityAlarmLevelClear);
	}
	*/
	* graphicsServerAnswer = 0;
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
	if (validData)
	{
		ret=DATO.AppendSiddhartaFile(siddhartaPathPointer);
		if (!ret)
		{
			setStateVariableSeverity(StateVariableTypeAlarmCU,"failed_to_write_output_file",chaos::common::alarm::MultiSeverityAlarmLevelHigh);
		}
		else
		{
			setStateVariableSeverity(StateVariableTypeAlarmCU,"failed_to_write_output_file",chaos::common::alarm::MultiSeverityAlarmLevelClear);
		}
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



