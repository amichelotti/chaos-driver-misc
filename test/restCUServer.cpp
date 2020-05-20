/*
 * File:   testDataSetAttribute.cpp
 * Author: michelo
 * test from UI
 * Created on September 10, 2015, 11:24 AM
 */

#include <cstdlib>
#include <chaos_metadata_service_client/ChaosMetadataServiceClient.h>

#include <driver/misc/core/RestCUServer.h>
using namespace std;
using namespace ::driver::misc;
using namespace chaos::metadata_service_client;
/*
 *
 */
#define PREFIX_LOG "-"<<chaos::common::utility::TimingUtil::toString(chaos::common::utility::TimingUtil::getTimeStamp())<<"- [" << name << "] "
#define LOG(x) \
std::cout<<PREFIX_LOG<<x<<std::endl;\
LDBG_<<x;

//#define ERR LOG <<"##"std::cout<<"-"<<chaos::common::utility::TimingUtil::toString(chaos::common::utility::TimingUtil::getTimeStamp())<<"- [" << name << "]

using namespace driver::misc;


int main(int argc, const char **argv) {

  std::string reportName(argv[0]);

  uint32_t port=8091;
  uint32_t thn=16;
  ChaosMetadataServiceClient::getInstance()
      ->getGlobalConfigurationInstance()
      ->addOption("port", po::value<uint32_t>(&port)->default_value(8091),
                  "server listen port");
  ChaosMetadataServiceClient::getInstance()
      ->getGlobalConfigurationInstance()
      ->addOption("nthread", po::value<uint32_t>(&thn)->default_value(16),
                  "number of thread serving the requests");
  
  ChaosMetadataServiceClient::getInstance()->init(argc, argv);
  ChaosMetadataServiceClient::getInstance()->start();
  RestCUServer server(port,thn);

  server.start();
  
  return 0;
}
