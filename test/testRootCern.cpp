/* 
 * File:   testDataSetAttribute.cpp
 * Author: michelo
 * test from UI
 * Created on September 10, 2015, 11:24 AM
 */

#include <ChaosMetadataServiceClient/ChaosMetadataServiceClient.h>

#include "ChaosController.h"
using namespace std;
#include "TROOT.h"
#include "TRint.h"
#include <driver/misc/models/cernRoot/rootUtil.h>
/*
 * 
 */
using namespace driver::misc;
using namespace chaos::metadata_service_client;

int main(int argc, char** argv) {


	ChaosMetadataServiceClient::getInstance()->init(argc,argv);
	ChaosMetadataServiceClient::getInstance()->start();
	 if (0) {
		 TTree* tt= queryChaosTree("buttami","-3m","-1",0,"");
		 printf("%x",tt);

	 }
	TRint *rootapp = new TRint("Rint", &argc, argv);

	  rootapp->Run();

    return 0;
}

