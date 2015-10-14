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
    
    struct syncInfo{
        uint64_t tlastread;
        uint64_t tchanged;
        unsigned changes;
        syncInfo();
               
    };
    boost::mutex lock_sync;
    typedef std::pair<ChaosDatasetAttribute*,syncInfo> attr_t;
    typedef std::vector<attr_t > cuset_t;
    /**
     * map a full path to an attribute
     */
    std::map<std::string,ChaosDatasetAttribute*> id2attr;
    /**
     * map a name of an attribute to a vector of attributes
     */
    std::map<std::string,std::vector<ChaosDatasetAttribute*> > name2attrs;
    
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
     @return the new created Attribute or NULL if error or already present
     */
    ChaosDatasetAttribute* add(std::string path);
    
    /**
     add a new Attribute to the synchronized pool
     @param attr attribute
     */
    void add(ChaosDatasetAttribute* attr);
    
    /**
     add a new Attribute to the synchronized pool
     @param attr attribute
     */
    void add(ChaosDatasetAttribute& attr);
    /**
     remove a new Attribute to the synchronized pool
     @param ChaosDatasetAttribute attribute
     */
    void remove(std::string path);

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
        for(cuset_t::iterator i=set.begin();i!=set.end();i++){
            i->first->setUpdateMode(ChaosDatasetAttribute::EVERYTIME,interval/2);
            i->first->get(NULL);
            if((T)*i->first == value){
                ChaosDatasetAttribute::datinfo& info=i->first->getInfo();
                ATTRDBG_<<"attribute \""<<i->first->getPath()<<"\" time :"<<info.tstamp<<"reached value:"<<(T)value;
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
     @return the max age if success,  otherwise
     */
    int64_t sync();
    
    
    /**
     * Return a list of attributes with the given name
     * @param name name of the attribute
     * @return a list of attributes with the corresponding name
     */
    std::vector<ChaosDatasetAttribute*> getAttrsByName(std::string name);
    
    /**
     * Return the attribute  with the given name
     * @param path full path that identifies the attribute
     * @return the attribute or null if not found
     */
    ChaosDatasetAttribute* getAttr(std::string path);
    /**
     * Return all the attributes
     * @return a vector of attributes
     */
    std::vector<ChaosDatasetAttribute*> getAttributes();
    
private:
    uint64_t interval;
    uint32_t timeo;
    
    

    int sortedFetch(uint64_t& max_age);
    cuset_t set;
    friend bool compareSet(attr_t const&,attr_t const&);
    uint64_t base_time;
 
};

bool compareSet(ChaosDatasetAttributeSyncronizer::attr_t const &i,ChaosDatasetAttributeSyncronizer::attr_t const&j);

#endif	/* ChaosDatasetAttributeSyncronizerSyncronizer_H */

