#include "RestCU.h"
#define MAX_EVENTS 128
using namespace chaos::common::data;
namespace driver {
namespace misc {
#define DPD_LOG_HEAD "[RestCU] - "

#define DPD_LAPP LAPP_ << DPD_LOG_HEAD
#define DPD_LDBG LDBG_ << DPD_LOG_HEAD << __PRETTY_FUNCTION__
#define DPD_LERR                                                               \
  LERR_ << DPD_LOG_HEAD << __PRETTY_FUNCTION__ << "(" << __LINE__ << ") "


CDWUniquePtr eventHandler(CDWUniquePtr ev,ChaosDatasetIO* p){
    RestCU* ptr=(RestCU*)p;
    std::string* sptr=new std::string();
    *sptr=ev->getJSONString();
    ptr->events.push(sptr);
    return CDWUniquePtr();
}
RestCU::~RestCU() {
    events.consume_all([this](std::string* i) {
    delete i;
  });
  chaos::CObjectProcessingQueue<chaos::common::data::CDataWrapper>::deinit();
    ChaosDatasetIO::deinit();
}

RestCU::RestCU(const std::string &cuname, const std::string &ds,
               const std::string &grupname)
    : ChaosDatasetIO(cuname, grupname),events(MAX_EVENTS) {
  chaos::common::data::CDataWrapper cw;
  cw.setSerializedJsonData(ds.c_str());

  if(cw.hasKey("input")&&cw.isCDataWrapperValue("input")){
        ::driver::misc::ChaosDataSet in=allocateDataset(chaos::DataPackCommonKey::DPCK_DATASET_TYPE_INPUT);
        CDWUniquePtr ptr=cw.getCSDataValue("input");
        ptr->copyAllTo(*in.get());
         
  }
  if(cw.hasKey("output")&&cw.isCDataWrapperValue("output")){
        ::driver::misc::ChaosDataSet out=allocateDataset(chaos::DataPackCommonKey::DPCK_DATASET_TYPE_OUTPUT);
        CDWUniquePtr ptr=cw.getCSDataValue("output");
        ptr->copyAllTo(*out.get());
        
  } else {
      ::driver::misc::ChaosDataSet out=allocateDataset(chaos::DataPackCommonKey::DPCK_DATASET_TYPE_OUTPUT);

      cw.copyAllTo(*out.get());
  }
  if(registerDataset()!=0){
      throw chaos::CException(-1, "Cannot register dataset of " + cuname,
                              __PRETTY_FUNCTION__);
  }

  init(1);// use one thread

}

void RestCU::processBufferElement(QueueElementShrdPtr element) {
  try {
    ChaosDatasetIO::pushDataset(element);
  } catch(chaos::CException&e){
    DPD_LERR<<" Chaos Exception pushing dataset:"<<element->getJSONString()<<" error:"<<e.what();

  }
  catch (...) {
      DPD_LERR<<" Uknown processing";
  }
}

int RestCU::pushDataset(const ChaosDataSet&ds,std::string&ans){
    chaos::CObjectProcessingQueue<chaos::common::data::CDataWrapper>::QueueElementShrdPtr a(ds);
    push(a);
    std::string *ev;
    if(events.pop(ev)){
        ans=*ev;
        delete ev;
    } else {
        ans="{}";
    }
    return 0;

}

int RestCU::pushJsonDataset(const std::string &json,std::string&ans) {
     try {
    ChaosDataSet ds(new chaos::common::data::CDataWrapper());
    ds->setSerializedJsonData(json.c_str());
    chaos::CObjectProcessingQueue<chaos::common::data::CDataWrapper>::QueueElementShrdPtr a(ds);
    push(a);
    std::string *ev;
    if(events.pop(ev)){
        ans=*ev;
        delete ev;
    } else {
        ans="{}";
    }
    return 0;
     } catch(chaos::CException&e){
    DPD_LERR<<" Chaos Exception pushing JSON in queue:"<<e.what();

  }
  catch (...) {
      DPD_LERR<<" Uknown pushing JSON";
  }
  return -102;
}
} // namespace misc
} // namespace driver