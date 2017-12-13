/*
 * Andrea Michelotti
 * */
#ifndef __ROOT_UTIL__
#define __ROOT_UTIL__
#include "TTree.h"
#include <driver/misc/core/ChaosDatasetAttribute.h>

using namespace ::driver::misc;
/**
 * Create a new Tree with default name of the chaosNode, it  creates a unique branch for a dataset
 * performs a time interval search of data
 * @param chaosNode[in] chaos node we want retrive historical data
 * @param start[in] epoch in ms of the start of the search, or time offset relative to now (i.e -10d5h4m22mm 10 days, 5 hours and 4 minutes 22 milliseconds before now), absolute time one of the following formats: %Y-%m-%d %H:%M:%S,%Y/%m/%d %H:%M:%S,%d.%m.%Y %H:%M:%S)
 * @parm end[in] epoch in ms of the end of the search
 * @parm channel[in] 0 output (i.e acquired device parameters),  1 input (i.e configuration parameters), -1 all
 * @parm treeid[in] alternative name for the tree
 * @parm desc[in] string description of the tree
 *
 * @parm pageLen[in] perform a paged query (0, means no paging), in case of paged query queryNextChaosTree must be used to retrieve successive elements.
 * @return the tree on success, 0 otherwise
 * */
TTree* queryChaosTreeSB(const std::string&chaosNode,const std::string& start,const std::string&end,const int channel,const std::string& treeid="",const std::string& desc="",int pageLen=0 );

/**
 * Create a new Tree with default name of the chaosNode, it  creates multiple branch
 * performs a time interval search of data
 * @param chaosNode[in] chaos node we want retrive historical data
 * @param start[in] epoch in ms of the start of the search, or time offset relative to now (i.e -10d5h4m22mm 10 days, 5 hours and 4 minutes 22 milliseconds before now), absolute time one of the following formats: %Y-%m-%d %H:%M:%S,%Y/%m/%d %H:%M:%S,%d.%m.%Y %H:%M:%S)
 * @parm end[in] epoch in ms of the end of the search
 * @parm channel[in] 0 output (i.e acquired device parameters),  1 input (i.e configuration parameters), -1 all
 * @parm treeid[in] alternative name for the tree
 * @parm desc[in] string description of the tree
 *
 * @parm pageLen[in] perform a paged query (0, means no paging), in case of paged query queryNextChaosTree must be used to retrieve successive elements.
 * @return the tree on success, 0 otherwise
 * */
TTree* queryChaosTree(const std::string&chaosNode,const std::string& start,const std::string&end,const int channel,const std::string& treeid="",const std::string& desc="",int pageLen=0 );

/**
 * Append to an existing Tree the data retrived
 * performs a time interval search of data
 * @param chaosNode[in] chaos node we want retrive historical data
 * @param start[in] epoch in ms of the start of the search, or time offset relative to now (i.e -10d5h4m22mm 10 days, 5 hours and 4 minutes 22 milliseconds before now), absolute time one of the following formats: %Y-%m-%d %H:%M:%S,%Y/%m/%d %H:%M:%S,%d.%m.%Y %H:%M:%S)
 * @parm end[in] epoch in ms of the end of the search
 * @parm channel[in] 0 output (i.e acquired device parameters),  1 input (i.e configuration parameters), -1 all
 * @parm branchid[in] alternative name for the branch (default is "chaosNode.channel" )
 * @parm pageLen[in] perform a paged query (0, means no paging), in case of paged query queryNextChaosTree must be used to retrieve successive elements.
 * @return the tree on success, 0 otherwise
 * */
TTree* queryChaosTree(TTree* tree,const std::string&chaosNode,const std::string& start,const std::string&end,const int channel,const std::string branchid="",int pageLen=0 );

/**
 * Retrive next pages of a queryChaosTree
 * @param tree[in] is the tree that has been created or appended of a queryChaosTree
 * @return false if nothing else
 * */
bool queryNextChaosTree(TTree*tree);
/**
 * Teels if something is still pending
 * @param tree[in] is the tree that has been created or appended of a queryChaosTree
 *
 * @return false if nothing else
 * */
bool queryHasNextChaosTree(TTree*tree);

/**
 * Free resources if a qqueryChaosTree
 * @param tree[in] is the tree that has been created or appended of a queryChaosTree
 * @return true on success
 */
bool queryFree(TTree*tree);

#endif
