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



TTree* buildTree(const std::string& name,const std::string& desc,const std::string &prefix,chaos::common::data::CDataWrapper*cd);
int addTree(TTree*tr,const std::string &prefix,chaos::common::data::CDataWrapper*cd);

TTree* queryChaosTree(const std::string&chaosNode,const std::string& start,const std::string&end,const int channel,const std::string treeid,int pageLen=0 );
/**
 * append to an existing TREE
 * */
TTree* queryChaosTree(TTree* tree,const std::string&chaosNode,const std::string& start,const std::string&end,const int channel,const std::string treeid,int pageLen=0 );

/**
 * Retrive next pages of a queryChaosTree
 * @return false if nothing else
 * */
bool queryNextChaosTree(TTree*tree);
/**
 * Teels if something is still pending
 * @return false if nothing else
 * */
bool queryHasNextChaosTree(TTree*tree);

#endif
