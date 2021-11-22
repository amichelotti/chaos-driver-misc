/*
 * File:   testDataSetAttribute.cpp
 * Author: michelo
 * test from UI
 * Created on September 10, 2015, 11:24 AM
 */

#include <cstdlib>
#include <chaos_service_common/ChaosServiceToolkit.h>
#include <driver/misc/core/ChaosDatasetIO.h>
using namespace std;
using namespace ::driver::misc;
#include <boost/thread.hpp>
#include <math.h>
static int tot_error = 0;
static int exit_after_nerror = 1;
/*
 *
 */
#define PREFIX_LOG "-"<<chaos::common::utility::TimingUtil::toString(chaos::common::utility::TimingUtil::getTimeStamp())<<"- [" << name << "] "
#define LOG(x) \
std::cout<<PREFIX_LOG<<x<<std::endl;\
LDBG_<<x;

//#define ERR LOG <<"##"std::cout<<"-"<<chaos::common::utility::TimingUtil::toString(chaos::common::utility::TimingUtil::getTimeStamp())<<"- [" << name << "]

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
       i++, cnt++) {
    uint64_t runid = 0, timestamp = 0, seq = 0;
    if(i->get()==NULL){
            std::cout << "\t  " << cnt << " packet empty "<<std::endl;
            continue;
    }
      continue;
    if (!(*i)->hasKey(
            chaos::ControlUnitNodeDefinitionKey::CONTROL_UNIT_RUN_ID) ||
        !(*i)->hasKey(
            chaos::NodeHealtDefinitionKey::NODE_HEALT_MDS_TIMESTAMP) ||
        !(*i)->hasKey(chaos::DataPackCommonKey::DPCK_SEQ_ID)) {
      reterr++;
      pcktmalformed++;
      std::cout << "\t ##[ " << cnt << " packet malformed missing some index :"
                << (*i)->getJSONString();
      continue;
    }
    runid = (*i)->getUInt64Value(
        chaos::ControlUnitNodeDefinitionKey::CONTROL_UNIT_RUN_ID);
    timestamp = (*i)->getUInt64Value(
        chaos::NodeHealtDefinitionKey::NODE_HEALT_MDS_TIMESTAMP);
    seq = (*i)->getUInt64Value(chaos::DataPackCommonKey::DPCK_SEQ_ID);

    if (runid != test->getRunID()) {
      std::cout << "\t ##[" << cnt << " r:" << runid << ",s:" << seq
                << ",t:" << timestamp << " ("
                << chaos::common::utility::TimingUtil::toString(timestamp)
                << ")] error different runid found:" << runid
                << " expected:" << test->getRunID() << std::endl;

      badid++;
      reterr++;
      cnt++;
      continue;
    }

    if (seq > pckt) {
      uint64_t missing = (seq - pckt);
      std::cout << "\t ##[" << cnt << " r:" << runid << ",s:" << seq
                << ",t:" << timestamp << " ("
                << chaos::common::utility::TimingUtil::toString(timestamp)
                << ")] error missing #pckts:" << missing << " starting from "
                << chaos::DataPackCommonKey::DPCK_SEQ_ID << "  :" << pckt
                << " to:" << seq << std::endl;
      if (i != res.begin()) {
        LERR_ << "\t ##[" << cnt << " r:" << runid << ",s:" << seq
              << ",t:" << timestamp << " ("
              << chaos::common::utility::TimingUtil::toString(timestamp)
              << ")] MISSING START:" << (*(i - 1))->getCompliantJSONString()
              << " END:" << (*(i))->getCompliantJSONString();
      } else {
        LERR_ << "\t ##[" << cnt << " r:" << runid << ",s:" << seq
              << ",t:" << timestamp << " ("
              << chaos::common::utility::TimingUtil::toString(timestamp)
              << ")] MISSING START FOUND:" << (*(i))->getCompliantJSONString();
      }

      reterr++;
      pckt = (seq + 1);
      pcktmissing += missing;
    } else if (seq < pckt) {
      pcktreplicated++;
      std::cout << "\t ##[" << cnt << " r:" << runid << ",s:" << seq
                << ",t:" << timestamp << " ("
                << chaos::common::utility::TimingUtil::toString(timestamp)
                << ")]error replicated packet id:" << seq
                << " expected:" << pckt << std::endl;
      LERR_ << "\t ##[" << cnt << " r:" << runid << ",s:" << seq
            << ",t:" << timestamp << " ("
            << chaos::common::utility::TimingUtil::toString(timestamp)
            << ")] REPLICATED:" << (*i)->getCompliantJSONString();
      reterr++;
    } else {
      pckt++;
    }
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
  uint32_t werrors;
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
static boost::atomic<int> thread_done;
static uint32_t nthreads = 1;
static ofstream fs;

static boost::mutex mutex_thread;
static boost::condition_variable_any cond;

int performTest(const std::string &name, testparam_t &tparam) {
  double freq = 1, phase = 0, amp = 1, afreq, aamp;
  double delta;
  int countErr = 0;
  int writeErr=0;
  auto start = boost::chrono::system_clock::now();
  // Some computation here
  auto end = boost::chrono::system_clock::now();
  boost::chrono::duration<double> elapsed_seconds; // = end-start;

  if ((exit_after_nerror > 0) && (tot_error >= exit_after_nerror)) {
    exit(tot_error);
  }
  for (uint32_t point_cnt = npoints, incr = 2;
       (point_cnt <= pointmax) &&
       ((exit_after_nerror == 0) || (tot_error < exit_after_nerror));
       (incr == 0) ? (point_cnt += pointincr)
                   : (point_cnt = (pow(pointincr, incr))),
                incr++) {
    std::vector<double> val;
    ChaosUniquePtr<ChaosDatasetIO> test(new ChaosDatasetIO(name, false));
    if(test.get()==NULL){
            LOG(" cannot allocate TEST");
            return -1;

    }
    test->setAgeing(7200);
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
     if(my_input.get()==NULL){
            LOG(" cannot allocate input dataset");
            return -2;

    }
    if(my_ouput.get()==NULL){
            LOG(" cannot allocate output dataset");
            return -2;

    }
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
                                 (const char *)buf, sizeof(buf));
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
      LOG( "registration OK");
      my_input->setValue("icounter64", (int64_t)18);
      my_input->setValue("icounter32", (int32_t)1970);
      my_input->setValue("idoublevar", (double)3.14);
      if (test->pushDataset(
              chaos::DataPackCommonKey::DPCK_DATASET_TYPE_INPUT) != 0) {
        LERR_ << " cannot push:" << my_input->getJSONString();
        countErr++;
        writeErr++;
      } else {
        //  LDBG_<<"pushing:"<<my_input->getJSONString();
      }
      
      sleep(5);
      LOG(" Starting write test payload output:"<<my_ouput->getBSONRawSize()<< "B, input:"<<my_input->getBSONRawSize()<<" B loops:"<<loops);

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
            my_ouput->setValue("wave", (const void *)buf,sizeof(buf));
          } else {
            my_ouput->setValue("wave", val);
          }
        }
        overhead_tot +=
            chaos::common::utility::TimingUtil::getTimeStamp() - start_overhead;
        // LDBG_<<"int32 value:"<<my_ouput->getInt32Value("counter32");
        if (test->pushDataset() != 0) {
          LERR_ << "## cannot push:" << my_ouput->getJSONString();
          writeErr++;
          LOG(  "# error writing ["<<writeErr<<"]");

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
      boost::chrono::duration<double> dursec = boost::chrono::system_clock::now() - start;
      end = boost::chrono::system_clock::now();
      LOG (" [" << chaos::common::utility::TimingUtil::toString(
                       query_time_start)
                << "] to:" << query_time_end << "["
                << chaos::common::utility::TimingUtil::toString(query_time_end)
                << "] runID:" << test->getRunID()
                << " Average time payload size(Bytes):" << payloadKB
                << " loops:" << loops << " is:" << push_avg
                << " push/s, tot us: " << push_time << " points:" << point_cnt
                << " bandwith (MB/s):" << bandwithMB
                << " Total transferred (KB):"<<(loops*payloadKB*1.0)/1024.0
                << " overhead:" << overhead_tot
                << " Total time:" << dursec.count()/1000.0 << " s");
      if (wait_retrive) {
        LOG( " waiting " << wait_retrive << " s before retrive data");
        sleep(wait_retrive);
      }

      query_time_end += 1000; // consider time errors
      query_time_start -= 1000;
      uint32_t total = 0;
      uint64_t pckmissing = 0, pcktreplicated = 0, pckmalformed = 0, badid = 0,
               pckt = 0;

      LOG(" retriving data... from:" << query_time_start << "["
                << chaos::common::utility::TimingUtil::toString(
                       query_time_start)
                << "] to:" << query_time_end << "["
                << chaos::common::utility::TimingUtil::toString(query_time_end)
                << "] runID:" << test->getRunID() << " pagelen:" << pagelen);
      start_time =
          chaos::common::utility::TimingUtil::getLocalTimeStampInMicroseconds();
      int retry = 2;
      int checkErr = 0;
      if (pagelen == 0) {
        std::vector<ChaosDataSet> res =
            test->queryHistoryDatasets(query_time_start, query_time_end);
        end_time = chaos::common::utility::TimingUtil::
            getLocalTimeStampInMicroseconds();
        pull_time = (end_time - start_time);

        pull_avg = res.size() * 1000000.0 / pull_time;
        double mb=0;
        if(res.size()){
          mb=res.size()*res[0]->getBSONRawSize()/(1024.0*1024);
          LOG(" retrived:" << res.size()
                    << " items, items/s:" << pull_avg << " time:" << pull_time<< " tot:"<<mb<< "MB "<<(mb* 1000000.0 / pull_time)<<" MB/s");
          total = res.size();
          checkErr = checkData(test, res, pckmissing, pckt, pcktreplicated,
                              pckmalformed, badid);
        } else {
          LOG(" NOTHING RETRIVED :" << res.size()
                    << " items, items/s:" << pull_avg << " time:" << pull_time<< " tot:"<<mb<< "MB "<<(mb* 1000000.0 / pull_time)<<" MB/s");
            countErr++;
          
        }
          countErr += checkErr;
      } else {

        uint32_t uid = test->queryHistoryDatasets(query_time_start,
                                                  query_time_end, pagelen);

        while (test->queryHasNext(uid)) {
          std::vector<ChaosDataSet> res = test->getNextPage(uid);
          total += res.size();
          end_time = chaos::common::utility::TimingUtil::
              getLocalTimeStampInMicroseconds();
          pull_time = (end_time - start_time);
          pull_avg = total * 1000000.0 / pull_time;
          end = boost::chrono::system_clock::now();

          LOG(" retrived:" << res.size() << "/"
                    << total << " items , items/s:" << pull_avg
                    << " pull time:" << pull_time
                    << " Total Time:" << (end - start).count() << " s");
          checkErr = checkData(test, res, pckmissing, pckt, pcktreplicated,
                               pckmalformed, badid);
          countErr += checkErr;
        }
      }
      if (total != loops) {
        LOG(" # number of data retrived " << total
                  << " different from expected:" << loops);
        countErr++;
        checkErr++;
        pckmissing++;
      }
      if (pckmissing == 0 && pcktreplicated == 0 && pckmalformed == 0 &&
          badid == 0) {
        LOG(" check ok");
      }
      if ((countErr != 0) || (writeErr!=0)) {
        LOG("## Total errors:" << (countErr+writeErr)
                  << " write error:" << writeErr
                  << " missing packets:" << pckmissing
                  << " replicated:" << pcktreplicated
                  << " pcktmalformed:" << pckmalformed << " badrunid:" << badid);
      }

      tparam.points = point_cnt;
      tparam.payload = payloadKB;
      tparam.push_sec = push_avg;
      tparam.pull_sec = pull_avg;
      tparam.push_time = push_time;
      tparam.pull_time = pull_time;
      tparam.bandwith = bandwithMB;
      tparam.overhead = overhead_tot;
      tparam.errors = countErr+writeErr;
      tparam.werrors = writeErr;


    } else {
      LOG("## [" << name << "] cannot register!:");
      countErr++;
    }

    {
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
        params_common.werrors = 0;

        for (int cnt = 0; cnt < nthreads; cnt++) {
          params_common.push_sec += params[cnt].push_sec;
          params_common.pull_sec += params[cnt].pull_sec;
          params_common.push_time += params[cnt].push_time;
          params_common.pull_time += params[cnt].pull_time;
          params_common.bandwith += params[cnt].bandwith;
          params_common.overhead += params[cnt].overhead;
          params_common.errors += params[cnt].errors;
          params_common.werrors += params[cnt].werrors;

        }
        params_common.push_time /= nthreads;
        params_common.pull_time /= nthreads;
        params_common.overhead /= nthreads;
        fs << point_cnt << "," << payloadKB << "," << params_common.push_sec
           << "," << params_common.pull_sec << "," << loops << ","
           << params_common.push_time << "," << params_common.pull_time << ","
           << params_common.bandwith << "," << params_common.overhead << ","
           << params_common.errors << "," << nthreads << ","<< params_common.werrors<<std::endl;
     /*   std::cout << point_cnt << "," << payloadKB << ","
                  << params_common.push_sec << "," << params_common.pull_sec
                  << "," << loops << "," << params_common.push_time << ","
                  << params_common.pull_time << "," << params_common.bandwith
                  << "," << params_common.overhead << ","
                  << params_common.errors << "," << nthreads << std::endl;*/

        cond.notify_all();

      } else {

        boost::mutex::scoped_lock lock(mutex_thread);
        LOG( "[" << name << "] waiting:" << thread_done
                  << " points:" << point_cnt);
        cond.wait_for(mutex_thread,boost::chrono::seconds(120));
        LOG( "[" << name << "] done");
        
        // std::cout <<"["<<name<<"] restart:" << thread_done<<"
        // points:"<<point_cnt<<std::endl;
      }
    }
      tot_error += countErr;

  }
  cond.notify_all();

  return countErr;
}
int main(int argc, const char **argv) {
  std::string name, group;

  std::string reportName(argv[0]);
  reportName = reportName + ".csv";
  chaos::service_common::ChaosServiceToolkit::getInstance()
      ->getGlobalConfigurationInstance()
      ->addOption("nerror",
                  po::value<int32_t>(&exit_after_nerror)
                      ->default_value(exit_after_nerror),
                  "Exit after an amout of errors (0=not exit)");

   chaos::service_common::ChaosServiceToolkit::getInstance()
      ->getGlobalConfigurationInstance()
      ->addOption("page", po::value<uint32_t>(&pagelen)->default_value(0),
                  "Page len to recover data");

   chaos::service_common::ChaosServiceToolkit::getInstance()
      ->getGlobalConfigurationInstance()
      ->addOption(
          "dsname",
          po::value<std::string>(&name)->default_value("PERFORMANCE_MESURE"),
          "name of the dataset (CU)");
   chaos::service_common::ChaosServiceToolkit::getInstance()
      ->getGlobalConfigurationInstance()
      ->addOption("dsgroup",
                  po::value<std::string>(&group)->default_value("DATASETIO"),
                  "name of the group (US)");

   chaos::service_common::ChaosServiceToolkit::getInstance()
      ->getGlobalConfigurationInstance()
      ->addOption("loops", po::value<uint32_t>(&loops)->default_value(1000),
                  "number of push/loop");
   chaos::service_common::ChaosServiceToolkit::getInstance()
      ->getGlobalConfigurationInstance()
      ->addOption("waitloop", po::value<uint32_t>(&waitloops)->default_value(0),
                  "us waits bewteen loops");
   chaos::service_common::ChaosServiceToolkit::getInstance()
      ->getGlobalConfigurationInstance()
      ->addOption("wait", po::value<uint32_t>(&wait_retrive)->default_value(5),
                  "seconds to wait to retrive data after pushing");
   chaos::service_common::ChaosServiceToolkit::getInstance()
      ->getGlobalConfigurationInstance()
      ->addOption("points",
                  po::value<uint32_t>(&npoints)->default_value(npoints),
                  "Number of sin points, 0 = no wave");
   chaos::service_common::ChaosServiceToolkit::getInstance()
      ->getGlobalConfigurationInstance()
      ->addOption("freqshift",
                  po::value<double>(&freqshift)->default_value(0.001),
                  "Modify freq Hz every loop, 0= no modify");
   chaos::service_common::ChaosServiceToolkit::getInstance()
      ->getGlobalConfigurationInstance()
      ->addOption("ampshift",
                  po::value<double>(&ampshift)->default_value(0.9999),
                  "Modify amplitude every loop, 0= no modify");
   chaos::service_common::ChaosServiceToolkit::getInstance()
      ->getGlobalConfigurationInstance()
      ->addOption("binary", po::value<bool>(&binary)->default_value(false),
                  "The wave is in binary");
   chaos::service_common::ChaosServiceToolkit::getInstance()
      ->getGlobalConfigurationInstance()
      ->addOption(
          "report",
          po::value<std::string>(&reportName)->default_value(reportName),
          "The report file name");
   chaos::service_common::ChaosServiceToolkit::getInstance()
      ->getGlobalConfigurationInstance()
      ->addOption("pointincr",
                  po::value<uint32_t>(&pointincr)->default_value(pointincr),
                  "Increment points by 2^number, from points to pointmaz");
   chaos::service_common::ChaosServiceToolkit::getInstance()
      ->getGlobalConfigurationInstance()
      ->addOption("pointmax",
                  po::value<uint32_t>(&pointmax)->default_value(pointmax),
                  "Max point");

   chaos::service_common::ChaosServiceToolkit::getInstance()
      ->getGlobalConfigurationInstance()
      ->addOption("nthreads",
                  po::value<uint32_t>(&nthreads)->default_value(nthreads),
                  "Number of concurrent accesses");

  // chaos::service_common::ChaosServiceToolkit::getInstance()->start();
  chaos::service_common::ChaosServiceToolkit::getInstance()->init(argc, argv);
  if (pointmax == 0) {
    pointmax = npoints;
  }
  fs.open(reportName);

  boost::thread *workers[nthreads];
  params = new testparam_t[nthreads];
  fs << "points,payload size(Bytes),push/s,pull/s,loop,push time(us),pull "
        "time(us),bandwith(MB/s),overhead(us),errors,threads,werrors"
     << std::endl;
  thread_done = 0;
  for (int cnt = 0; cnt < nthreads; cnt++) {
    std::stringstream ss;
    ss << name << "_" << cnt;
    params[cnt].thid = cnt;
    workers[cnt] = new boost::thread(
        boost::bind(&performTest, ss.str(), boost::ref(params[cnt])));
  }
  LOG(" Waiting for "<<nthreads<<" to end");

  sleep(5);
  for (int cnt = 0; cnt < nthreads; cnt++) {
    workers[cnt]->join();
    delete (workers[cnt]);
  }
  delete[] params;
  LOG(" Stopping services");
  sleep(5);

   chaos::service_common::ChaosServiceToolkit::getInstance()->stop();
  //   LOG(" Deinit services");

   chaos::service_common::ChaosServiceToolkit::getInstance()->deinit();
  if(tot_error){
      LOG("## exiting with "<<exit_after_nerror<<",  tot errors:"<<tot_error);
  } else {
      LOG("* exiting with "<<tot_error<<" errors");

  }

  return tot_error;
}
