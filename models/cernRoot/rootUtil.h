/*
 * Andrea Michelotti
 * */
#ifndef __ROOT_UTIL__
#define __ROOT_UTIL__
#include "TTree.h"
#include <driver/misc/core/ChaosDatasetAttribute.h>
#include <driver/misc/core/ChaosDatasetIO.h>

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
 * Create a new Tree with default name of the chaosNode, it  creates multiple branch
 * performs a time interval search of data
 * @param chaosNode[in] chaos node we want retrive historical data
 * @param start[in] epoch in ms of the start of the search, or time offset relative to now (i.e -10d5h4m22mm 10 days, 5 hours and 4 minutes 22 milliseconds before now), absolute time one of the following formats: %Y-%m-%d %H:%M:%S,%Y/%m/%d %H:%M:%S,%d.%m.%Y %H:%M:%S)
 * @parm end[in] epoch in ms of the end of the search
 * @parm tags[in] a vector of tags to search
 * @parm channel[in] 0 output (i.e acquired device parameters),  1 input (i.e configuration parameters), -1 all
 * @parm treeid[in] alternative name for the tree
 * @parm desc[in] string description of the tree
 *
 * @parm pageLen[in] perform a paged query (0, means no paging), in case of paged query queryNextChaosTree must be used to retrieve successive elements.
 * @return the tree on success, 0 otherwise
 * */
TTree* queryChaosTree(const std::string&chaosNode,const std::string& start,const std::string&end,const std::vector<std::string>& tags,const int channel,const std::string& treeid="",const std::string& desc="",int pageLen=0 );

/**
 * Create a new Tree with default name of the chaosNode, it  creates multiple branch
 * performs a tags/ run query
 * @param chaosNode[in] chaos node we want retrive historical data
 * @parm tags[in] a vector of tags to search
 * @parm run[in] query
 * @parm channel[in] 0 output (i.e acquired device parameters),  1 input (i.e configuration parameters), -1 all
 * @parm treeid[in] alternative name for the tree
 * @parm desc[in] string description of the tree
 *
 * @parm pageLen[in] perform a paged query (0, means no paging), in case of paged query queryNextChaosTree must be used to retrieve successive elements.
 * @return the tree on success, 0 otherwise
 * */
TTree* queryChaosTree(const std::string&chaosNode,const std::vector<std::string>& tags,const uint64_t runid, const int channel,const std::string& treeid="",const std::string& desc="",int pageLen=0 );

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

void treeToCDataWrapper(chaos::common::data::CDataWrapper& dst,const TTree* val);

void treeToCDataWrapper(chaos::common::data::CDataWrapper& dst,const std::string& key,const TTree* val);
/**
                 export current CDataWrapper to Tree with the given name
                 \param name name of the tree
                 \param branch_name branch name
                 \param multiple creates a branch for each key, otherwise creates just on branch with all keys
                 \return NULL if error, an allocated and initialized Tree otherwise
                 */
TTree*getTreeFromCDataWrapper(const chaos::common::data::CDataWrapper& src,const std::string& name,const std::string& branch_name);
void initChaosRoot();

struct branchAlloc;


struct chaosBranch{
    std::string name;
    std::string cdkey;
    std::string rootType;
    bool is_vector;
    chaos::DataType::DataType chaosType;
    int data_element_size;
    chaos::DataType::DataType chaosSubType;
    Int_t vector_size;
    void*ptr;
    int size;
    TTree*parent;
    chaosBranch(TTree* par,const std::string&key,const chaos::common::data::CDataWrapper& cd,const std::string&brsuffix="");
    int realloc(int newsize);
    ~chaosBranch();
    bool add(const chaos::common::data::CDataWrapper& cd);
};


class ChaosToTree{
    TTree*root;
    const std::string brsuffix;
    const std::string tname;

    typedef std::map<std::string,ChaosSharedPtr<chaosBranch> > branch_map_t;
    branch_map_t branches;

    public:
        ChaosToTree(TTree*root,const std::string&brsuffix="");

        ChaosToTree(const::std::string& treename,const std::string&brsuffix="");
        ~ChaosToTree();

        int addData(const chaos::common::data::CDataWrapper&);
        TTree*getTree();
};

#endif
