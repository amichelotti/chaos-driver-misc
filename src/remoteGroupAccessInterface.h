/* 
 * File:   remoteGroupAccess.h
 * Author: michelo
 *
 * Created on October 14, 2015, 9:40 AM
 */

#ifndef REMOTEGROUPACCESS_H
#define	REMOTEGROUPACCESS_H
#include "ChaosControllerGroup.h"
#include "ChaosDatasetAttributeGroup.h"
#include <chaos/cu_toolkit/driver_manager/driver/BasicIODriverInterface.h>
namespace driver{
    namespace misc{

class remoteGroupAccessInterface : public chaos::cu::driver_manager::driver::BasicIODriverInterface {

    ChaosControllerGroup<ChaosController>* ctrl_group;
    ChaosDatasetAttributeGroup* data_group;
    
    public:
    remoteGroupAccessInterface(chaos::cu::driver_manager::driver::DriverAccessor*);
    virtual ~remoteGroupAccessInterface();
    /**
     * Connect the driver with the remote resources
     * @return 0 on success
     */
    int connect();
    /**
     * Initialize the controlled CU
     * @param force force the status 
     * @return 0 on success
     */
    int init(int force);
    /**
     * Stop the controlled CUs
     * @param force force the status to stop
     * @return 0 on success
     */
    int stop(int force);
    /**
     * Start the controlled CUs
     * @param force the status to start
     * @return 0 on success
     */
    int start(int force);
    /**
     * Deinitialize the controlled CUs
     * @param force the status to deinit
     * @return 0 on success
     */
    int deinit(int force);
    /**
     * Set the schedule of the acquire to the given value
     * @param us microseconds 
     * @return 0 on success
     */
    int setSchedule(uint64_t us);
    /**
     * Set the timeout for the answer of a CU
     * @param timeo_us timeout expresset in microseconds
     */
    void setTimeout(uint64_t timeo_us);
    /**
     * Send the command to the controlled CUs
     * @param alias
     * @param params
     * @return 0 on succes
     */
    int broadcastCmd(std::string alias,chaos::common::data::CDataWrapper*params);
   
    
    std::vector<ChaosDatasetAttribute*> getRemoteVariables();
      /**
     * Return a list of variables with the given name
     * @param name name of the attribute
     * @return a list of attributes with the corresponding name
     */
    std::vector<ChaosDatasetAttribute*> getRemoteVariables(std::string name);
    
    /**
     * Return the remote variable  with the given path
     * @param path full path that identifies the attribute
     * @return the attribute or null if not found
     */
    ChaosDatasetAttribute* getRemoteVariable(std::string path);
private:

};
    }}
#endif	/* REMOTEGROUPACCESS_H */

