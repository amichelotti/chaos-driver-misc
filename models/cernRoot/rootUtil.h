/*
 * Andrea Michelotti
 * */
#ifdef __ROOT_UTIL__
#define __ROOT_UTIL__
#include "TTree.h"

TTree* queryChaosTree(const std::string&chaosNode,const std::string& start,const std::string&end,const std::string treeid,int pageLen=1000 );
#endif
