#include <chaos/common/ChaosCommon.h>
#include <chaos/common/utility/StartableService.h>
#ifndef __CHAOSROOT_H__
#define __CHAOSROOT_H__
class TRint;
namespace driver {
namespace misc {
namespace root {

class ChaosRoot :public chaos::ChaosCommon<ChaosRoot>{
  
  TRint *rootApp;
//  const char** rootopts;
 // int rootargs;


  chaos::common::data::CDWUniquePtr _load(chaos::common::data::CDWUniquePtr dataset_attribute_values);

public:
  std::string rootopts;

  ChaosRoot();
  ~ChaosRoot();


  //! C and C++ attribute parser
  /*!
                     Specialized option for startup c and cpp program main
   options parameter
                     */
  void init(int argc, const char *argv[]) ;
  //! stringbuffer parser
  /*
                     specialized option for string stream buffer with boost
   semantics
                     */
  void setRootOpts(const std::string& opts);
  void init(std::istringstream &initStringStream) ;
  void init(void *init_data) ;
  void start() ;
  void stop() ;
  void deinit() ;

};
} // namespace cernroot
} // namespace misc
} // namespace driver

#endif