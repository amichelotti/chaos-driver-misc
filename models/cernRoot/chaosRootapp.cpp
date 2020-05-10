/* 
 * File:   testDataSetAttribute.cpp
 * Author: michelo
 * test from UI
 * Created on September 10, 2015, 11:24 AM
 */

#include "ChaosRoot.h"
using namespace driver::misc::root;
int main(int argc, const char **argv)
{
    std::string rootopt;
    ChaosRoot::getInstance()->getGlobalConfigurationInstance()->addOption< std::string >("node-uid",
                                                                                              "Node Unique Name",
                                                                                              ChaosRoot::getInstance()->uid,
                                                                                              &ChaosRoot::getInstance()->uid);
    ChaosRoot::getInstance()->getGlobalConfigurationInstance()->addOption("rootopt", po::value<std::string>(&rootopt), "Options to give to CERN ROOT interpreter ");
    ChaosRoot::getInstance()->init(argc, argv);
    
    ChaosRoot::getInstance()->setRootOpts(rootopt);
    ChaosRoot::getInstance()->start();

    //  ChaosMetadataServiceClient::getInstance()->stop();
    //  ChaosMetadataServiceClient::getInstance()->deinit();
    return 0;
}
