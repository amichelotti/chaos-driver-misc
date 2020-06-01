/*
 * File:   kestKafkaProducer.cpp
 * Author: michelo
 
 */
#include <chaos_metadata_service_client/ChaosMetadataServiceClient.h>
#include <chaos/common/message/MessagePSProducer.h>
#include <cstdlib>
using namespace std;
using namespace chaos::metadata_service_client;
#include <boost/thread.hpp>
#include <math.h>
static int tot_error = 0;
static int exit_after_nerror = 1;
/*
 *
 */
#define PREFIX_LOG                                                             \
  "-" << chaos::common::utility::TimingUtil::toString(                         \
             chaos::common::utility::TimingUtil::getTimeStamp())               \
      << "- [" << name << "] "
#define LOG(x)                                                                 \
  std::cout << PREFIX_LOG << x << std::endl;                                   \
  LDBG_ << x;

//#define ERR LOG
//<<"##"std::cout<<"-"<<chaos::common::utility::TimingUtil::toString(chaos::common::utility::TimingUtil::getTimeStamp())<<"-
//[" << name << "]


using namespace chaos::metadata_service_client::node_controller;
using namespace chaos::common::data;
int main(int argc, const char **argv) {

  std::string server="localhost:9092";
  std::string dsname="CHAOS/KAFKA/CU";
  uint32_t paylod_size=1024;
  uint32_t loop=1000;
  ChaosMetadataServiceClient::getInstance()
      ->getGlobalConfigurationInstance()
      ->addOption("server",
                  po::value<std::string>(&server)->default_value(server),
                  "A kafka server");

  ChaosMetadataServiceClient::getInstance()
      ->getGlobalConfigurationInstance()
      ->addOption("dsname", po::value<std::string>(&dsname)->default_value(dsname),
                  "topicname/cuname");

  ChaosMetadataServiceClient::getInstance()
      ->getGlobalConfigurationInstance()
      ->addOption("size", po::value<uint32_t>(&paylod_size)->default_value(paylod_size),
                  "Payload size");
ChaosMetadataServiceClient::getInstance()
      ->getGlobalConfigurationInstance()
      ->addOption("loop", po::value<uint32_t>(&loop)->default_value(loop),
                  "Number of loops");
  ChaosMetadataServiceClient::getInstance()->init(argc, argv);
//  ChaosMetadataServiceClient::getInstance()->start();

  chaos::common::message::MessagePSProducer k("RDK");
  k.addServer(server);
  int errors=0;
  if(k.applyConfiguration()==0){
    std::cout << "* configuration ok"<<std::endl;
    char buffer[paylod_size];
    CDataWrapper p;
    p.addBinaryValue("payload",buffer,paylod_size);
    int cnt=loop;
    uint64_t st=chaos::common::utility::TimingUtil::getTimeStampInMicroseconds();
    while(cnt--){
      if(k.pushMsgAsync(p,dsname)!=0){
        errors++;
      }
    }
    uint64_t tm=chaos::common::utility::TimingUtil::getTimeStampInMicroseconds()-st;
    double rate=loop*1000000.0/tm;
    std::cout<<" errors:"<<errors<<" pushed:"<<p.getBSONRawSize()*loop/1024<<" KB,"<<" in "<<tm<<" us,"<<" rate:"<<rate<<std::endl;
  }
  return errors;
}
