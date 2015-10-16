/* 
 * File:   remoteGroupAccessDriver.h
 * Author: michelo
 *
 * Created on October 14, 2015, 10:00 AM
 */

#ifndef REMOTEGROUPACCESSDRIVER_H
#define	REMOTEGROUPACCESSDRIVER_H
#include <chaos/cu_toolkit/driver_manager/driver/BasicIODriver.h>

#include "ChaosControllerGroup.h"
#include "ChaosDatasetAttributeGroup.h"

DEFINE_CU_DRIVER_DEFINITION_PROTOTYPE(remoteGroupAccessDriver);
namespace driver{
    
    namespace misc{
class remoteGroupAccessDriver: public chaos::cu::driver_manager::driver::BasicIODriver {
public:
    remoteGroupAccessDriver();
    virtual ~remoteGroupAccessDriver();
    
    
                    
     int read(void *buffer, int addr, int bcount);
     int initIO(void *buffer, int sizeb);
     int deinitIO();           
     int write(void *buffer, int addr, int bcount){return 0;};      
     int iop(int operation,void*data,int sizeb){return 0;}

private:
    ChaosControllerGroup<ChaosController>* group;
    ChaosDatasetAttributeGroup* data_group;
};
    }}
#endif	/* REMOTEGROUPACCESSDRIVER_H */

