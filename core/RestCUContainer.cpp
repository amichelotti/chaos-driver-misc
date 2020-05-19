#include "RestCUContainer.h"
#include "RestCU.h"
#include <chaos/common/data/CDataWrapper.h>
namespace driver {
namespace misc {
#define DPD_LOG_HEAD "[RestCUContainer] - "
#define DPD_LAPP LAPP_ << DPD_LOG_HEAD
#define DPD_LDBG LDBG_ << DPD_LOG_HEAD << __PRETTY_FUNCTION__
#define DPD_LERR                                                               \
  LERR_ << DPD_LOG_HEAD << __PRETTY_FUNCTION__ << "(" << __LINE__ << ") "

using namespace chaos::common::data;

ChaosDataSet RestCUContainer::retriveDS(const std::string &json,
                                        std::string &cuname) {
  ChaosDataSet cd(new chaos::common::data::CDataWrapper());
  cd->setSerializedJsonData(json.c_str());
  if (cd->hasKey(chaos::NodeDefinitionKey::NODE_UNIQUE_ID)) {
    cuname = cd->getStringValue(chaos::NodeDefinitionKey::NODE_UNIQUE_ID);
  } else if (cd->hasKey("output") && cd->isCDataWrapperValue("output")) {
    CDWUniquePtr cdo = cd->getCSDataValue("output");
    if (cdo->hasKey(chaos::NodeDefinitionKey::NODE_UNIQUE_ID)) {
      cuname = cdo->getStringValue(chaos::NodeDefinitionKey::NODE_UNIQUE_ID);
    }

  } else {
    cuname = "";
  }
  return cd;
}

int RestCUContainer::addCU(const std::string &ds, const std::string &name) {
  ChaosWriteLock l(iomutex);

  std::map<std::string, RestCU *>::iterator f = restCUs.find(name);
  if (f != restCUs.end()) {
    try {
      std::string cuname = name;
      if (name == "") {
        retriveDS(ds, cuname);
      }
      if (cuname == "") {
        DPD_LERR << " Not a valid CU name found in ds";
        return -1;
      }
      RestCU *ptr = new RestCU(cuname, ds);
      if (ptr) {
        DPD_LDBG << " Adding REST CU:" << cuname << " tot:" << restCUs.size();
        restCUs[name] = ptr;
        return 0;
      }
    } catch (chaos::CException &e) {
      DPD_LERR << " Chaos exception adding REST CU:" << name << ":" << e.what()
               << " tot:" << restCUs.size();

    } catch (...) {
      DPD_LERR << " Uknown exception adding REST CU:" << name
               << " tot:" << restCUs.size();
    }
  }

  return -1;
}

int RestCUContainer::removeCU(const std::string &name) {
  ChaosWriteLock l(iomutex);
  std::map<std::string, RestCU *>::iterator f = restCUs.find(name);
  if (f != restCUs.end()) {
    restCUs.erase(f);
  } else {
    DPD_LERR << "REST CU \"" << name << "\" not found";

    return -1;
  }
  return 0;
}

int RestCUContainer::push(const std::string &json, std::string &json_answer) {
  std::string name;

  try {
    ChaosDataSet ds = retriveDS(json, name);
    std::map<std::string, RestCU *>::iterator f = restCUs.find(name);
    if (f != restCUs.end()) {
      return f->second->pushDataset(ds, json_answer);
    }
  DPD_LERR << "REST CU \"" << name << "\" not found";
  return -1;

  } catch (chaos::CException &e) {
    DPD_LERR << " Chaos Exception:" << e.what();
  }

    return -10;

}
} // namespace misc
} // namespace driver
