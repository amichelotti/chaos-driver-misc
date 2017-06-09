/* 
 * File:   testDataSetAttribute.cpp
 * Author: michelo
 * test from UI
 * Created on September 10, 2015, 11:24 AM
 */

#include <cstdlib>
#include <ChaosMetadataServiceClient/ChaosMetadataServiceClient.h>

#include "ChaosDatasetAttribute.h"
#include "ChaosDatasetAttributeGroup.h"
#include "ChaosController.h"
#include "ChaosControllerGroup.h"
#include "ChaosDatasetAttributeSinchronizer.h"
using namespace std;
using namespace chaos::metadata_service_client;

/*
 * 
 */
using namespace driver::misc;
int main(int argc, char** argv) {

    stringstream is;
    if(argc<3){
    	std::cout<<"Usage is:"<<argv[0]<<" <metadataserver> <CUID>"<<std::endl;
    	return -1;
    }
    is<<"metadata-server="<<argv[1];
    istringstream iis(is.str().c_str());

	ChaosMetadataServiceClient::getInstance()->init(iis);

    ChaosController mycu(argv[2]);
    chaos::common::data::CDataWrapper *d=mycu.fetch(0);

    
     std::cout<<"dataset:"<<d->getJSONString();
    return 0;
}

