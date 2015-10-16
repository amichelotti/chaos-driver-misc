/* 
 * File:   ChaosDatasetAttributeGroupSyncronizer.h
 * Author: michelo
 *
 * Created on September 2, 2015, 5:29 PM
 */

#ifndef ChaosDatasetAttributeGroup_H
#define	ChaosDatasetAttributeGroup_H
#include <map>
#include <string>
#include "ChaosDatasetAttribute.h"

namespace driver{
    
    namespace misc{
class ChaosDatasetAttributeGroup{
    
     boost::mutex lock_sync;

    /**
     * map a full path to an attribute
     */
    std::map<std::string,ChaosDatasetAttribute*> id2attr;
    /**
     * map a name of an attribute to a vector of attributes
     */
    std::map<std::string,std::vector<ChaosDatasetAttribute*> > name2attrs;
    
public:

    ChaosDatasetAttributeGroup();
    virtual ~ChaosDatasetAttributeGroup();
    
   
    
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
     add a new Attribute to the  pool
     @param ChaosDatasetAttribute attribute
     @return the new created Attribute or NULL if error or already present
     */
     ChaosDatasetAttribute* add(std::string path);
    
    /**
     add a new Attribute to the  pool
     @param attr attribute
     */
    virtual void add(ChaosDatasetAttribute* attr);
    
    /**
     add a new Attribute to the  pool
     @param attr attribute
     */
    void add(ChaosDatasetAttribute& attr);
    /**
     remove a new Attribute to the  pool
     @param ChaosDatasetAttribute attribute
     */
    virtual void remove(std::string path);

    
    
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
    
protected:
    uint64_t interval;
    uint32_t timeo;
    
  
 
};

    }}
#endif	/*  */

