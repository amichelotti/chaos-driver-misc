/* 
 * File:   testDataSetAttribute.cpp
 * Author: michelo
 * test from UI
 * Created on September 10, 2015, 11:24 AM
 */

#include <cstdlib>
#include <ChaosMetadataServiceClient/ChaosMetadataServiceClient.h>
#include "ChaosController.h"
using namespace std;
using namespace chaos::metadata_service_client;
#include <iostream>
#include <TApplication.h>
#include "TROOT.h"
#include "TRint.h"
#include "TTree.h"

TTree* queryChaosTree(const std::string&chaosNode,const std::string& start,const std::string&end,const std::string treeid,int pageLen=1000 ){
	try{
		ChaosController ctrl(chaosNode);

	} catch (chaos::CException e) {
		LERR_ << "[ "<<__PRETTY_FUNCTION__<<"]" << "Exception on \""<<chaosNode<<"\""<< " errn:"<<e.errorCode<<" domain:"<<e.errorDomain<<" what:"<<e.what();
		return NULL;
	} catch (std::exception ee) {
		LERR_ << "[ "<<__PRETTY_FUNCTION__<<"]" << " Library Exception on \""<<chaosNode<<"\""<<" what:"<<e.what();
		return NULL;
	} catch (...) {
		LERR_ << "[ "<<__PRETTY_FUNCTION__<<"]" << " Unexpected Exception on \""<<chaosNode;
		return NULL;
	}
}
