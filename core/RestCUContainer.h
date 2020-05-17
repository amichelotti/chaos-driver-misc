#include <chaos/common/chaos_types.h>
#include <chaos/common/utility/Singleton.h>
#include <map>
#ifndef __RESTCU_CONTAINER__
#define __RESTCU_CONTAINER__
namespace driver {
namespace misc {
class RestCU;
class RestCUContainer : public chaos::common::utility::Singleton<RestCUContainer> {

  std::map<std::string, RestCU *> restCUs;
  ChaosSharedMutex iomutex;

public:
  /**
   * @brief Add a new CU if not exits alread
   * @param name is the name of the CU
   * @param ds is a json dataset
   * @return 0 if success
   */
  int addCU(const std::string &name, const std::string &ds);

  /**
   * @brief Remove a CU
   * @param name name of the CU to remove
   * @return 0 if success
   */
  int removeCU(const std::string &name);

  /**
   * @brief Remove a CU
   * @param jsonDataset dataset to push
   * @param json_answer returns a json answer to send back
   * @return 0 if success
   */
  int push(const std::string &name,const std::string &jsonDataset, std::string &json_answer);
};
} // namespace misc
} // namespace driver
#endif