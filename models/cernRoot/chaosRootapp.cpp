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
    ChaosRoot root;
    root.init(argc, argv);
    
    root.start();

    //  ChaosMetadataServiceClient::getInstance()->stop();
    //  ChaosMetadataServiceClient::getInstance()->deinit();
    return 0;
}
