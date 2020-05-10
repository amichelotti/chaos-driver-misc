#include <chaos/common/ChaosCommon.h>
#include <chaos/common/utility/StartableService.h>
#ifndef __CHAOSROOT_H__
#define __CHAOSROOT_H__
class TRint;
namespace driver {
namespace misc {
namespace root {

class ChaosRoot : public chaos::ChaosCommon<ChaosRoot> {
  friend class chaos::common::utility::Singleton<ChaosRoot>;
  //	common::utility::StartableServiceContainer<WANProxyRegister>
  // wan_register;

  ChaosRoot();
  ~ChaosRoot();

  TRint *rootApp;
  std::string rootopts;
//  const char** rootopts;
 // int rootargs;
 chaos::common::data::CDWUniquePtr updateConfiguration(chaos::common::data::CDWUniquePtr update_pack);

public:
  std::string uid;

  //! C and C++ attribute parser
  /*!
                     Specialized option for startup c and cpp program main
   options parameter
                     */
  void init(int argc, const char *argv[]) throw(chaos::CException);
  //! stringbuffer parser
  /*
                     specialized option for string stream buffer with boost
   semantics
                     */
  void setRootOpts(const std::string& opts);
  void init(std::istringstream &initStringStream) throw(chaos::CException);
  void init(void *init_data) throw(chaos::CException);
  void start() throw(chaos::CException);
  void stop() throw(chaos::CException);
  void deinit() throw(chaos::CException);
};
} // namespace cernroot
} // namespace misc
} // namespace driver

#endif