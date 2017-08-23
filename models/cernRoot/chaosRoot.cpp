/* 
 * File:   testDataSetAttribute.cpp
 * Author: michelo
 * test from UI
 * Created on September 10, 2015, 11:24 AM
 */


#include <chaos_metadata_service_client/ChaosMetadataServiceClient.h>
#include <driver/misc/core/ChaosController.h>
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
	std::string rootopt;
	char* root_opts[120];
	int nroot_opts=0;
	std::string buf;
	ChaosMetadataServiceClient::getInstance()->getGlobalConfigurationInstance()->addOption("rootopt", po::value<string>(&rootopt), "Options to give tu CERN ROOT interpreter ");
	ChaosMetadataServiceClient::getInstance()->init(argc,argv);
	ChaosMetadataServiceClient::getInstance()->start();
	 if (0) {
		 TTree* tt= queryChaosTree("buttami","-3m","-1",0,"");
		 printf("%x",tt);

	 }
	 root_opts[nroot_opts++]=argv[0];

	 stringstream ss(rootopt);
	 while (ss >> buf){
		 root_opts[nroot_opts++]=strdup(buf.c_str());
	 }


	TRint *rootapp = new TRint("Rint", &nroot_opts, root_opts);
	rootapp->SetPrompt("chaosRoot[%d]>");
	  rootapp->Run();

    return 0;
}

