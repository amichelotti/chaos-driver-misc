/*
 * File:   testDataSetAttribute.cpp
 * Author: michelo
 * test from UI
 * Created on September 10, 2015, 11:24 AM
 */
#include <boost/filesystem.hpp>
#include <chaos/common/configuration/GlobalConfiguration.h>
#include <chaos/common/message/MessagePSDriver.h>
#include <cstdlib>
#include <driver/misc/models/cernRoot/rootUtil.h>
#include <fstream>
#include <iostream>

using namespace std;
using namespace ::driver::misc;
using namespace chaos::metadata_service_client;
#ifdef CERN_ROOT

#include "TFile.h"
#endif
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
  int32_t pagelen = -1;
  uint32_t maxele = 0;
  uint64_t now = chaos::common::utility::TimingUtil::getTimeStamp();
  bool checkseq = true;
  bool checkrunid = true;
  bool dontout = false;
  bool bsonen = false;
  bool jsonen = false;
  bool pngen = false;
  std::string server = "localhost:9092";

  std::string groupid = "chaosKafkaExport";
  uint64_t start_ts = now - (1000 * 3600), end_ts = now;
  std::string nodeid;
  uint64_t lost_pckt = 0;
  std::string off_type = "earliest";

  ChaosStringVector meta_tags, projection_key_vec;
  //"%d-%m-%Y %H:%M:%S"
  qend =
      chaos::common::utility::TimingUtil::toString(end_ts, "%d-%m-%Y %H:%M:%S");
  qstart = chaos::common::utility::TimingUtil::toString(start_ts,
                                                        "%d-%m-%Y %H:%M:%S");

  /*std::cout << nodeid << " Query fronm: "
                << qstart
                << "("<<start_ts<<") from:"
                << qend
                << "("<<end_ts<<") page:" << pagelen << std::endl;
  */
  /*chaos::GlobalConfiguration::getInstance()

      ->addOption("start",
                  po::value<std::string>(&qstart)->default_value(qstart),
                  "start of the wuery format :%d-%m-%Y %H:%M:%S");

  chaos::GlobalConfiguration::getInstance()

      ->addOption("end", po::value<std::string>(&qend)->default_value(qend),
                  "end of the wuery format :%d-%m-%Y %H:%M:%S");
*/

  chaos::GlobalConfiguration::getInstance()->addOption(
      "server", po::value<std::string>(&server)->default_value(server),
      "A kafka server");
  chaos::GlobalConfiguration::getInstance()->addOption(
      "page", po::value<int32_t>(&pagelen)->default_value(pagelen),
      "Page len to recover data (number of data), -1 retrive continuosly)");

  chaos::GlobalConfiguration::getInstance()->addOption(
      "checkseq", po::value<bool>(&checkseq)->default_value(checkseq),
      "check consecutive sequence id");

  chaos::GlobalConfiguration::getInstance()->addOption(
      "checkrunid", po::value<bool>(&checkrunid)->default_value(checkrunid),
      "check for change runid");
#ifdef CERN_ROOT

  chaos::GlobalConfiguration::getInstance()->addOption(
      "root", po::value<bool>(&dontout)->default_value(dontout),
      "Write root file");
#endif
  chaos::GlobalConfiguration::getInstance()->addOption(
      "json", po::value<bool>(&dontout)->default_value(jsonen),
      "Write json file");
  chaos::GlobalConfiguration::getInstance()->addOption(
      "bson", po::value<bool>(&dontout)->default_value(bsonen),
      "Write bson file");
  chaos::GlobalConfiguration::getInstance()->addOption(
      "image", po::value<bool>(&dontout)->default_value(pngen),
      "Write image files if FRAMEBUFFER present");
  chaos::GlobalConfiguration::getInstance()->addOption(
      "nodeid", po::value<std::string>(&nodeid)->default_value(""),
      "chaos node id");

  /*chaos::GlobalConfiguration::getInstance()

      ->addOption<ChaosStringVector>("tags", "Meta tags list", &meta_tags);*/

  chaos::GlobalConfiguration::getInstance()->addOption(
      "groupid", po::value<std::string>(&groupid)->default_value(groupid),
      "make part of this group");

  chaos::GlobalConfiguration::getInstance()->addOption(
      "offtype", po::value<std::string>(&off_type)->default_value(off_type),
      "Offset Strategy 'earliest' or 'latest'");
  chaos::GlobalConfiguration::getInstance()->preParseStartupParameters();

  chaos::GlobalConfiguration::getInstance()->loadStartupParameterFromEnv();
  chaos::GlobalConfiguration::getInstance()->loadStartupParameter(argc, argv);
  chaos::GlobalConfiguration::getInstance()->scanOption();

  //  chaos::GlobalConfiguration::getInstance()->start();

  // start_ts =
  // chaos::common::utility::TimingUtil::getTimestampFromString(qstart,"%d-%m-%Y
  // %H:%M:%S",false); end_ts =
  // chaos::common::utility::TimingUtil::getTimestampFromString(qend,"%d-%m-%Y
  // %H:%M:%S",false);

  if (nodeid == "") {
    std::cerr << " must specify a nodeid" << std::endl;
    return -1;
  }
  try {

    chaos::common::message::consumer_uptr_t cons =
        chaos::common::message::MessagePSDriver::getConsumerDriver("kafka-rdk",
                                                                   groupid);

    /* chaos::GlobalConfiguration::getInstance()->getNewCUController(nodeid,
                                                                   &controller);

     if (!controller) {
       std::cerr << " cannot allocate controller for:" << nodeid;
       return -1;
     }*/
    ChaosStringSet search_tags;
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

    std::string ds = nodeid + chaos::DataPackPrefixID::OUTPUT_DATASET_POSTFIX;

    cons->addServer(server);

    if (cons->setOption("auto.offset.reset", off_type) != 0) {
      std::cerr << "## cannot apply configuration configuration:"
                << cons->getLastError() << std::endl;
      return -1;
    }
    if (cons->applyConfiguration() == 0) {
      std::cout << "* consumer configuration ok" << std::endl;
      if (cons->subscribe(nodeid) != 0) {
        std::cerr << "## error consumer subscribe:" << cons->getLastError()
                  << std::endl;
        return -7;
      }
    } else {
      std::cerr << " cannot apply configuration:" << cons->getLastError()
                << std::endl;
      return -10;
    }
    cons->start();
    std::string pname = "chaosKafkaExport";

    std::string rootname = pname + ".root";
    std::ofstream jsonfile;
    if (jsonen) {
      jsonfile.open(pname + ".json");
      jsonfile << "[";
    }

#ifdef CERN_ROOT

    TFile *fout;
    ChaosToTree *ti;
    if (!dontout) {
      std::cout << "* tree file out:" << rootname << std::endl;
      fout = new TFile(rootname.c_str(), "RECREATE");
      ti = new ChaosToTree(rootname);
    }
#endif
    int64_t last_rid = 0;
    int64_t last_sid = 0;
    uint64_t bytes = 0;
    uint64_t tot_us = 0;
    int64_t first_sid = -1;
    int64_t first_rid = -1;

    int tot_ele = 0;
    int errors = 0, warning = 0;
    std::cout << "Exporting... " << std::endl;

    while ((pagelen < 0) || (tot_ele < pagelen)) {
      uint64_t st =
          chaos::common::utility::TimingUtil::getTimeStampInMicroseconds();

      chaos::common::message::ele_uptr_t p = cons->getMsg();
      if (p.get()) {
        chaos::common::data::CDWShrdPtr q_result = p->cd;
        if (q_result.get()) {
          if (q_result->hasKey(
                  chaos::ControlUnitNodeDefinitionKey::CONTROL_UNIT_RUN_ID) &&
              q_result->hasKey(chaos::DataPackCommonKey::DPCK_SEQ_ID)) {
            int64_t rid = q_result->getInt64Value(
                chaos::ControlUnitNodeDefinitionKey::CONTROL_UNIT_RUN_ID);
            int64_t sid =
                q_result->getInt64Value(chaos::DataPackCommonKey::DPCK_SEQ_ID);
            int64_t ts = q_result->getInt64Value(
                chaos::DataPackCommonKey::DPCK_TIMESTAMP);
            if (first_rid < 0) {
              first_rid = rid;
            }
            if (first_sid < 0) {
              first_sid = sid;
            }
            if (checkrunid) {
              if ((last_rid != 0) && (last_rid > rid)) {

                std::cout << "[" << tot_ele
                          << "] ## run id inversion:" << last_rid
                          << " got:" << rid << " TS:" << ts << "["
                          << chaos::common::utility::TimingUtil::toString(ts)
                          << "]" << std::endl;
                errors++;
              } else if ((last_rid != 0) && (last_rid != rid)) {

                std::cout << "[" << tot_ele << "] %% run id changed "
                          << last_rid << " to:" << rid << " TS:" << ts << "["
                          << chaos::common::utility::TimingUtil::toString(ts)
                          << "]" << std::endl;
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
                          << " missing:" << (sid - last_sid - 1) << " packets"
                          << " TS:" << ts << "["
                          << chaos::common::utility::TimingUtil::toString(ts)
                          << "]" << std::endl;
                lost_pckt += (sid - last_sid - 1);
                errors++;
              }
            }
            last_sid = sid;
            last_rid = rid;
            tot_ele++;
            bytes += q_result->getBSONRawSize();
            tot_us += (chaos::common::utility::TimingUtil::
                           getTimeStampInMicroseconds() -
                       st);
            if ((tot_ele % (((pagelen > 0) ? pagelen / 10 : 100))) == 0) {
              double mb = (double)bytes / (1024 * 1024.0);

              std::cout << " retrieved " << tot_ele << " in "
                        << (double)tot_us * 1.0 / 1000000.0 << "s , MB:" << mb
                        << "MB bandwith MB/s:"
                        << ((tot_us) ? (mb * 1000000.0 / tot_us) : 0)
                        << " errors:" << errors << ", warning:" << warning
                        << ", lost percent:" << lost_pckt * 100.0 / tot_ele
                        << std::endl;
            }
            if (pngen) {
              if (q_result->hasKey("FRAMEBUFFER") &&
                  q_result->isBinaryValue("FRAMEBUFFER")) {
                std::string name = q_result->getStringValue("ndk_uid");
                std::string ext = q_result->getStringValue("FMT");
                std::string tag;

                if(q_result->hasKey("dsndk_tag")){
                  tag=q_result->getStringValue("dsndk_tag");
                }
                boost::filesystem::path p(name);
                if ((boost::filesystem::exists(p) == false)) {
                  try {
                    if ((boost::filesystem::create_directories(p) == false) &&
                        ((boost::filesystem::exists(p) == false))) {
                      std::cerr << "cannot create directory:" << p
                                << " runid:" << rid << " seq:" << sid
                                << std::endl;
                    }
                  } catch (boost::filesystem::filesystem_error &e) {
                    std::cerr << " Exception creating directory:" << e.what()
                              << std::endl;
                  }
                }
                std::stringstream fname;
                if(tag.size()>0){
                  fname << name << "/" << tag<<"/"<<rid << "_" << sid << "." << ext;

                } else {
                  fname << name << "/" << rid << "_" << sid << "." << ext;
                }
                fstream f(fname.str());
                uint32_t siz = 0;
                const char *ptr = q_result->getBinaryValue("FRAMEBUFFER", siz);
                f.write(ptr, siz);
                f.close();
              }
            }
            if (jsonen) {
              jsonfile << q_result->getCompliantJSONString();
              if ((pagelen < 0) || (tot_ele + 1 < pagelen)) {
                jsonfile << ",";
              }
            }
#ifdef CERN_ROOT
            if (!dontout) {
              ti->addData(*q_result.get());
            }
#endif
          } else {
            std::cout << "[" << tot_ele
                      << "] ## invalid packet, missing indexing key"
                      << std::endl;
            errors++;
          }

          // write the data
        } else {
          std::cout << "[" << tot_ele << "] ## empty packet" << std::endl;
          errors++;
        }
      }
    } // while
#ifdef CERN_ROOT

    if (!dontout) {
      fout->Write();
      fout->Close();
      delete fout;
      delete ti;
    }
#endif
    if (jsonen) {
      jsonfile << "]";
      jsonfile.close();
    }

    double mb = (double)bytes / (1024 * 1024.0);
    if (tot_ele > 0) {
      std::cout << " sid:[" << first_sid << "," << last_sid << "] rid:["
                << first_rid << "," << last_rid << "]"
                << " retrieved " << tot_ele << " in "
                << (double)tot_us * 1.0 / 1000000.0 << "s , MB:" << mb
                << "MB bandwith MB/s:"
                << ((tot_us) ? (mb * 1000000.0 / tot_us) : 0)
                << " errors:" << errors << ", warning:" << warning
                << ", lost percent:" << lost_pckt * 100.0 / tot_ele
                << std::endl;
    } else {
      std::cout << " NOTHING FOUND" << std::endl;
    }

    sleep(1); // give time to commit offsets;
    cons->stop();

    // chaos::GlobalConfiguration::getInstance()->stop();
    // chaos::GlobalConfiguration::getInstance()->deinit();

    //  std::cout << "Releasing controller" << std::endl;
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

  // chaos::GlobalConfiguration::getInstance()->stop();
  //    chaos::GlobalConfiguration::getInstance()->deinit();
  return errors;
}
