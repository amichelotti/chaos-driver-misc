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
  bool haskey=cd->hasKey(chaos::NodeDefinitionKey::NODE_UNIQUE_ID);

  if(cuname==""){
    if (haskey) {
      cuname = cd->getStringValue(chaos::NodeDefinitionKey::NODE_UNIQUE_ID);
    } else if (cd->hasKey("output") && cd->isCDataWrapperValue("output")) {
      CDWUniquePtr cdo = cd->getCSDataValue("output");
      if (cdo->hasKey(chaos::NodeDefinitionKey::NODE_UNIQUE_ID)) {
        cuname = cdo->getStringValue(chaos::NodeDefinitionKey::NODE_UNIQUE_ID);
      }

    } 
  } else {
    if(!haskey){
      cd->addStringValue(chaos::NodeDefinitionKey::NODE_UNIQUE_ID,cuname);
    }
  }
  return cd;
}

int RestCUContainer::addCU(const std::string &ds, const std::string &name) {
  ChaosWriteLock l(iomutex);
  std::string cuname=name;
  retriveDS(ds, cuname);
  if(cuname==""){
      DPD_LERR << "cannot find CU name";
      return -4;

  } 
  std::map<std::string, RestCU *>::iterator f = restCUs.find(cuname);
  if (f == restCUs.end()) {
    try {
    
       RestCU *ptr = new RestCU(cuname, ds);
      if (ptr) {
        DPD_LDBG << " Adding REST CU:" << cuname << " tot:" << restCUs.size();
        restCUs[cuname] = ptr;
        return 0;
      }
    } catch (chaos::CException &e) {
      DPD_LERR << " Chaos exception adding REST CU:" << cuname << ":" << e.what()
               << " tot:" << restCUs.size();

    } catch (...) {
      DPD_LERR << " Uknown exception adding REST CU:" << cuname
               << " tot:" << restCUs.size();
    }
  } else {
    DPD_LDBG << " Already registered" << cuname << " tot:" << restCUs.size();
    return 0;
  }

  return -1;
}

int RestCUContainer::removeCU(const std::string &name) {
  ChaosWriteLock l(iomutex);
  std::map<std::string, RestCU *>::iterator f = restCUs.find(name);
  if (f != restCUs.end()) {
    restCUs.erase(f);
    delete f->second;
  } else {
    DPD_LERR << "REST CU \"" << name << "\" not found";

    return -1;
  }
  return 0;
}

int RestCUContainer::push(const std::string &json, const std::string& cname, std::string &json_answer){
 std::string name=cname;

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

int RestCUContainer::push(const std::string &json, std::string &json_answer) {
 return push(json, "", json_answer);
}
} // namespace misc
} // namespace driver
