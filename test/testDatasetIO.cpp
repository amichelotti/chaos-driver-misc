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
    int reterr=0;
    uint32_t loops;
    uint32_t waitloops;
    ChaosMetadataServiceClient::getInstance()->getGlobalConfigurationInstance()->addOption("loops", po::value<uint32_t>(&loops)->default_value(1000),"number of push/loop");
    ChaosMetadataServiceClient::getInstance()->getGlobalConfigurationInstance()->addOption("wait", po::value<uint32_t>(&waitloops)->default_value(0),"us waits bewteen loops");



    ChaosMetadataServiceClient::getInstance()->init(argc,argv);
    ChaosMetadataServiceClient::getInstance()->start();

   ChaosDatasetIO test("PERFORMANCE_MESURE");
   ChaosDataSet my_ouput=test.allocateDataset(chaos::DataPackCommonKey::DPCK_DATASET_TYPE_OUTPUT);
   ChaosDataSet my_input=test.allocateDataset(chaos::DataPackCommonKey::DPCK_DATASET_TYPE_INPUT);

    my_ouput->addInt64Value("counter64",(int64_t)0);
    my_ouput->addInt32Value("counttoper32",0);
    my_ouput->addStringValue("stringa","hello dataset");
    my_ouput->addDoubleValue("doublevar",0.0);

    my_input->addInt64Value("icounter64",(int64_t)0);
    my_input->addInt32Value("icounter32",0);
    my_input->addStringValue("istringa","hello input dataset");
    my_input->addDoubleValue("idoublevar",0.0);
    int tenpercent=loops/10;
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
        uint64_t query_time_start=chaos::common::utility::TimingUtil::getTimeStamp();
        uint64_t end_time,start_time=chaos::common::utility::TimingUtil::getLocalTimeStampInMicroseconds();
        double avg;
        for(int cnt=0;cnt<loops;cnt++){
            my_ouput->setValue("counter64",(int64_t)2*cnt);
            my_ouput->setValue("counter32",(int32_t)(2*cnt+1));
            my_ouput->setValue("doublevar",(double)cnt);

           // LDBG_<<"int32 value:"<<my_ouput->getInt32Value("counter32");
            if(test.pushDataset()!=0){
                LERR_<<" cannot push:"<<my_ouput->getJSONString();

            }
            if(waitloops){
                usleep(waitloops);
            }
            if((cnt%tenpercent)==0){
                int cntt=cnt+1;
                end_time=chaos::common::utility::TimingUtil::getLocalTimeStampInMicroseconds();
                avg=cntt*1000000.0/(end_time-start_time);
                std::cout<<"Average time for:"<<cntt<<" loops is:"<<avg<<" push/s, tot us: "<<(end_time-start_time)<<std::endl;

            }
        }
        end_time=chaos::common::utility::TimingUtil::getLocalTimeStampInMicroseconds();
        uint64_t query_time_end=chaos::common::utility::TimingUtil::getTimeStamp();

        avg=loops*1000000.0/(end_time-start_time);
        std::cout<<test.getUid()<<": Average time for:"<<loops<<" is:"<<avg<<" push/s, tot us: "<<(end_time-start_time)<<std::endl;
        sleep(1);
        std::cout<<"Recovering data... from:"<<query_time_start<<" to:"<<query_time_end;
        start_time=chaos::common::utility::TimingUtil::getLocalTimeStampInMicroseconds();
        std::vector<ChaosDataSet> res=test.queryHistoryDatasets(query_time_start,query_time_end);
        end_time=chaos::common::utility::TimingUtil::getLocalTimeStampInMicroseconds();
        avg=res.size()*1000000.0/(end_time-start_time);

        std::cout<<"Retrived:"<<res.size()<<" item/s:"<<avg<<" tot us: "<<(end_time-start_time)<<std::endl;
        if(res.size()!=loops){
            std::cout<<"# number of data retrived "<<res.size()<<" different from expected:"<<loops;
            reterr++;
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

