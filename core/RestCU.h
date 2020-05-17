#include "ChaosDatasetIO.h"
#include <chaos/common/pqueue/CObjectProcessingQueue.h>
#include <boost/lockfree/queue.hpp>

#ifndef __RESTCU__
#define __RESTCU__

namespace driver{
    namespace misc{
class RestCU:public ChaosDatasetIO,public chaos::CObjectProcessingQueue<chaos::common::data::CDataWrapper>{


    void processBufferElement(QueueElementShrdPtr element);
public:
    boost::lockfree::queue<std::string*> events;

    RestCU(const std::string& cuname,const std::string& ds,const std::string& grupname="");
    ~RestCU();
    int pushJsonDataset(const std::string&json);

};
    }}
#endif