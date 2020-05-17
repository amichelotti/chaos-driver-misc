#include <driver/misc/core/ChaosDatasetIO.h>
#include <chaos/common/utility/Singleton.h>

#ifndef __RESTCU_CONTAINER__
#define __RESTCU_CONTAINER__
namespace driver{
    namespace misc{

class RestCUContainer:public common::utility::Singleton<RestCUContainer>{
    


std::map<std::string,ChaosDatasetIO*> restCUs;

chaos::CObjectProcessingQueue<std::string> 
public:
/**
 * @brief Add a new CU if not exits alread
 * @param name is the name of the CU
 * @param ds is a json dataset
 * @return 0 if success
 */
int addCU(const std::string&name,const std::string& ds);

/**
 * @brief Remove a CU
 * @param name name of the CU to remove
 * @return 0 if success
 */
int removeCU(const std::string&name);

/**
 * @brief Remove a CU
 * @param name name of the CU to remove
 * @return 0 if success
 */
int push(const std::string&jsonDataset);
};
    }
    }
#endif