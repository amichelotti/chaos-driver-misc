#ifndef __RESTCU_SERVER__
#define __RESTCU_SERVER__
#include <served/served.hpp>
#include "RestCUContainer.h"
namespace driver{
    namespace misc{
class RestCUServer{

    //RestCUContainer* container;
    served::multiplexer mux;
    served::net::server* server;
    int nthread,port;
    public:
    RestCUServer(int port,int nthread=4);

    ~RestCUServer();
    
    int start();
    int stop();

};
}
}
#endif
