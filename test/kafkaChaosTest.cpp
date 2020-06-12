/*
 * File:   kestKafkaProducer.cpp
 * Author: michelo

 */
#include <chaos/common/message/MessagePSConsumer.h>
#include <chaos/common/message/MessagePSProducer.h>
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

stats_t writeTest(chaos::common::message::MessagePSProducer &k,
                  const std::string &dsname, int32_t start_seq, int loop,
                  int payload_size) {

  stats_t ret;
  char buffer[payload_size];
  chaos::common::data::CDataWrapper p;
  int errors = 0;
  p.addBinaryValue("payload", buffer, payload_size);
  int cnt = loop;
  p.addInt32Value("counter", start_seq);
  int size = p.getBSONRawSize();
  ret.payload_size = size;
  uint64_t st =
      chaos::common::utility::TimingUtil::getTimeStampInMicroseconds();
  LOG("Perform write test payload " << size << " loops:" << loop << " topic:"
                                    << dsname << " start seq:" << start_seq);
  while (cnt--) {
    p.setValue("counter", start_seq++);

    if (k.pushMsgAsync(p, dsname) != 0) {
      errors++;
    } else {
      ret.tot_bytes += size;
    }
  }
  ret.tot_us =
      chaos::common::utility::TimingUtil::getTimeStampInMicroseconds() - st;
  ret.tot_err = errors;
  return ret;
}
stats_t readTest(chaos::common::message::MessagePSConsumer &k,
                 const std::string &dsname, uint32_t start_seq, int loop) {

  stats_t ret;
  memset(&ret, sizeof(ret), 0);
  int errors = 0;
  int cnt = loop;
  LOG("Perform read test payload loops:" << loop << " topic:" << dsname
                                         << " start seq:" << start_seq);

  uint64_t st =
      chaos::common::utility::TimingUtil::getTimeStampInMicroseconds();

  k.getMsgAsync(dsname, start_seq);
  if (k.waitCompletion() == 0) {
    ret.tot_us =
        chaos::common::utility::TimingUtil::getTimeStampInMicroseconds() - st;

    for (int cnt = 0; cnt < k.msgInQueue(); cnt++) {
      chaos::common::data::CDWShrdPtr p = k.getMsg(cnt);
      LDBG_ << cnt << "] id:" << p->getInt32Value("counter");
      ret.tot_bytes += p->getBSONRawSize();
      ret.payload_size = p->getBSONRawSize();
    }

  } else {
    errors++;
  }

  ret.tot_err = errors;
  return ret;
}

#define FIELDS "payload size,total size,tot time (us), bandwith (MB/s),errors"
using namespace chaos::metadata_service_client::node_controller;
using namespace chaos::common::data;
int main(int argc, const char **argv) {
  int errors = 0;
  std::string server = "localhost:9092";
  std::string dsname = "CHAOS/KAFKA/CU";
  std::string kafkadriver = "asio";
  bool deleteKey = false;
  uint32_t paylod_size = 0;
  uint32_t loop = 1000;
  bool writeenable = true;
  bool readenable = false;
  uint32_t maxpayload = 1024 * 1024;
  std::ofstream wfile("benchmark_write.csv");
  std::ofstream rfile("benchmark_read.csv");

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
          "Kafka driver");

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

  ChaosMetadataServiceClient::getInstance()->init(argc, argv);
  //  ChaosMetadataServiceClient::getInstance()->start();

  if (paylod_size > 0) {
    maxpayload = paylod_size;
  }
  chaos::common::message::MessagePSProducer *prod = NULL;
  chaos::common::message::MessagePSConsumer *cons = NULL;
  if (writeenable) {
    wfile << FIELDS << std::endl;

    prod = new chaos::common::message::MessagePSProducer(kafkadriver);
    prod->addServer(server);
    if (prod->applyConfiguration() == 0) {
      std::cout << "* production configuration ok" << std::endl;
      if (deleteKey) {
        std::cout << "* deleting " << dsname << std::endl;

        prod->deleteKey(dsname);
        if (prod->waitCompletion(10000) != 0) {
          std::cout << "## timeout deleting " << dsname << std::endl;
        }
      }
    }
  }
  uint32_t start_seq = 0;
  if (readenable) {
    rfile << FIELDS << std::endl;

    cons = new chaos::common::message::MessagePSConsumer(kafkadriver);
    cons->addServer(server);
    if (cons->applyConfiguration() == 0) {
      std::cout << "* consumer configuration ok" << std::endl;
    }
  }

    for (int cnt = ((paylod_size > 2) ? paylod_size : 2); cnt <= maxpayload;
         cnt <<= 1) {
      stats_t wstat, rstat;
      if (writeenable) {
        std::stringstream ss;
        wstat = writeTest(*prod, dsname, start_seq, loop, cnt);
        ss << wstat.payload_size << "," << wstat.tot_bytes << ","
           << wstat.tot_us << ","
           << (wstat.tot_bytes * 1000000.0 / (1024.0 * 1024.0 * wstat.tot_us))
           << "," << wstat.tot_err;
        wfile << ss.str() << std::endl;
        LOG(ss.str());
        errors += wstat.tot_err;
      }
      if (readenable) {
        rstat = readTest(*cons, dsname, start_seq, loop);
        std::stringstream ss;

        ss << rstat.payload_size << "," << rstat.tot_bytes << ","
           << rstat.tot_us << ","
           << (rstat.tot_bytes * 1000000.0 / (1024.0 * 1024.0 * rstat.tot_us))
           << "," << rstat.tot_err;
        rfile << ss.str() << std::endl;
        LOG(ss.str());

        errors += rstat.tot_err;
      }
      start_seq += loop;
    }
  
  if (cons) {
    delete cons;
  }
  if (prod) {
    delete prod;
  }

  return errors;
}
