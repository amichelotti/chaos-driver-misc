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
    ChaosRoot::getInstance()->getGlobalConfigurationInstance()->addOption("rootopt", po::value<std::string>(&ChaosRoot::getInstance()->rootopts), "Options to give to CERN ROOT interpreter ");
    ChaosRoot::getInstance()->init(argc, argv); 
    ChaosRoot::getInstance()->start();
    //  ChaosMetadataServiceClient::getInstance()->stop();
    //  ChaosMetadataServiceClient::getInstance()->deinit();
    return 0;
}
