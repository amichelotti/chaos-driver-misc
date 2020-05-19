#include "RestCUServer.h"

namespace driver{
    namespace misc{
#define DPD_LOG_HEAD "[RestCUServer] - "
#define DPD_LAPP LAPP_ << DPD_LOG_HEAD
#define DPD_LDBG LDBG_ << DPD_LOG_HEAD << __PRETTY_FUNCTION__
#define DPD_LERR LERR_ << DPD_LOG_HEAD << __PRETTY_FUNCTION__ << "(" << __LINE__ << ") "

RestCUServer::RestCUServer(int port,int nthread){
    std::stringstream ss;
    ss<<port;
    std::string sport;
    ss>>sport;
    server =new served::net::server("0.0.0.0",sport,mux); // all interfaces

   
}

RestCUServer::~RestCUServer(){
    delete server;
    
}
    
int RestCUServer::start(){
 mux.handle("api/v1/jsonregister")
		.post([](served::response & res, const served::request & req) {
            if(RestCUContainer::getInstance()->addCU(req.body())==0){
                res.set_status(served::status_2XX::OK);

            } else {
                res.set_status(served::status_4XX::EXPECTATION_FAILED);
                DPD_LERR<<" not a valid registration packet:"<<req.body();

            }
           
            res << "{}";


		});
    mux.handle("api/v1/jsoninsert")
		.post([](served::response & res, const served::request & req) {
            std::string resp;
            if(RestCUContainer::getInstance()->push(req.body(),resp)==0){
                res.set_status(served::status_2XX::OK);

            } else {
                res.set_status(served::status_4XX::EXPECTATION_FAILED);
                DPD_LERR<<" not a valid registration packet:"<<req.body();

            }
           
            res << "{}";

		});
	server->run(nthread);

    return 0;
}
int RestCUServer::stop(){
    server->stop();
    return 0;
}
    }}