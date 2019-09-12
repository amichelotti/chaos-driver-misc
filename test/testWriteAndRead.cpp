/*
 * File:   testDatasetWriteAndRead.cpp
 * Author: michelo
 * test from UI
 * Created on September 10, 2015, 11:24 AM
 */

#include <chaos_metadata_service_client/ChaosMetadataServiceClient.h>
#include <cstdlib>

#include <driver/misc/core/ChaosDatasetIO.h>
using namespace std;
using namespace ::driver::misc;
using namespace chaos::metadata_service_client;
#include <boost/thread.hpp>
#include <math.h>
static int tot_error = 0;
static int exit_after_nerror = 1;
static uint32_t ttl = 7200;
/*
 *
 */
using namespace driver::misc;
static int checkData(ChaosUniquePtr<ChaosDatasetIO> &test,
                     std::vector<ChaosDataSet> &res, uint64_t &pcktmissing,
                     uint64_t &pckt, uint64_t &pcktreplicated,
                     uint64_t &pcktmalformed, uint64_t &badid) {
  int reterr = 0;
  //  uint64_t
  //  end_time=chaos::common::utility::TimingUtil::getLocalTimeStampInMicroseconds();
  //   double avg=total*1000000.0/(end_time-start_time);
  //   printf("Retrived %.8d items items/s:%.4f tot
  //   us:%.10llu\r",res.size(),avg,(end_time-start_time));
  uint64_t cnt = 0;
  for (std::vector<ChaosDataSet>::iterator i = res.begin(); i != res.end();
       i++) {
    if ((*i)->hasKey(
            chaos::ControlUnitNodeDefinitionKey::CONTROL_UNIT_RUN_ID)) {
      uint64_t p = (*i)->getUInt64Value(
          chaos::ControlUnitNodeDefinitionKey::CONTROL_UNIT_RUN_ID);
      if (p != test->getRunID()) {
        std::cout << "\t ##[" << cnt << "] error different runid found:" << p
                  << " expected:" << test->getRunID() << std::endl;

        badid++;
        reterr++;
        continue;
      }
    } else {
      reterr++;
      pcktmalformed++;
    }
    if ((*i)->hasKey(chaos::DataPackCommonKey::DPCK_SEQ_ID)) {
      uint64_t p = (*i)->getUInt64Value(chaos::DataPackCommonKey::DPCK_SEQ_ID);
      if (p > pckt) {
        uint64_t missing = (p - pckt);
        std::cout << "\t ##[" << cnt << "] error missing #pckts:" << missing
                  << " starting from " << chaos::DataPackCommonKey::DPCK_SEQ_ID
                  << "  :" << pckt << " to:" << p << std::endl;
        if (i != res.begin()) {
          LERR_ << "[" << cnt
                << "] MISSING START:" << (*(i - 1))->getCompliantJSONString()
                << " END:" << (*(i))->getCompliantJSONString();
        } else {
          LERR_ << "[" << cnt
                << "] MISSING START FOUND:" << (*(i))->getCompliantJSONString();
        }

        reterr++;
        pckt = (p + 1);
        pcktmissing += missing;
      } else if (p < pckt) {
        pcktreplicated++;
        std::cout << "\t ##[" << cnt << "] error replicated packet id:" << p
                  << " expected:" << pckt << std::endl;
        LERR_ << "[" << cnt
              << "] REPLICATED:" << (*i)->getCompliantJSONString();
        reterr++;
      } else {
        pckt++;
      }
    } else {
      std::cout << "\t ##[" << cnt << "] missing "
                << chaos::DataPackCommonKey::DPCK_SEQ_ID;
      reterr++;
    }
    cnt++;
  }
  return reterr;
}

typedef struct _testParams {
  int thid;
  uint32_t size;
  float payload;
  float push_sec;
  float pull_sec;
  float push_time;
  float pull_time;
  float overhead;
  uint32_t errors;
} testparam_t;
static testparam_t *params;
static testparam_t params_common;

static uint32_t pointmax, pointincr;
static double freqshift = 0.001, ampshift = 0.9999;
static bool binary;
static uint32_t loops;
static uint32_t dssizeb = 1024;
static uint32_t waitloops, wait_retrive;
static uint32_t pagelen = 0;
static const double PI = 3.141592653589793238463;
static int thread_done = 0;
static uint32_t nthreads = 1;
static ofstream fs;
static boost::mutex mutex_sync;

static boost::mutex mutex_thread;
static boost::condition_variable_any cond;

int performTest(const std::string &name, testparam_t &tparam) {
  int countErr = 0;
  auto start = boost::chrono::system_clock::now();
  // Some computation here
  boost::chrono::duration<double> elapsed_seconds; // = end-start;
  char buf[tparam.size];
  uint64_t end_time, start_time;
  double push_avg, push_time,pull_avg,pull_time, bandwithMB;
  uint64_t pckmissing = 0, pcktreplicated = 0, pckmalformed = 0, badid = 0,
           pckt = 0;
  uint32_t total = 0;
  uint64_t query_time_end,query_time_start =
          chaos::common::utility::TimingUtil::getTimeStamp();

  if ((exit_after_nerror > 0) && (tot_error >= exit_after_nerror)) {
    exit(tot_error);
  }
  ChaosUniquePtr<ChaosDatasetIO> test(new ChaosDatasetIO(name, ""));
  ChaosDataSet my_ouput =
      test->allocateDataset(chaos::DataPackCommonKey::DPCK_DATASET_TYPE_OUTPUT);
  my_ouput->addBinaryValue("data", chaos::DataType::SUB_TYPE_INT8,
                           (const char *)buf, tparam.size);
  if (test->registerDataset() == 0) {
    test->setAgeing(ttl);
    
    for (int cnt = 0; cnt < tparam.size / 4; cnt++) {
      unsigned *ptr = (unsigned *)&buf[cnt];
      *ptr = tparam.thid << 24 | cnt;
    }
    start_time =
        chaos::common::utility::TimingUtil::getLocalTimeStampInMicroseconds();
    std::cout << "[" << name << "] Starting Loop ("<<loops<<") Writing ("<<tparam.size<<" bytes) starting at:"<< query_time_start << std::endl;

    for (int cnt = 0; cnt < loops; cnt++) {
      if (test->pushDataset() != 0) {
        LERR_ << " cannot push:" << my_ouput->getJSONString();
        countErr++;
      }
      if (waitloops) {
        usleep(waitloops);
      }
    }
    auto end = boost::chrono::system_clock::now();

    end_time =
        chaos::common::utility::TimingUtil::getLocalTimeStampInMicroseconds();
    push_avg = (loops)*1000000 / (end_time - start_time);
    bandwithMB = (push_avg * tparam.size) / (1024 * 1024);
    push_time= (end_time - start_time);
    query_time_end=          chaos::common::utility::TimingUtil::getTimeStamp();

    std::cout << "[" << name << "] loops:" << loops << " push avg:" << push_avg
              << " push/s, tot us: " << (end_time - start_time)
              << " sizeb:" << tparam.size << " bandwith (MB/s):" << bandwithMB
              << " Total time:" << (push_time)/1000.0<< " ms Ended at:" << query_time_end<< std::endl;
  } else {
    LERR_ << "[" << name << "] cannot register!:";
    countErr++;
    return -1;
  }

  if (wait_retrive) {
    std::cout << "[" << name << "] waiting " << wait_retrive
              << " s before retrive data" << std::endl;
    sleep(wait_retrive);
  }

  int retry = 2;
  int checkErr = 0;

  start_time =
      chaos::common::utility::TimingUtil::getLocalTimeStampInMicroseconds();
  std::cout << "[" << name << "] perform query from " << query_time_start << " page:"<<pagelen<<std::endl;

  if (pagelen == 0) {
    std::vector<ChaosDataSet> res =
        test->queryHistoryDatasets(query_time_start-1000, query_time_end+1000);
    end_time =
        chaos::common::utility::TimingUtil::getLocalTimeStampInMicroseconds();
    pull_time = (end_time - start_time);

    pull_avg = res.size() * 1000000.0 / pull_time;
    total=res.size();
    std::cout << "[" << name << "] retrieved:" << res.size()
              << " items, items/s:" << pull_avg << " time:" << pull_time
              << " us"<<std::endl;
    checkErr = checkData(test, res, pckmissing, pckt, pcktreplicated,
                         pckmalformed, badid);
    countErr += checkErr;
  } else {
    uint32_t uid =
        test->queryHistoryDatasets(query_time_start-1000, query_time_end+1000, pagelen);
    while (test->queryHasNext(uid)) {
      std::vector<ChaosDataSet> res = test->getNextPage(uid);
      total += res.size();
      end_time =
          chaos::common::utility::TimingUtil::getLocalTimeStampInMicroseconds();
      pull_time = (end_time - start_time);
      pull_avg = total * 1000000.0 / pull_time;

      std::cout << "[" << name << "] retrived:" << res.size() << "/" << total
                << " items , items/s:" << pull_avg << " pull time:" << pull_time
                << std::endl;
      checkErr = checkData(test, res, pckmissing, pckt, pcktreplicated,
                           pckmalformed, badid);
      countErr += checkErr;
    }
  }

  if (total != loops) {
    std::cout << "[" << name << "] # number of data retrived " << total
              << " different from expected:" << loops << std::endl;
    countErr++;
    checkErr++;
  }
  if (countErr != 0) {
    std::cout << "[" << name << "] ## Total errors:" << countErr
              << " missing packets:" << pckmissing
              << " replicated:" << pcktreplicated
              << " pcktmalformed:" << pckmalformed << " badrunid:" << badid
              << std::endl;
  } else {
    std::cout << "[" << name << "] check ok" << std::endl;
  }

  tparam.payload = tparam.size / 1024.0;
  tparam.push_sec = push_avg;
  tparam.pull_sec = pull_avg;
  tparam.push_time = push_time;
  tparam.pull_time = pull_time;
  tparam.errors = countErr;


tot_error += countErr;
return countErr;
}
int main(int argc, const char **argv) {
  std::string name, group;

  std::string reportName(argv[0]);
  reportName = reportName + ".csv";
  ChaosMetadataServiceClient::getInstance()
      ->getGlobalConfigurationInstance()
      ->addOption("nerror",
                  po::value<int32_t>(&exit_after_nerror)
                      ->default_value(exit_after_nerror),
                  "Exit after an amout of errors (0=not exit)");

  ChaosMetadataServiceClient::getInstance()
      ->getGlobalConfigurationInstance()
      ->addOption("page", po::value<uint32_t>(&pagelen)->default_value(0),
                  "Page len to recover data");
  ChaosMetadataServiceClient::getInstance()
      ->getGlobalConfigurationInstance()
      ->addOption("ttl", po::value<uint32_t>(&ttl)->default_value(ttl),
                  "Time to live (s)");
  ChaosMetadataServiceClient::getInstance()
      ->getGlobalConfigurationInstance()
      ->addOption(
          "dsname",
          po::value<std::string>(&name)->default_value("PERFORMANCE_MESURE"),
          "name of the dataset (CU)");
  ChaosMetadataServiceClient::getInstance()
      ->getGlobalConfigurationInstance()
      ->addOption("dsgroup",
                  po::value<std::string>(&group)->default_value("DATASETIO"),
                  "name of the group (US)");

  ChaosMetadataServiceClient::getInstance()
      ->getGlobalConfigurationInstance()
      ->addOption("loops", po::value<uint32_t>(&loops)->default_value(1000),
                  "number of push/loop");

  ChaosMetadataServiceClient::getInstance()
      ->getGlobalConfigurationInstance()
      ->addOption("wait", po::value<uint32_t>(&wait_retrive)->default_value(60),
                  "seconds to wait to retrive data after write");
  ChaosMetadataServiceClient::getInstance()
      ->getGlobalConfigurationInstance()
      ->addOption("dssize",
                  po::value<uint32_t>(&dssizeb)->default_value(dssizeb),
                  "Data Set Payload size");
  ChaosMetadataServiceClient::getInstance()
      ->getGlobalConfigurationInstance()
      ->addOption(
          "report",
          po::value<std::string>(&reportName)->default_value(reportName),
          "The report file name");

  ChaosMetadataServiceClient::getInstance()
      ->getGlobalConfigurationInstance()
      ->addOption("nthreads",
                  po::value<uint32_t>(&nthreads)->default_value(nthreads),
                  "Number of concurrent accesses");

  ChaosMetadataServiceClient::getInstance()->init(argc, argv);
  ChaosMetadataServiceClient::getInstance()->start();

  fs.open(reportName);

  boost::thread *workers[nthreads];
  params = new testparam_t[nthreads];

  fs << "th,payload size(Bytes),push/s,pull/s,loop,push time(ms),pull "
        "time(ms),bandwith(MB/s),W (MB/s),R(MB/s),errors"
     << std::endl;

  for (int cnt = 0; cnt < nthreads; cnt++) {
    std::stringstream ss;
    ss << name << "_" << cnt;
    params[cnt].thid = cnt;
    params[cnt].size = dssizeb;
    workers[cnt] = new boost::thread(
        boost::bind(&performTest, ss.str(), boost::ref(params[cnt])));
  }
  sleep(5);
  double tot_size = 0, push_sec = 0, pull_sec = 0, band = 0, errors = 0;
  for (int cnt = 0; cnt < nthreads; cnt++) {
    workers[cnt]->join();
    delete (workers[cnt]);
    fs << cnt << ","<<params[cnt].size << "," << params[cnt].push_sec << ","
       << params[cnt].pull_sec << "," << loops << "," << params[cnt].push_time/1000.0
       << "," << params[cnt].pull_time/1000.0 << ","<< params[cnt].push_sec*params[cnt].size*loops/(1024*1024)<< ","<< params[cnt].pull_sec*params[cnt].size*loops/(1024*1024)<<","<< params[cnt].errors << std::endl;
    tot_size += params[cnt].size;
    push_sec += params[cnt].push_sec;
    pull_sec += params[cnt].pull_sec;
    errors += params[cnt].errors;
  }
  std::cout << "Tot Size(B):" << tot_size << " push/s:" << push_sec
        << " pull/s:" << pull_sec << " errors:" << errors<<std::endl;
  delete[] params;
  ChaosMetadataServiceClient::getInstance()->stop();
  //    ChaosMetadataServiceClient::getInstance()->deinit();
  return tot_error;
}
