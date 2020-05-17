#include "RestCUContainer.h"
#include "RestCU.h"
namespace driver {
namespace misc {
#define DPD_LOG_HEAD  "[RestCUContainer] - "
#define DPD_LAPP LAPP_ << DPD_LOG_HEAD
#define DPD_LDBG LDBG_ << DPD_LOG_HEAD << __PRETTY_FUNCTION__
#define DPD_LERR                                                               \
  LERR_ << DPD_LOG_HEAD << __PRETTY_FUNCTION__ << "(" << __LINE__ << ") "

int RestCUContainer::addCU(const std::string &name, const std::string &ds) {
  ChaosWriteLock l(iomutex);

  std::map<std::string, RestCU *>::iterator f = restCUs.find(name);
  if (f != restCUs.end()) {
    try {
      RestCU *ptr = new RestCU(name, ds);
      if (ptr) {
        DPD_LDBG << " Adding REST CU:" << name << " tot:" << restCUs.size();
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

int RestCUContainer::push(const std::string &name,const std::string &jsonDataset,
                          std::string &json_answer) {
  std::map<std::string, RestCU *>::iterator f = restCUs.find(name);
  if (f != restCUs.end()) {
    return f->second->pushJsonDataset(jsonDataset, json_answer);
  }
  DPD_LERR << "REST CU \"" << name << "\" not found";

  return -1;
}
} // namespace misc
} // namespace driver
