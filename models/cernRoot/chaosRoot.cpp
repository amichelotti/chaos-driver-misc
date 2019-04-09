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
class ChaosRoot:public TRint {
public:
    ChaosRoot(const char* appClassName, int* argc, char** argv, void* options = 0, int numOptions = 0, Bool_t noLogo = kFALSE):TRint(appClassName, argc, argv, options , numOptions, noLogo){
    }
    ~ChaosRoot(){


    }
    void Terminate(int status){
        std::cout<<"Deinitializing ChaosRoot..."<<std::endl;
        ChaosMetadataServiceClient::getInstance()->stop();
        ChaosMetadataServiceClient::getInstance()->deinit();
        std::cout<<"...done"<<std::endl;
        exit(status);
    }
};
static ChaosRoot *rootapp;
int main(int argc, const char** argv) {
	std::string rootopt;
    const char* root_opts[120];
	int nroot_opts=0;
	std::string buf;
	ChaosMetadataServiceClient::getInstance()->getGlobalConfigurationInstance()->addOption("rootopt", po::value<string>(&rootopt), "Options to give to CERN ROOT interpreter ");
	ChaosMetadataServiceClient::getInstance()->init(argc,argv);
	ChaosMetadataServiceClient::getInstance()->start();
    initChaosRoot();
	 root_opts[nroot_opts++]=argv[0];

	 stringstream ss(rootopt);
	 while (ss >> buf){
         root_opts[nroot_opts++]=buf.c_str();
	 }


    rootapp = new ChaosRoot("Rint", &nroot_opts, (char**)root_opts);
	rootapp->SetPrompt("chaosRoot[%d]>");
	rootapp->Run();
  //  ChaosMetadataServiceClient::getInstance()->stop();
  //  ChaosMetadataServiceClient::getInstance()->deinit();
    return 0;
}

