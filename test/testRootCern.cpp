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
#include <iostream>
#include <TRandom1.h>
#include <TH1F.h>
#include <TApplication.h>
#include "TROOT.h"
#include "TRint.h"
/*
 * 
 */
using namespace driver::misc;
int main(int argc, char** argv) {

    stringstream is;
    //    if(argc<3){
   //	std::cout<<"Usage is:"<<argv[0]<<" <metadataserver> <CUID>"<<std::endl;
    //	return -1;
    //    }
    //    char*options[]={"app","--metadata-server","localhost:5000"};
  //  is<<"metadata-server="<<argv[1]<<std::endl<<"event-disable=true"<<std::endl;
   // istringstream iis(is.str().c_str());
  //  std::cout<<"options:"<<is.str().c_str();

	ChaosMetadataServiceClient::getInstance()->init(argc,argv);
	ChaosMetadataServiceClient::getInstance()->start();
	TRint *rootapp = new TRint("Rint", &argc, argv);

	//TApplication* rootapp = new TApplication("example",&argc, argv);
	  rootapp->Run();
	  //    ChaosController mycu(argv[2]);
	  //    chaos::common::data::CDataWrapper *d=mycu.fetch(0);

    
	  //     std::cout<<"dataset:"<<d->getJSONString();
    return 0;
}

