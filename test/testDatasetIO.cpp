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
   ChaosMetadataServiceClient::getInstance()->start();
    sleep(2);
   ChaosDatasetIO test("IMA/DAQ");
   ChaosDataSet my_ouput=test.allocateDataset(chaos::DataPackCommonKey::DPCK_DATASET_TYPE_OUTPUT);
   ChaosDataSet my_input=test.allocateDataset(chaos::DataPackCommonKey::DPCK_DATASET_TYPE_INPUT);

    my_ouput->addInt64Value("counter64",(int64_t)0);
    my_ouput->addInt32Value("counter32",0);
    my_ouput->addStringValue("stringa","hello dataset");
    my_ouput->addDoubleValue("doublevar",0.0);

    my_input->addInt64Value("icounter64",(int64_t)0);
    my_input->addInt32Value("icounter32",0);
    my_input->addStringValue("istringa","hello input dataset");
    my_input->addDoubleValue("idoublevar",0.0);

    if(test.registerDataset()==0){
        LDBG_<<" registration OK";
        my_input->setValue("icounter64",(int64_t)18);
        my_input->setValue("icounter32",(int32_t)1970);
        my_input->setValue("idoublevar",(double)3.14);
        if(test.pushDataset(chaos::DataPackCommonKey::DPCK_DATASET_TYPE_INPUT)!=0){
            LERR_<<" cannot push:"<<my_input->getJSONString();

        } else {
            LDBG_<<"pushing:"<<my_input->getJSONString();
        }
        for(int cnt=0;cnt<10;cnt++){
            my_ouput->setValue("counter64",(int64_t)2*cnt);
            my_ouput->setValue("counter32",(int32_t)(2*cnt+1));
            my_ouput->setValue("doublevar",(double)cnt);

            LDBG_<<"int32 value:"<<my_ouput->getInt32Value("counter32");
            if(test.pushDataset()!=0){
                LERR_<<" cannot push:"<<my_ouput->getJSONString();

            } else {
                LDBG_<<"pushing:"<<my_ouput->getJSONString();
            }

        }

    } else {
        LERR_<<" cannot register!:";
    }

   ChaosMetadataServiceClient::getInstance()->stop();
    sleep(1);
     ChaosMetadataServiceClient::getInstance()->deinit();
     sleep(1);
     return 0;
}

