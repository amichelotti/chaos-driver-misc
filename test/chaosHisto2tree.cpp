/*
 * File:   testDataSetAttribute.cpp
 * Author: michelo
 * test from UI
 * Created on September 10, 2015, 11:24 AM
 */

#include <chaos_metadata_service_client/ChaosMetadataServiceClient.h>
#include <cstdlib>
#include <driver/misc/models/cernRoot/rootUtil.h>
using namespace std;
using namespace ::driver::misc;
using namespace chaos::metadata_service_client;
#include "TFile.h"
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

using namespace driver::misc;

using namespace chaos::metadata_service_client::node_controller;
using namespace chaos::common::data;
int main(int argc, const char **argv) {
  int64_t tot_ele = 0, errors = 0, warning = 0;

  std::string qstart, qend;
  uint32_t pagelen = 1000;
  uint32_t maxele = 0;
  uint64_t now = chaos::common::utility::TimingUtil::getTimeStamp();
  bool checkseq = true;
  bool checkrunid = true;
  bool dontout = false;
  uint64_t start_ts = now - (1000 * 3600), end_ts = now;
  std::string nodeid;

  ChaosStringVector meta_tags, projection_key_vec;
  //"%d-%m-%Y %H:%M:%S"
  qend = chaos::common::utility::TimingUtil::toString(end_ts, "%d%m%Y%H%M%S");
  qstart =
      chaos::common::utility::TimingUtil::toString(start_ts, "%d%m%Y%H%M%S");
  ChaosMetadataServiceClient::getInstance()
      ->getGlobalConfigurationInstance()
      ->addOption("start",
                  po::value<std::string>(&qstart)->default_value(qstart),
                  "start of the wuery format :%d%m%Y%H%M%S");

  ChaosMetadataServiceClient::getInstance()
      ->getGlobalConfigurationInstance()
      ->addOption("end", po::value<std::string>(&qend)->default_value(qend),
                  "end of the wuery format :%d%m%Y%H%M%S");

  ChaosMetadataServiceClient::getInstance()
      ->getGlobalConfigurationInstance()
      ->addOption("page", po::value<uint32_t>(&pagelen)->default_value(pagelen),
                  "Page len to recover data");

  ChaosMetadataServiceClient::getInstance()
      ->getGlobalConfigurationInstance()
      ->addOption("max", po::value<uint32_t>(&maxele)->default_value(maxele),
                  "Max number of elements (0 = all)");

  ChaosMetadataServiceClient::getInstance()
      ->getGlobalConfigurationInstance()
      ->addOption("checkseq",
                  po::value<bool>(&checkseq)->default_value(checkseq),
                  "check consecutive sequence id");

  ChaosMetadataServiceClient::getInstance()
      ->getGlobalConfigurationInstance()
      ->addOption("checkrunid",
                  po::value<bool>(&checkrunid)->default_value(checkrunid),
                  "check for change runid");

  ChaosMetadataServiceClient::getInstance()
      ->getGlobalConfigurationInstance()
      ->addOption(
          "notoutput", po::value<bool>(&dontout)->default_value(dontout),
          "dont write down data, perform only request and check (if enabled)");

  ChaosMetadataServiceClient::getInstance()
      ->getGlobalConfigurationInstance()
      ->addOption("nodeid", po::value<std::string>(&nodeid)->default_value(""),
                  "chaos node id");

  ChaosMetadataServiceClient::getInstance()
      ->getGlobalConfigurationInstance()
      ->addOption<ChaosStringVector>("tags", "Meta tags list", &meta_tags);
  ChaosMetadataServiceClient::getInstance()
      ->getGlobalConfigurationInstance()
      ->addOption<ChaosStringVector>("keys", "retrieve just these keys",
                                     &projection_key_vec);

  ChaosMetadataServiceClient::getInstance()->init(argc, argv);
  ChaosMetadataServiceClient::getInstance()->start();

  start_ts = chaos::common::utility::TimingUtil::getTimestampFromString(qstart);
  end_ts = chaos::common::utility::TimingUtil::getTimestampFromString(qend);
  CUController *controller = NULL;
  if (nodeid == "") {
    std::cerr << " must specify a nodeid" << std::endl;
    return -1;
  }
  try {
    ChaosMetadataServiceClient::getInstance()->getNewCUController(nodeid,
                                                                  &controller);

    if (!controller) {
      std::cerr << " cannot allocate controller for:" << nodeid;
      return -1;
    }
    ChaosStringSet search_tags;
    chaos::common::io::QueryCursor *query_cursor = NULL;
    for (ChaosStringVectorIterator it = meta_tags.begin(),
                                   end = meta_tags.end();
         it != end; it++) {
      search_tags.insert(*it);
    }
    ChaosStringSet projection_keys;
    for (ChaosStringVectorIterator it = projection_key_vec.begin(),
                                   end = projection_key_vec.end();
         it != end; it++) {
      projection_keys.insert(*it);
    }
    controller->executeTimeIntervallQuery(
        chaos::cu::data_manager::KeyDataStorageDomainOutput, start_ts, end_ts,
        search_tags, projection_keys, &query_cursor, pagelen);

    std::cout << nodeid << " Query fronm: "
              << chaos::common::utility::TimingUtil::toString(start_ts)
              << " from:"
              << chaos::common::utility::TimingUtil::toString(end_ts)
              << " page:" << pagelen << std::endl;

    std::string name = std::string(argv[0]) + ".root";
    TFile *fout;
    ChaosToTree ti(name);

    fout = new TFile(name.c_str(), "RECREATE");

    int64_t last_rid = 0;
    int64_t last_sid = 0;
    uint64_t bytes = 0;
    uint64_t tot_ms = 0;
    if (query_cursor) {
      std::cout << "Exporting... " << std::endl;
      while (query_cursor->hasNext()) {
        uint64_t st = chaos::common::utility::TimingUtil::getTimeStamp();
        ChaosSharedPtr<CDataWrapper> q_result(query_cursor->next());
        if (q_result.get()) {
          if (q_result->hasKey(
                  chaos::ControlUnitNodeDefinitionKey::CONTROL_UNIT_RUN_ID) &&
              q_result->hasKey(chaos::DataPackCommonKey::DPCK_SEQ_ID)) {
            int64_t rid = q_result->getInt64Value(
                chaos::ControlUnitNodeDefinitionKey::CONTROL_UNIT_RUN_ID);
            int64_t sid =
                q_result->getInt64Value(chaos::DataPackCommonKey::DPCK_SEQ_ID);

            if (checkrunid) {
              if ((last_rid != 0) && (last_rid > rid)) {
                std::cout << "[" << tot_ele
                          << "] ## run id inversion:" << last_rid
                          << " got:" << rid << std::endl;
                errors++;
              } else if ((last_rid != rid)) {
                std::cout << "[" << tot_ele << "] %% run id changed" << last_rid
                          << " to:" << rid << std::endl;
                last_sid = 0;
                warning++;
              }
            }

            if (checkseq) {
              if ((last_sid != 0) && (last_rid == rid) &&
                  (last_sid + 1) != sid) {
                std::cout << "[" << tot_ele
                          << "] ## bad sequence expected:" << (last_sid + 1)
                          << " got:" << sid
                          << " missing:" << (sid - last_sid - 1) << std::endl;
                errors++;
              }
            }
            last_sid = sid;
            last_rid = rid;
            tot_ele++;
            bytes += q_result->getBSONRawSize();
            tot_ms += chaos::common::utility::TimingUtil::getTimeStamp() - st;
            if (!dontout) {
              ti.addData(*q_result.get());
            }
          }

          // write the data
        }
      }
      if (!dontout) {
        fout->Write();
      }
      double mb = (double)bytes / 1024 * 1024.0;
      std::cout << " retrived " << tot_ele << " in " << tot_ms << " MB:" << mb
                << " bandwith MB/s:" << mb * 1000.0 / tot_ms
                << " errors:" << errors << " warning:" << warning << std::endl;
      controller->releaseQuery(query_cursor);
    }

    std::cout << "Releasing controller" << std::endl;
    ChaosMetadataServiceClient::getInstance()->deleteCUController(controller);
    return errors;
  } catch (chaos::CException &e) {
    std::cout << "\x1B[?25h";
    std::cerr << e.errorCode << " - " << e.errorDomain << " - "
              << e.errorMessage << std::endl;
    return -1;
  } catch (...) {
    std::cout << "\x1B[?25h";
    std::cerr << "General error " << std::endl;
    return -2;
  }

  ChaosMetadataServiceClient::getInstance()->stop();
  //    ChaosMetadataServiceClient::getInstance()->deinit();
  return errors;
}
