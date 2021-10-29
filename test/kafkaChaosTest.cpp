/*
 * File:   kestKafkaProducer.cpp
 * Author: michelo

 */
#include <chaos/common/message/MessagePSDriver.h>
#include <chaos_metadata_service_client/ChaosMetadataServiceClient.h>

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
      << "- "
#define LOG(x)                                                                 \
  std::cout << PREFIX_LOG << x << std::endl;                                   \
  LDBG_ << x;

//#define ERR LOG
//<<"##"std::cout<<"-"<<chaos::common::utility::TimingUtil::toString(chaos::common::utility::TimingUtil::getTimeStamp())<<"-
//[" << name << "]

typedef struct stats {
  uint32_t tot_bytes;
  uint32_t payload_size;
  uint32_t tot_us;
  uint32_t tot_err;
  stats() : tot_bytes(0), payload_size(0), tot_us(0), tot_err(0) {}
} stats_t;

stats_t writeTest(chaos::common::message::producer_uptr_t &k,
                  const std::string &dsname, int32_t start_seq, int loop,
                  int payload_size) {

  stats_t ret;
  char buffer[payload_size];
  chaos::common::data::CDataWrapper p,r;
  int errors = 0;
  // calculate packet size;
  r.addBinaryValue("payload", buffer, 0);
  r.addInt32Value("counter", start_seq);
  r.addInt32Value("ts",(int32_t) time(NULL));
  if(r.getBSONRawSize()<payload_size){
    if(payload_size==1024*1024){
      payload_size--;
    }
    payload_size=(payload_size-r.getBSONRawSize());
    
  }
  
  p.addBinaryValue("payload", buffer, payload_size);
  int cnt = loop;
  p.addInt32Value("counter", start_seq);
  p.addInt32Value("ts",(int32_t) time(NULL));

  int size = p.getBSONRawSize();
  ret.payload_size = size;
  uint64_t st =
      chaos::common::utility::TimingUtil::getTimeStampInMicroseconds();
  LOG("Perform write test payload " << size << " loops:" << loop << " topic:"
                                    << dsname << " start seq:" << start_seq);
  while (cnt--) {
    p.setValue("counter", start_seq++);

    if (k->pushMsgAsync(p, dsname) != 0) {
      errors++;
    } else {
      ret.tot_bytes += size;
    }
  }
  if(k->flush()!=0){
    LOG("Error flushing all data");
  }
  ret.tot_us =
      chaos::common::utility::TimingUtil::getTimeStampInMicroseconds() - st;
  ret.tot_err = errors;
  
  return ret;
}
stats_t readTest(chaos::common::message::consumer_uptr_t &k,
                 const std::string &dsname, uint32_t start_seq, int loop) {

  stats_t ret;
  memset(&ret, sizeof(ret), 0);
  int errors = 0;
  int cnt = loop;
  int size;
  LOG("Perform read test payload loops:" << loop << " topic:" << dsname
                                         << " start seq:" << start_seq);

  uint64_t st =
      chaos::common::utility::TimingUtil::getTimeStampInMicroseconds();
  k->start();
    for (int cnt = 0; cnt < loop; cnt++) {
            chaos::common::message::ele_uptr_t p = k->getMsg();
      if(p.get()){
        chaos::common::data::CDWUniquePtr& cd=p->cd;
        if(cd.get()){
          uint32_t ts=0;
          if(cd->hasKey("ts")){
            ts=cd->getInt32Value("ts");
          }
          size= cd->getBSONRawSize();
          LDBG_ << cnt << "] id:" << cd->getInt32Value("counter")<<" ts:"<<ts<<"size:"<<size<<" off:"<<p->off<<" par:"<<p->par<<" in queue:"<<k->msgInQueue();
          ret.tot_bytes +=size;
          ret.payload_size = size;
        }
      }
    }
    ret.tot_us =
        chaos::common::utility::TimingUtil::getTimeStampInMicroseconds() - st;

    k->stop();

  

  ret.tot_err = errors;
  return ret;
}

#define FIELDS "payload size,total size,tot time (us), bandwith (MB/s),op/s"
using namespace chaos::metadata_service_client::node_controller;
using namespace chaos::common::data;
int main(int argc, const char **argv) {
  int errors = 0;
  std::string server = "localhost:9092";
  std::string dsname = "CHAOS/KAFKA/CU";
  std::string csvname="benchmark";
  std::string kafkadriver = "kafka-rdk";
  bool deleteKey = false;
  uint32_t paylod_size = 0;
  uint32_t loop = 1000;
  bool writeenable = true;
  bool readenable = false;
  uint32_t maxpayload = 1024 * 1024;
  std::string off_type="latest";
  ChaosMetadataServiceClient::getInstance()
      ->getGlobalConfigurationInstance()
      ->addOption("server",
                  po::value<std::string>(&server)->default_value(server),
                  "A kafka server");

  ChaosMetadataServiceClient::getInstance()
      ->getGlobalConfigurationInstance()
      ->addOption("dsname",
                  po::value<std::string>(&dsname)->default_value(dsname),
                  "topicname/cuname");

  ChaosMetadataServiceClient::getInstance()
      ->getGlobalConfigurationInstance()
      ->addOption("size",
                  po::value<uint32_t>(&paylod_size)->default_value(paylod_size),
                  "Payload size (0 means perform test for sizes power of 2 "
                  "till maxsize)");

  ChaosMetadataServiceClient::getInstance()
      ->getGlobalConfigurationInstance()
      ->addOption("maxsize",
                  po::value<uint32_t>(&maxpayload)->default_value(maxpayload),
                  "max payload size");

  ChaosMetadataServiceClient::getInstance()
      ->getGlobalConfigurationInstance()
      ->addOption("loop", po::value<uint32_t>(&loop)->default_value(loop),
                  "Number of loops");

  ChaosMetadataServiceClient::getInstance()
      ->getGlobalConfigurationInstance()
      ->addOption(
          "driver",
          po::value<std::string>(&kafkadriver)->default_value(kafkadriver),
          "Kafka driver (kafka-rdk | kafka-asio)");

  ChaosMetadataServiceClient::getInstance()
      ->getGlobalConfigurationInstance()
      ->addOption("delete",
                  po::value<bool>(&deleteKey)->default_value(deleteKey),
                  "Delete Topic");

  ChaosMetadataServiceClient::getInstance()
      ->getGlobalConfigurationInstance()
      ->addOption("write",
                  po::value<bool>(&writeenable)->default_value(writeenable),
                  "Perform Write");
  ChaosMetadataServiceClient::getInstance()
      ->getGlobalConfigurationInstance()
      ->addOption("read",
                  po::value<bool>(&readenable)->default_value(readenable),
                  "Perform Read");

ChaosMetadataServiceClient::getInstance()
      ->getGlobalConfigurationInstance()
      ->addOption("csvprefix",
                  po::value<std::string>(&csvname)->default_value(csvname),
                  "CSV prefix");

ChaosMetadataServiceClient::getInstance()
      ->getGlobalConfigurationInstance()
      ->addOption("offtype",
                  po::value<std::string>(&off_type)->default_value(off_type),
                  "Offset Strategy 'earliest' or 'latest'");

  ChaosMetadataServiceClient::getInstance()->init(argc, argv);
  //  ChaosMetadataServiceClient::getInstance()->start();

  if (paylod_size > 0) {
    maxpayload = paylod_size;
  }
  chaos::common::message::consumer_uptr_t cons = chaos::common::message::MessagePSDriver::getConsumerDriver(kafkadriver,"miogruppo");
  chaos::common::message::producer_uptr_t prod = chaos::common::message::MessagePSDriver::getProducerDriver(kafkadriver);
  prod->addServer(server);
  if (prod->applyConfiguration() == 0) {
      std::cout << "* production configuration ok" << std::endl;
      if (deleteKey) {
        std::cout << "* deleting " << dsname << std::endl;
        if (prod->deleteKey(dsname) != 0) {
          std::cout << "## error deleting " << dsname << std::endl;
          return -1;
        }
      }
  }
  std::ofstream wfile(csvname+"_"+kafkadriver+"_"+"write.csv");
  std::ofstream rfile(csvname+"_"+kafkadriver+"_"+"read.csv");

  if (writeenable) {
    wfile << FIELDS << std::endl;
   // prod->setMaxMsgSize(1024*1024);
    if(prod->createKey(dsname)!=0){
       std::cout << "## cannot create  " << dsname << std::endl;
          return -2;
    }

  }
  uint32_t start_seq = 0;
  if (readenable) {
    rfile << FIELDS << std::endl;

    cons->addServer(server);

    
    if(cons->setOption("auto.offset.reset", off_type)!=0){
      std::cerr << "## cannot apply configuration configuration:" <<cons->getLastError()<< std::endl;

    }
    if (cons->applyConfiguration() == 0) {
      std::cout << "* consumer configuration ok" << std::endl;
      if(cons->subscribe(dsname)!=0){
       std::cerr << "## error consumer subscribe:" <<cons->getLastError()<< std::endl;
        return -7;
      }

    } else {
      std::cerr << "## error consumer configuration:" <<cons->getLastError()<< std::endl;
      return -5;
    }
  }

    
      if(writeenable){
          stats_t wstat;

          for (int cnt = ((paylod_size > 2) ? paylod_size : 2); cnt <= maxpayload;
            cnt <<= 1) {
               std::stringstream ss;
                wstat = writeTest(prod, dsname, start_seq, loop, cnt);
                if(wstat.tot_err==0){
                ss << wstat.payload_size << "," << wstat.tot_bytes << ","
                  << wstat.tot_us << ","
                  << (wstat.tot_bytes * 1000000.0 / ( 1024.0 *1024.0* wstat.tot_us))
                  << "," << ((loop * 1000000.0)/wstat.tot_us)<<","<<wstat.tot_err;
                }
                wfile << ss.str() << std::endl;
                LOG(ss.str());
                errors += wstat.tot_err;
                 start_seq += loop;

          }
      }

      start_seq=0;
      if (readenable) {
        stats_t rstat;
        if(writeenable){
          LOG("sleeping 10s before read");
          sleep(10);
        }

        for (int cnt = ((paylod_size > 2) ? paylod_size : 2); cnt <= maxpayload;
            cnt <<= 1) {
               rstat = readTest(cons, dsname, start_seq, loop);
              std::stringstream ss;
              if(rstat.tot_err==0){
              ss << rstat.payload_size << "," << rstat.tot_bytes << ","
                << rstat.tot_us << ","
                << (rstat.tot_bytes * 1000000.0 / (1024.0 * 1024.0* rstat.tot_us))
                << "," << ((loop * 1000000.0)/rstat.tot_us)<<","<<rstat.tot_err;
              }
              rfile << ss.str() << std::endl;
              LOG(ss.str());

              errors += rstat.tot_err;
              start_seq += loop;

            }
      }

    
  
  
  return errors;
}
