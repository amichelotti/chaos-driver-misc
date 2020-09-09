#include "RestCUServer.h"

namespace driver{
    namespace misc{
#define DPD_LOG_HEAD "[RestCUServer] - "
#define DPD_LAPP LAPP_ << DPD_LOG_HEAD
#define DPD_LDBG LDBG_ << DPD_LOG_HEAD << __FUNCTION__
#define DPD_LERR LERR_ << DPD_LOG_HEAD << __PRETTY_FUNCTION__ << "(" << __LINE__ << ") "


#define REGISTERPATH "api/v1/producer/jsonregister"
#define PUSHPATH "api/v1/producer/jsoninsert"
#define UNREGISTERPATH "api/v1/producer/jsonremove"

RestCUServer::RestCUServer(uint32_t port,uint32_t _nthread):nthread(_nthread){
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
 mux.handle(REGISTERPATH)
		.post([](served::response & res, const served::request & req) {
       //     req.url().path().find("api/v1/producer/jsonregister/")
            DPD_LDBG<<"PATH:\""<<req.url().path()<<"\" query:"<<req.url().query()<<" req:"<<req.body();
            
            std::string uid;
            if(req.url().path().size()>sizeof(REGISTERPATH)){
                uid=req.url().path().substr(sizeof(REGISTERPATH)+2);
                DPD_LDBG<<"UID:\""<<uid;
            }
            if((RestCUContainer::getInstance()->addCU(req.body(),uid)==0)){
                res.set_status(served::status_2XX::OK);
                res << "{\"error\":0}";

            } else {
                res.set_status(served::status_4XX::EXPECTATION_FAILED);
                DPD_LERR<<" not a valid registration packet:"<<req.body();
                res << "{\"error\":1,\"msg\":\"not a valid registration packet\""+req.body()+"\"}";

            }
           


		});
    mux.handle(PUSHPATH)
		.post([](served::response & res, const served::request & req) {
            std::string resp;
          //  DPD_LDBG<<"Push:"<<req.body();
          std::string uid;
          if(req.url().path().size()>sizeof(REGISTERPATH)){
                uid=req.url().path().substr(sizeof(REGISTERPATH)+2);
                DPD_LDBG<<"UID:\""<<uid;
            }

            if(RestCUContainer::getInstance()->push(req.body(),uid,resp)==0){
                res.set_status(served::status_2XX::OK);
                res << "{\"error\":0}";

            } else {
                res.set_status(served::status_4XX::EXPECTATION_FAILED);
                DPD_LERR<<" not a valid push packet:"<<req.body();
                res << "{\"error\":1,\"msg\":\"not a valid push packet,i.e 'ndk_uid' key is mandatory\""+req.body()+"\"}";

            }
           

		});
      mux.handle(UNREGISTERPATH)
		.post([](served::response & res, const served::request & req) {
            std::string resp;
            DPD_LDBG<<"Unregister:"<<req.body();
            std::string uid=req.url().path().substr(sizeof(UNREGISTERPATH)+2);

            if(RestCUContainer::getInstance()->removeCU(uid)==0){
                res.set_status(served::status_2XX::OK);
                res << "{\"error\":0}";

            } else {
                res.set_status(served::status_4XX::EXPECTATION_FAILED);
                DPD_LERR<<" error ungegistering"<<req.body();
                res << "{\"error\":1,\"msg\":\"error unregistering\""+req.body()+"\"}";

            }
           

		});
    
    DPD_LDBG<<"Starting server with:"<<nthread<<" threads";
	server->run(nthread);

    return 0;
}
int RestCUServer::stop(){
    server->stop();
    return 0;
}
    }}