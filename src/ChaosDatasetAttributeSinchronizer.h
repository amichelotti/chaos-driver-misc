/* 
 * File:   ChaosDatasetAttributeSinchronizerSyncronizer.h
 * Author: michelo
 *
 * Created on September 2, 2015, 5:29 PM
 */

#ifndef ChaosDatasetAttributeSinchronizerSyncronizer_H
#define	ChaosDatasetAttributeSinchronizerSyncronizer_H
#include <map>
#include <string>
#include "ChaosDatasetAttributeGroup.h"
namespace driver{
    namespace misc{

class ChaosDatasetAttributeSinchronizer: public ChaosDatasetAttributeGroup {
    
    struct syncInfo{
        uint64_t tlastread;
        uint64_t tchanged;
        unsigned changes;
        syncInfo();
               
    };
public:
    typedef std::pair<ChaosDatasetAttribute*,syncInfo> attr_t;
private:
    typedef std::vector<attr_t > cuset_t;
    

public:

    ChaosDatasetAttributeSinchronizer();
    virtual ~ChaosDatasetAttributeSinchronizer();
    
   

    /**
     add a new Attribute to the synchronized pool
     @param attr attribute
     */
    virtual void add(ChaosDatasetAttribute* attr);
    
    /**
     remove a new Attribute to the synchronized pool
     @param ChaosDatasetAttribute attribute
     */
    virtual void remove(std::string path);
    

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
            ChaosDatasetAttribute::datinfo& info=i->first->getInfo();

            if((T)*i->first == value){
                ATTRDBG_<<"attribute \""<<i->first->getPath()<<"\" time :"<<info.tstamp<<"reached value:"<<(T)value <<" "<<ok<<"/"<<set.size();
                ok++;
            } else {
                ATTRDBG_<<"attribute \""<<i->first->getPath()<<"\" time :"<<info.tstamp<<"NOT in sync value:"<<(T)*i->first <<" "<<ok<<"/"<<set.size();
            }
                 
        }
      }
     ATTRDBG_<<"micro spent waiting: "<<micro_spent<<" timeo:"<<timeo;
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
    
    

    
private:
    
    

    int sortedFetch(uint64_t& max_age);
    cuset_t set;
    friend bool compareSet(ChaosDatasetAttributeSinchronizer::attr_t const&,ChaosDatasetAttributeSinchronizer::attr_t const&);
    uint64_t base_time;
 
};


    }}
bool compareSet(::driver::misc::ChaosDatasetAttributeSinchronizer::attr_t const &i,::driver::misc::ChaosDatasetAttributeSinchronizer::attr_t const&j);
#endif	/* ChaosDatasetAttributeSinchronizerSyncronizer_H */

