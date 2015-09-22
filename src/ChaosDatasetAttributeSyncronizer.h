/* 
 * File:   ChaosDatasetAttributeSyncronizerSyncronizer.h
 * Author: michelo
 *
 * Created on September 2, 2015, 5:29 PM
 */

#ifndef ChaosDatasetAttributeSyncronizerSyncronizer_H
#define	ChaosDatasetAttributeSyncronizerSyncronizer_H
#include <map>
#include <string>
#include "ChaosDatasetAttribute.h"

class ChaosDatasetAttributeSyncronizer{
    
   
    
public:

    ChaosDatasetAttributeSyncronizer();
    virtual ~ChaosDatasetAttributeSyncronizer();
    
   
    
    /**
     set the interval or error  between two good synchronized attributes 
     @param interval (seconds or value ) t
     */
    void setInterval(uint64_t interval);
    
    /**
     set the timeout for synchornizing items 
     @param timeo in us
     */
    void setTimeout(uint64_t timeo);
    
    /**
     add a new Attribute to the synchronized pool
     @param ChaosDatasetAttribute attribute
     */
    void add(ChaosDatasetAttribute&d);
    /**
     read and wait until all the pool is synchronized on a given value
     @return the tune in microseconds to synchronize , 0 otherwise
     */
    template<typename T>
    uint64_t sync(T value){
        int ok=0;
    uint64_t micro_spent;
     boost::posix_time::ptime start=boost::posix_time::microsec_clock::local_time();
      while((ok!=set.size()) && (((micro_spent=((boost::posix_time::microsec_clock::local_time()-start).total_microseconds()))<timeo))){
        ok=0;
        for(std::vector<ChaosDatasetAttribute*>::iterator i=set.begin();i!=set.end();i++){
            (*i)->setUpdateMode(ChaosDatasetAttribute::EVERYTIME,interval/2);
            (*i)->get(NULL);
            if((T)**i == value){
                ChaosDatasetAttribute::datinfo& info=(*i)->getInfo();
                ATTRDBG_<<"attribute \""<<(*i)->getPath()<<"\" time :"<<info.tstamp<<"reached value:"<<(T)value;
            }
                 
        }
      }
     if(ok==set.size()){
         return micro_spent;
     }
     return 0;
    }
    /**
     read and wait untill all the pool is synchronized in timestamp
     @return the max timestamp if success, 0 otherwise
     */
    uint64_t sync();

private:
    uint64_t interval;
    uint32_t timeo;
    std::vector<ChaosDatasetAttribute*> set;
    uint64_t base_time;
 
};

#endif	/* ChaosDatasetAttributeSyncronizerSyncronizer_H */

