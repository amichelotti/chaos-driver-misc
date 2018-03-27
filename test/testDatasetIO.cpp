/* 
 * File:   testDataSetAttribute.cpp
 * Author: michelo
 * test from UI
 * Created on September 10, 2015, 11:24 AM
 */

#include <cstdlib>
#include <chaos_metadata_service_client/ChaosMetadataServiceClient.h>

#include <driver/misc/core/ChaosDatasetIO.h>
using namespace std;
using namespace ::driver::misc;
using namespace chaos::metadata_service_client;

/*
 * 
 */
using namespace driver::misc;
int main(int argc, char** argv) {



	ChaosMetadataServiceClient::getInstance()->init(argc,argv);
    ChaosDatasetIO test("IMA/TESTDASETIO");
    chaos::common::data::CDataWrapper my_dataset;
    my_dataset.addInt64Value("counter64",(int64_t)0);
    my_dataset.addInt32Value("counter32",0);
    my_dataset.addStringValue("stringa","hello dataset");
    my_dataset.addDoubleValue("doublevar",0.0);
    if(test.registerDataset(my_dataset)==0){
        LDBG_<<" registration OK";
        for(int cnt=0;cnt<10;cnt++){
            my_dataset.setValue("counter64",(int64_t)2*cnt);
            my_dataset.setValue("counter32",(int32_t)(2*cnt+1));
            my_dataset.setValue("doublevar",(double)cnt);
            if(test.pushDataset(&my_dataset)!=0){
                LERR_<<" cannot push:"<<my_dataset.getJSONString();

            } else {
                LDBG_<<"pushing:"<<my_dataset.getJSONString();
            }
        }

    } else {
        LERR_<<" cannot register!:"<<my_dataset.getJSONString();
    }

        return 0;
}

