/*
 * File:   testDataSetAttribute.cpp
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
  uint32_t points;
  float payload;
  float push_sec;
  float pull_sec;
  uint32_t push_time;
  uint32_t pull_time;
  float bandwith;
  float overhead;
  uint32_t errors;
} testparam_t;
static testparam_t *params;
static testparam_t params_common;

static uint32_t pointmax, pointincr;
static double freqshift = 0.001, ampshift = 0.9999;
static bool binary;
static uint32_t loops;
static uint32_t npoints = 15000;
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
  double freq = 1, phase = 0, amp = 1, afreq, aamp;
  double delta;
  int err = 0;
  for (uint32_t point_cnt = npoints, incr = 2;
       (point_cnt <= pointmax) &&
       ((exit_after_nerror == 0) || (err < exit_after_nerror));
       (incr == 0) ? (point_cnt += pointincr)
                   : (point_cnt = (pow(pointincr, incr))),
                incr++) {
    std::vector<double> val;
    mutex_sync.lock();
    ChaosUniquePtr<ChaosDatasetIO> test(new ChaosDatasetIO(name, ""));
    mutex_sync.unlock();
    ChaosDataSet my_input = test->allocateDataset(
        chaos::DataPackCommonKey::DPCK_DATASET_TYPE_INPUT);
    ChaosDataSet my_ouput = test->allocateDataset(
        chaos::DataPackCommonKey::DPCK_DATASET_TYPE_OUTPUT);
    delta = 1.0 / point_cnt;
    uint64_t push_time = 0, pull_time = 0;
    uint64_t overhead_tot = 0, start_overhead = 0;
    uint64_t overhead_pull_tot = 0, start_pull_overhead = 0;
    float payloadKB, bandwithMB;

    double buf[point_cnt];
    afreq = freq;
    aamp = amp;
    my_ouput->addInt64Value("counter64", (int64_t)0);
    my_ouput->addInt32Value("counttoper32", 0);
    my_ouput->addStringValue("stringa", "hello dataset");
    my_ouput->addDoubleValue("doublevar", 0.0);
    if (point_cnt) {
      if (binary) {
        for (int cnt = 0; cnt < point_cnt; cnt++) {
          double data = sin(2 * PI * freq * (delta * cnt) + phase) * amp;
          buf[cnt] = data;
        }
        my_ouput->addBinaryValue("wave", chaos::DataType::SUB_TYPE_DOUBLE,
                                 (const char *)buf, point_cnt * sizeof(double));
      } else {
        for (int cnt = 0; cnt < point_cnt; cnt++) {
          double data = sin(2 * PI * freq * (delta * cnt) + phase) * amp;
          my_ouput->appendDoubleToArray(data);
          val.push_back(data);
        }
        my_ouput->finalizeArrayForKey("wave");
      }
    }
    my_input->addInt64Value("icounter64", (int64_t)0);
    my_input->addInt32Value("icounter32", 0);
    my_input->addStringValue("istringa", "hello input dataset");
    my_input->addDoubleValue("idoublevar", 0.0);

    int tenpercent = loops / 10;
    if (test->registerDataset() == 0) {
      LDBG_ << " registration OK";
      my_input->setValue("icounter64", (int64_t)18);
      my_input->setValue("icounter32", (int32_t)1970);
      my_input->setValue("idoublevar", (double)3.14);
      if (test->pushDataset(
              chaos::DataPackCommonKey::DPCK_DATASET_TYPE_INPUT) != 0) {
        LERR_ << " cannot push:" << my_input->getJSONString();
        err++;
      } else {
        //  LDBG_<<"pushing:"<<my_input->getJSONString();
      }

      uint64_t query_time_start =
          chaos::common::utility::TimingUtil::getTimeStamp();
      uint64_t end_time, start_time = chaos::common::utility::TimingUtil::
                             getLocalTimeStampInMicroseconds();
      double push_avg, pull_avg;
      for (int cnt = 0; cnt < loops; cnt++) {
        start_overhead = chaos::common::utility::TimingUtil::getTimeStamp();
        my_ouput->setValue("counter64", (int64_t)2 * cnt);
        my_ouput->setValue("counter32", (int32_t)(2 * cnt + 1));
        my_ouput->setValue("doublevar", (double)cnt);
        if (freqshift != 0) {
          afreq += freqshift;
        }

        if (ampshift > 0 || freqshift > 0) {
          aamp = amp;
          for (int cntt = 0; cntt < point_cnt; cntt++) {
            if (ampshift > 0) {
              aamp = aamp * ampshift;
            }
            double data = sin(2 * PI * afreq * (delta * cntt) + phase) * aamp;
            if (binary) {
              buf[cntt] = data;
            } else {
              val[cntt] = data;
            }
          }
          if (binary) {
            my_ouput->setValue("wave", (const void *)buf);
          } else {
            my_ouput->setValue("wave", val);
          }
        }
        overhead_tot +=
            chaos::common::utility::TimingUtil::getTimeStamp() - start_overhead;
        // LDBG_<<"int32 value:"<<my_ouput->getInt32Value("counter32");
        if (test->pushDataset() != 0) {
          LERR_ << " cannot push:" << my_ouput->getJSONString();
          err++;
        }
        if (waitloops) {
          usleep(waitloops);
        }
        /*                if ((cnt % tenpercent) == 0)
        {
            int cntt = cnt + 1;
            end_time =
        chaos::common::utility::TimingUtil::getLocalTimeStampInMicroseconds();
            push_time=(end_time - start_time);
            push_avg = cntt * 1000000.0 / push_time;
            std::cout << "["<<name<<"]\t Average time for:" << cntt << " loops
        is:" << push_avg << " push/s, tot us: " << push_time << std::endl;
            }*/
      }
      end_time =
          chaos::common::utility::TimingUtil::getLocalTimeStampInMicroseconds();
      uint64_t query_time_end =
          chaos::common::utility::TimingUtil::getTimeStamp();
      push_time = (end_time - start_time) - overhead_tot;
      payloadKB = my_ouput->getBSONRawSize();
      push_avg = loops * 1000000.0 / push_time;
      bandwithMB = (payloadKB / (1024.0 * 1024)) * push_avg;
      std::cout << "[" << name
                << "] : Average time payload size(Bytes):" << payloadKB
                << " loops:" << loops << " is:" << push_avg
                << " push/s, tot us: " << push_time << " points:" << point_cnt
                << " bandwith (MB/s):" << bandwithMB
                << " overhead:" << overhead_tot << std::endl;
      if (wait_retrive) {
        std::cout << "[" << name << "] waiting " << wait_retrive
                  << " s before retrive data" << std::endl;
        sleep(wait_retrive);
      }
      std::cout << "[" << name
                << "] retriving data... from:" << query_time_start
                << " to:" << query_time_end << " runID:" << test->getRunID()
                << " pagelen:" << pagelen << std::endl;
      start_time =
          chaos::common::utility::TimingUtil::getLocalTimeStampInMicroseconds();
      uint32_t uid =
          test->queryHistoryDatasets(query_time_start, query_time_end, pagelen);
      uint32_t total = 0;

      uint64_t pckmissing = 0, pcktreplicated = 0, pckmalformed = 0, badid = 0,
               pckt = 0;
      query_time_end += 5000; // consider time errors
      query_time_start -= 2000;
      if (pagelen == 0) {
        std::vector<ChaosDataSet> res =
            test->queryHistoryDatasets(query_time_start, query_time_end);
        end_time = chaos::common::utility::TimingUtil::
            getLocalTimeStampInMicroseconds();
        pull_time = (end_time - start_time);

        pull_avg = res.size() * 1000000.0 / pull_time;
        std::cout << "[" << name << "] retrived:" << res.size()
                  << " items, items/s:" << pull_avg << " time:" << pull_time
                  << std::endl;
        total = res.size();
        err += checkData(test, res, pckmissing, pckt, pcktreplicated,
                         pckmalformed, badid);
      } else {
        while (test->queryHasNext(uid)) {
          std::vector<ChaosDataSet> res = test->getNextPage(uid);
          total += res.size();
          end_time = chaos::common::utility::TimingUtil::
              getLocalTimeStampInMicroseconds();
          pull_time = (end_time - start_time);
          pull_avg = total * 1000000.0 / pull_time;
          std::cout << "[" << name << "] retrived:" << res.size() << "/"
                    << total << " items , items/s:" << pull_avg
                    << " time:" << pull_time << std::endl;

          err += checkData(test, res, pckmissing, pckt, pcktreplicated,
                           pckmalformed, badid);
        }
      }

      if (total != loops) {
        std::cout << "[" << name << "] # number of data retrived " << total
                  << " different from expected:" << loops << std::endl;
        err++;
      }
      if (err != 0) {
        std::cout << "[" << name << "] ## Total errors:" << err
                  << " missing packets:" << pckmissing
                  << " replicated:" << pcktreplicated
                  << " pcktmalformed:" << pckmalformed << " badrunid:" << badid
                  << std::endl;
      } else {
        std::cout << "[" << name << "] check ok" << std::endl;
      }
      tparam.points = point_cnt;
      tparam.payload = payloadKB;
      tparam.push_sec = push_avg;
      tparam.pull_sec = pull_avg;
      tparam.push_time = push_time;
      tparam.pull_time = pull_time;
      tparam.bandwith = bandwithMB;
      tparam.overhead = overhead_tot;
      tparam.errors = err;

    } else {
      LERR_ << "[" << name << "] cannot register!:";
      err++;
    }

    {
      mutex_sync.lock();
      test->deinit();
      if (++thread_done == nthreads) {
        // std::cout <<"["<<name<<"] restart all:" << thread_done<<"
        // points:"<<point_cnt<<std::endl;
        thread_done = 0;
        params_common.points = point_cnt;
        params_common.payload = payloadKB * nthreads;
        params_common.push_sec = 0;
        params_common.pull_sec = 0;
        params_common.push_time = 0;
        params_common.pull_time = 0;
        params_common.bandwith = 0;
        params_common.overhead = 0;
        params_common.errors = 0;
        for (int cnt = 0; cnt < nthreads; cnt++) {
          params_common.push_sec += params[cnt].push_sec;
          params_common.pull_sec += params[cnt].pull_sec;
          params_common.push_time += params[cnt].push_time;
          params_common.pull_time += params[cnt].pull_time;
          params_common.bandwith += params[cnt].bandwith;
          params_common.overhead += params[cnt].overhead;
          params_common.errors += params[cnt].errors;
        }
        params_common.push_time /= nthreads;
        params_common.pull_time /= nthreads;
        params_common.overhead /= nthreads;
        fs << point_cnt << "," << payloadKB << "," << params_common.push_sec
           << "," << params_common.pull_sec << "," << loops << ","
           << params_common.push_time << "," << params_common.pull_time << ","
           << params_common.bandwith << "," << params_common.overhead << ","
           << params_common.errors << "," << nthreads << std::endl;
        std::cout << point_cnt << "," << payloadKB << ","
                  << params_common.push_sec << "," << params_common.pull_sec
                  << "," << loops << "," << params_common.push_time << ","
                  << params_common.pull_time << "," << params_common.bandwith
                  << "," << params_common.overhead << ","
                  << params_common.errors << "," << nthreads << std::endl;

        mutex_sync.unlock();
        cond.notify_all();

      } else {
        mutex_sync.unlock();

        boost::unique_lock<boost::mutex> lock(mutex_thread);
        // std::cout <<"["<<name<<"] waiting:" << thread_done<<"
        // points:"<<point_cnt<<std::endl;
        cond.wait(mutex_thread);
        cond.notify_all();
        // std::cout <<"["<<name<<"] restart:" << thread_done<<"
        // points:"<<point_cnt<<std::endl;
      }
    }
    cond.notify_all();
  }
  tot_error += err;
  return err;
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
      ->addOption("page", po::value<uint32_t>(&pagelen)->default_value(0),
                  "Page len to recover data");

  ChaosMetadataServiceClient::getInstance()
      ->getGlobalConfigurationInstance()
      ->addOption("loops", po::value<uint32_t>(&loops)->default_value(1000),
                  "number of push/loop");
  ChaosMetadataServiceClient::getInstance()
      ->getGlobalConfigurationInstance()
      ->addOption("waitloop", po::value<uint32_t>(&waitloops)->default_value(0),
                  "us waits bewteen loops");
  ChaosMetadataServiceClient::getInstance()
      ->getGlobalConfigurationInstance()
      ->addOption("wait", po::value<uint32_t>(&wait_retrive)->default_value(5),
                  "seconds to wait to retrive data after pushing");
  ChaosMetadataServiceClient::getInstance()
      ->getGlobalConfigurationInstance()
      ->addOption("points",
                  po::value<uint32_t>(&npoints)->default_value(npoints),
                  "Number of sin points, 0 = no wave");
  ChaosMetadataServiceClient::getInstance()
      ->getGlobalConfigurationInstance()
      ->addOption("freqshift",
                  po::value<double>(&freqshift)->default_value(0.001),
                  "Modify freq Hz every loop, 0= no modify");
  ChaosMetadataServiceClient::getInstance()
      ->getGlobalConfigurationInstance()
      ->addOption("ampshift",
                  po::value<double>(&ampshift)->default_value(0.9999),
                  "Modify amplitude every loop, 0= no modify");
  ChaosMetadataServiceClient::getInstance()
      ->getGlobalConfigurationInstance()
      ->addOption("binary", po::value<bool>(&binary)->default_value(false),
                  "The wave is in binary");
  ChaosMetadataServiceClient::getInstance()
      ->getGlobalConfigurationInstance()
      ->addOption(
          "report",
          po::value<std::string>(&reportName)->default_value(reportName),
          "The report file name");
  ChaosMetadataServiceClient::getInstance()
      ->getGlobalConfigurationInstance()
      ->addOption("pointincr",
                  po::value<uint32_t>(&pointincr)->default_value(pointincr),
                  "Increment points by 2^number, from points to pointmaz");
  ChaosMetadataServiceClient::getInstance()
      ->getGlobalConfigurationInstance()
      ->addOption("pointmax",
                  po::value<uint32_t>(&pointmax)->default_value(pointmax),
                  "Max point");

  ChaosMetadataServiceClient::getInstance()
      ->getGlobalConfigurationInstance()
      ->addOption("nthreads",
                  po::value<uint32_t>(&nthreads)->default_value(nthreads),
                  "Number of concurrent accesses");

  ChaosMetadataServiceClient::getInstance()->init(argc, argv);
  ChaosMetadataServiceClient::getInstance()->start();
  if (pointmax == 0) {
    pointmax = npoints;
  }
  fs.open(reportName);
  boost::thread *workers[nthreads];
  params = new testparam_t[nthreads];
  fs << "points,payload size(Bytes),push/s,pull/s,loop,push time(us),pull "
        "time(us),bandwith(MB/s),overhead(us),errors,threads"
     << std::endl;

  for (int cnt = 0; cnt < nthreads; cnt++) {
    std::stringstream ss;
    ss << name << "_" << cnt;
    params[cnt].thid = cnt;
    workers[cnt] = new boost::thread(
        boost::bind(&performTest, ss.str(), boost::ref(params[cnt])));
  }
  sleep(5);
  for (int cnt = 0; cnt < nthreads; cnt++) {
    workers[cnt]->join();
    delete (workers[cnt]);
  }
  delete[] params;
  ChaosMetadataServiceClient::getInstance()->stop();
  //    ChaosMetadataServiceClient::getInstance()->deinit();
  return tot_error;
}
