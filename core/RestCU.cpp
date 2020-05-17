#include "RestCU.h"
#define MAX_EVENTS 128
using namespace chaos::common::data;
namespace driver {
namespace misc {


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
    pushDataset(element);
  } catch (...) {
  }
}

int RestCU::pushJsonDataset(const std::string &json) {
     try {
    ChaosDataSet ds(new chaos::common::data::CDataWrapper());
    ds->setSerializedJsonData(json.c_str());
    chaos::CObjectProcessingQueue<chaos::common::data::CDataWrapper>::QueueElementShrdPtr a(ds);
    push(a);
    return 0;
     } catch (...) {
  }
 
  return -102;
}
} // namespace misc
} // namespace driver