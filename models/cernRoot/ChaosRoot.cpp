/*
 * File:   testDataSetAttribute.cpp
 * Author: michelo
 * test from UI
 * Created on September 10, 2015, 11:24 AM
 */

#include <driver/misc/core/ChaosController.h>
#include <regex>
using namespace std;
#include "ChaosRoot.h"
#include "TROOT.h"
#include "TRint.h"
#include <chaos/common/healt_system/HealtManager.h>
#include <chaos/common/io/SharedManagedDirecIoDataDriver.h>
//#include <chaos/cu_toolkit/data_manager/DataManager.h>
#include <chaos/common/metadata_logging/metadata_logging.h>

#include <driver/misc/models/cernRoot/rootUtil.h>
#include <boost/process/environment.hpp>
#include <string>
/*
 *
 */
using namespace chaos;
using namespace driver::misc;
using namespace chaos::metadata_service_client;
using namespace chaos::common::io;
using namespace chaos::common::utility;
using namespace chaos::common::healt_system;
using namespace chaos::common::data;
using namespace chaos::cu::data_manager;

#define ROOTERR ERR_LOG(ChaosRoot)

#define ROOTDBG DBG_LOG(ChaosRoot)

namespace driver
{
  namespace misc
  {
    namespace root
    {

      ChaosRoot::ChaosRoot()
      {
        // int id=boost::this_process::get_id();
        // uid=std::to_string(id);
        // ROOTDBG<<" ChaosRoot Process ID:"<<uid;
        rootApp = NULL;
      }
      ChaosRoot::~ChaosRoot()
      {
        ROOTDBG << " ChaosRoot " << nodeuid << " remove";
        if (rootApp)
        {
          delete rootApp;
        }
      }
      void ChaosRoot::init(int argc, const char *argv[]) throw(chaos::CException)
      {
        chaos::ChaosCommon<ChaosRoot>::init(argc, argv);

        /*InizializableService::initImplementation(SharedManagedDirecIoDataDriver::getInstance(), NULL,"SharedManagedDirecIoDataDriver", __PRETTY_FUNCTION__);
  StartableService::initImplementation(HealtManager::getInstance(), NULL,
                                       "HealthManager", __PRETTY_FUNCTION__);
  */
      }
      void ChaosRoot::init(istringstream &initStringStream) throw(chaos::CException)
      {
        chaos::ChaosCommon<ChaosRoot>::init(initStringStream);
      }
      chaos::common::data::CDWUniquePtr ChaosRoot::_load(chaos::common::data::CDWUniquePtr dataset_attribute_values)
      {
        ROOTDBG << " CHAOSROOT LOAD:" << nodeuid << dataset_attribute_values->getJSONString();

        return chaos::common::data::CDWUniquePtr();
      }

      void ChaosRoot::start() throw(chaos::CException)
      {

        ChaosUniquePtr<chaos::common::data::CDataWrapper> result(
            new chaos::common::data::CDataWrapper());
        const char *root_opts[120];
        int nroot_opts = 1;
        int ret;
        // root_opts[nroot_opts++] = nodeuid.c_str();
        std::string buf;

        std::stringstream ss(rootopts);
        while (ss >> buf)
        {
          ROOTDBG << "options[" << root_opts << "]=" << buf;

          root_opts[nroot_opts++] = strdup(buf.c_str());
        }
        if (nodeuid.size() == 0)
        {
          // if nodeuid not given, use the name of function
          for (int cnt = 1; cnt < nroot_opts; cnt++)
          {
            if (*root_opts[cnt] != '-')
            {
              std::string path = root_opts[cnt];
              std::regex reg("([-\\.\\w]+)\\({0,1}[-,\\.\\w]*\\){0,1}$");
              std::smatch m;
              nodeuid = path;
              if (std::regex_search(path, m, reg))
              {
                nodeuid = m[1];
              }
            }
          }
        }
        root_opts[0] = nodeuid.c_str();

        ::driver::misc::ChaosDatasetIO::ownerApp = nodeuid;

        rootApp = new TRint("Rint", &nroot_opts, (char **)root_opts, 0, 0, kFALSE);

        // rootapp->init(NULL);
        // rootapp->start();
        //chaos::ChaosCommon<ChaosRoot>::start();

      //  StartableService::startImplementation(DataManager::getInstance(), "DataManager", "ChaosCUToolkit::start");
      StartableService::startImplementation(HealtManager::getInstance(), "HealtManager", __PRETTY_FUNCTION__);

        rootApp->SetPrompt("chaosRoot[%d]>");
        rootApp->Run();
      }
      void ChaosRoot::setRootOpts(const std::string &opts)
      {
        rootopts = opts;
      }

      void ChaosRoot::deinit() throw(chaos::CException)
      {
        CHAOS_NOT_THROW(
            StartableService::deinitImplementation(
                HealtManager::getInstance(), "HealtManager", __PRETTY_FUNCTION__););

        InizializableService::deinitImplementation(
            SharedManagedDirecIoDataDriver::getInstance(),
            "SharedManagedDirecIoDataDriver", __PRETTY_FUNCTION__);
            CHAOS_NOT_THROW(InizializableService::deinitImplementation(
                          chaos::common::metadata_logging::MetadataLoggingManager::getInstance(),
                          "MetadataLoggingManager",
                          __PRETTY_FUNCTION__););

        chaos::ChaosCommon<ChaosRoot>::deinit();
      }
      void ChaosRoot::stop() throw(chaos::CException)
      {
        CHAOS_NOT_THROW(StartableService::stopImplementation(
                            HealtManager::getInstance(), "HealthManager",
                            __PRETTY_FUNCTION__););
        chaos::ChaosCommon<ChaosRoot>::stop();
      }
      void ChaosRoot::init(void *init_data) throw(chaos::CException)
      {
        chaos::ChaosCommon<ChaosRoot>::init(init_data);
        InizializableService::initImplementation(SharedManagedDirecIoDataDriver::getInstance(), NULL, "SharedManagedDirecIoDataDriver", __PRETTY_FUNCTION__);
       // StartableService::initImplementation(DataManager::getInstance(), NULL, "DataManager", "ChaosRoot::init");
        StartableService::initImplementation(HealtManager::getInstance(), NULL, "HealtManager", __PRETTY_FUNCTION__);
        InizializableService::initImplementation(chaos::common::metadata_logging::MetadataLoggingManager::getInstance(),
                                                 NULL,
                                                 "MetadataLoggingManager",
                                                 __PRETTY_FUNCTION__);
      }
    } // namespace cernroot
  }   // namespace misc
} // namespace driver
