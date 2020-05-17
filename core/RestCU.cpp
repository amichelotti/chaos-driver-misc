#include "RestCU.h"
using namespace chaos::common::data;
namespace driver {
namespace misc {

RestCU::~RestCU() {}

RestCU::RestCU(const std::string &cuname, const std::string &ds,
               const std::string &grupname)
    : ChaosDatasetIO(cuname, grupname) {
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