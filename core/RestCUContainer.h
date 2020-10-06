#include <chaos/common/chaos_types.h>
#include <chaos/common/utility/Singleton.h>
#include <map>
#include "RestCU.h"
#ifndef __RESTCU_CONTAINER__
#define __RESTCU_CONTAINER__
namespace driver {
namespace misc {
class RestCUContainer : public chaos::common::utility::Singleton<RestCUContainer> {

  std::map<std::string, RestCU *> restCUs;
  ChaosSharedMutex iomutex;
  ChaosDataSet retriveDS(const std::string&ds, std::string& cuname);

public:
  /**
   * @brief Add a new CU if not exits alread
   * @param ds is a json dataset
   * @param name if not specified look into the DS name.
   * @return 0 if success
   */
  int addCU(const std::string &ds,const std::string &name="" );



  /**
   * @brief Remove a CU
   * @param name name of the CU to remove
   * @return 0 if success
   */
  int removeCU(const std::string &name);

  /**
   * @brief push
   * @param jsonDataset dataset to push
   * @param json_answer returns a json answer to send back
   * @return 0 if success
   */
  int push(const std::string &jsonDataset, std::string &json_answer);
    /**
   * @brief push
   * @param name node name
   * @param jsonDataset dataset to push
   * @param json_answer returns a json answer to send back
   * @return 0 if success
   */
  int push(const std::string &jsonDataset, const std::string& name, std::string &json_answer);

};
} // namespace misc
} // namespace driver
#endif