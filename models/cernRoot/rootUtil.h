/*
 * Andrea Michelotti
 * */
#ifndef __ROOT_UTIL__
#define __ROOT_UTIL__
#include "TTree.h"
namespace chaos{
	namespace common{
		namespace data{
		class CDataWrapper;
		}
	}
}



TTree* buildTree(const std::string& name,chaos::common::data::CDataWrapper*);
int addTree(TTree*,chaos::common::data::CDataWrapper*);
TTree* queryChaosTree(const std::string&chaosNode,const std::string& start,const std::string&end,const int channel,const std::string treeid,int pageLen=1000 );
#endif
