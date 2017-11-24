/* 
 * File:   testDataSetAttribute.cpp
 * Author: michelo
 * test from UI
 * Created on September 10, 2015, 11:24 AM
 */

#include <driver/misc/core/ChaosController.h>
#include "rootUtil.h"
#include <stdlib.h>
using namespace std;
using namespace chaos::metadata_service_client;
#include "TROOT.h"
#include "TTree.h"

using namespace chaos::common::data;
using namespace driver::misc;
typedef struct branchAlloc{
    char* branchBuffer;
    int32_t size;
    std::string branchContent;
    std::string brname;
    branchAlloc(){size=0;branchBuffer=NULL;}
    ~branchAlloc(){if((size>0)&&(branchBuffer)) {        LDBG_<<" removing breanch:'"<<brname<<"' size:"<<size<<" content type:"<<branchContent;free(branchBuffer);}}
} branchAlloc_t;

typedef struct treeQuery{
    uint32_t page_uid;
    int32_t channel;
    uint32_t page;
    bool treeAllocated;
    TTree* tree;
    branchAlloc_t* branch;
    int nbranch;
    ChaosController*ctrl;
    treeQuery(){ctrl=NULL;branch=NULL;treeAllocated=false;tree=NULL;nbranch=0;}
} treeQuery_t;

static std::map<TTree*, treeQuery_t> queries;
//static std::map<std::string, ChaosController*> ctrls;
//create a unique branch with all dataset

static branchAlloc_t* createBranch(TTree* tr,treeQuery& q,chaos::common::data::CDataWrapper*cd,const std::string& brname,bool multiple=true) {
    std::stringstream varname;
    std::vector<std::string> contained_key;
    int disable_dump=0;
    cd->getAllKey(contained_key);
    branchAlloc_t*query=NULL;
    int branch_counter=0;
    std::string branch_prefix;
    if(contained_key.size()==0){
        return NULL;
    }
    if(multiple){
        query = new branchAlloc_t[contained_key.size()];
        q.nbranch=contained_key.size();

    } else {
        query = new branchAlloc_t[1];
        q.nbranch=1;
        branch_prefix="";//brname+std::string("__");
    }
    LDBG_<<" creating "<<q.nbranch<<" branches";

    q.branch=query;
    if(query[0].size>0){
        free(query[0].branchBuffer);
        query[0].branchBuffer=0;
    }
    query[0].size=0;

    for (std::vector<std::string>::iterator it = contained_key.begin();
         it != contained_key.end(); it++,branch_counter++) {
        disable_dump=0;
        int found=0;
        if(multiple==false){
            branch_counter=0;
        } else {
            varname.str(branch_prefix);
            if(query[branch_counter].size>0){
                free(query[branch_counter].branchBuffer);
                query[branch_counter].branchBuffer=0;
            }
            query[branch_counter].size=0;
        }
        int type_size = CDataWrapperTypeDouble;
        if (cd->isVector(*it)) {
            int size = 0;
            CMultiTypeDataArrayWrapper* da = cd->getVectorValue(*it);
            //if(!multiple)
                varname <<*it;

            varname << "[" << da->size() << "]";
            found++;
            if (da->size()) {
                if (da->isDoubleElementAtIndex(0)) {
                    type_size = CDataWrapperTypeDouble;
                    query[branch_counter].size+=da->size()*sizeof(double);

                } else if (da->isInt32ElementAtIndex(0)) {
                    type_size = CDataWrapperTypeInt32;
                    query[branch_counter].size+=da->size()*sizeof(int32_t);

                } else if (da->isInt64ElementAtIndex(0)) {
                    type_size = CDataWrapperTypeInt64;
                    query[branch_counter].size+=da->size()*sizeof(int64_t);

                } else if (da->isStringElementAtIndex(0)) {
                    type_size = CDataWrapperTypeString;
                    query[branch_counter].size+=da->size()*(da->getStringElementAtIndex(0).size());

                }
            }
            //	LDBG_<<" BELE "<<varname<<" tot size:"<<query[branch_counter].size;

        } else {
            if((type_size==CDataWrapperTypeDouble )||(type_size==CDataWrapperTypeInt32)||(type_size==CDataWrapperTypeInt64)||(type_size==CDataWrapperTypeBool)){

                type_size = cd->getValueType(*it);
                query[branch_counter].size+=cd->getValueSize(*it);

                /*if((type_size==CDataWrapperTypeString)){
                    int ret;
                    if(!(ret=(cd->getValueSize(*it)%4))){
                        query[branch_counter].size+=ret;
                    }
                }*/
                LDBG_<<" BELE "<<*it<< " ele size:"<<cd->getValueSize(*it)<<" tot size:"<<query[branch_counter].size;
            }
        }
        switch (type_size) {
        case CDataWrapperTypeNoType:
            break;
        case CDataWrapperTypeNULL:
            break;
        case CDataWrapperTypeBool:
            found++;
         //   if(!multiple)
                varname <<*it;
            varname << "/O";

            break;
        case CDataWrapperTypeInt32:
            found++;
       //     if(!multiple)
                varname <<*it;
            varname << "/I";

            break;
        case CDataWrapperTypeInt64:
            found++;
           // if(!multiple)
                varname <<*it;
            varname << "/L";

            break;
        case CDataWrapperTypeDouble:
            found++;
            //if(!multiple)
                varname <<*it;
            varname << "/D";

            break;
        case CDataWrapperTypeString:
            disable_dump=1;
     //               found++;
     //               varname << *it;
     //           varname << "/C";

            break;
        case CDataWrapperTypeBinary:
            break;
        case CDataWrapperTypeObject:
            break;
        case CDataWrapperTypeVector:
            break;
        default:
            break;
        }
        if((multiple==false) && (found &&( (it+1)!= contained_key.end()))){
            varname<<":";
        }
        if(multiple && (disable_dump == 0)){
            std::stringstream ss;
            ss<<brname<<"."<<(*it);
            query[branch_counter].branchContent=varname.str();
            query[branch_counter].branchBuffer=(char*)malloc(query[branch_counter].size);
            query[branch_counter].brname=ss.str();
            tr->Branch(query[branch_counter].brname.c_str(), (void*)query[branch_counter].branchBuffer,varname.str().c_str());
            LDBG_<<"create ROOT BRANCH \""<<query[branch_counter].brname<<"\""<< " content:\""<<varname.str()<<"\" size:"<<query[branch_counter].size<<" address 0x"<<std::hex<<(uint64_t)query[branch_counter].branchBuffer<<std::dec;
        }
    }
    if(multiple==false){
        query[0].brname=brname;
        query[0].branchContent=varname.str();
        query[0].branchBuffer=(char*)malloc(query[0].size);
        tr->Branch(brname.c_str(), (void*)query[0].branchBuffer,varname.str().c_str());
        LDBG_<<"create ROOT BRANCH \""<<query[0].brname<<"\""<< "content:\""<<varname.str()<<"\" size:"<<query[0].size<<" address 0x"<<std::hex<<(uint64_t)query[0].branchBuffer<<std::dec;

    }

    return query;
}

static TTree* buildTree(const std::string& name, const std::string& desc) {
    TTree* tr = new TTree(name.c_str(), desc.c_str());
    LDBG_<<"create ROOT TREE \""<<name<<"\""<< " desc:\""<<desc<<"\"";

    if (tr == NULL) {
        LERR_<< "[ "<<__PRETTY_FUNCTION__<<"]" << " cannot create tree  \""<<name<<"\"";

        return NULL;
    }

    /*for (std::vector<std::string>::iterator it = contained_key.begin();
            it != contained_key.end(); it++) {

        createBranch(tr, prefix, *it, cd);
    }*/
    return tr;
}

static int addTree(treeQuery_t& q, chaos::common::data::CDataWrapper*cd) {
    std::vector < std::string > contained_key;
    branchAlloc_t* query=q.branch;
    int branch_counter=0;
    int type_size=0;
    cd->getAllKey(contained_key);
    int ptr=0;
    for (std::vector<std::string>::iterator it = contained_key.begin();
         it != contained_key.end(); it++) {
        if (cd->isVector(*it)) {
            int size = 0;
            CMultiTypeDataArrayWrapper* da = cd->getVectorValue(*it);

            for(int cnt=0;cnt<da->size();cnt++){
                if (da->isDoubleElementAtIndex(cnt)) {
                    double tmp=da->getDoubleElementAtIndex(cnt);
                    memcpy(query[branch_counter].branchBuffer+ptr,static_cast<void*>(&tmp),sizeof(tmp));
                    ptr+=sizeof(tmp);
                } else if (da->isInt32ElementAtIndex(cnt)) {
                    int32_t tmp=da->getInt32ElementAtIndex(cnt);
                    memcpy(query[branch_counter].branchBuffer+ptr,static_cast<void*>(&tmp),sizeof(tmp));
                    ptr+=sizeof(tmp);
                } else if (da->isInt64ElementAtIndex(cnt)) {
                    int64_t tmp=da->getInt64ElementAtIndex(cnt);
                    memcpy(query[branch_counter].branchBuffer+ptr,static_cast<void*>(&tmp),sizeof(tmp));
                    ptr+=sizeof(tmp);

                } else if (da->isStringElementAtIndex(cnt)) {
                    std::string tmp=da->getStringElementAtIndex(cnt);
                    memcpy(query[branch_counter].branchBuffer+ptr,(void*)(tmp.c_str()),tmp.size());
                    ptr+=tmp.size();

                }
            }
        } else {
            //LDBG_<<"ELE "<<*it<<" size:"<<cd->getValueSize(*it);
            switch(cd->getValueType(*it)){
            case CDataWrapperTypeDouble:
            case CDataWrapperTypeInt64:
            case CDataWrapperTypeBool:
            case CDataWrapperTypeInt32:{
                memcpy(query[branch_counter].branchBuffer+ptr,cd->getRawValuePtr(*it),cd->getValueSize(*it));
                ptr+=cd->getValueSize(*it);
                break;
            }
            }

        }
        if((branch_counter+1)<q.nbranch){
            // if multiple branch buffer is different, offset to zero
            ptr=0;
            branch_counter++;
        }
    }
    q.tree->Fill();
    return 0;
}

static TTree* query_int(TTree* tree_ret, const std::string&chaosNode,
                        const std::string& start, const std::string&end, const int channel,
                        const std::string treeid, const std::string desc,int pageLen,bool multi) {
    std::string brname;
    try {
        treeQuery_t q;
        branchAlloc_t* branch=NULL;
        q.treeAllocated=false;

        ChaosController* ctrl = NULL;
        std::vector<boost::shared_ptr<chaos::common::data::CDataWrapper> > res;
   /*     if (ctrls.find(chaosNode) != ctrls.end()) {
            ctrl = ctrls[chaosNode];
        } else {
            ctrl = new ChaosController(chaosNode);
            ctrls[chaosNode] = ctrl;
        }
*/
        ctrl = new ChaosController(chaosNode);

        int32_t ret = ctrl->queryHistory(start, end, channel, res, pageLen);
        int cnt = 0;
        if (res.size() > 0) {
            LDBG_<<"Query result size with pagelen "<<pageLen<<" is "<<res.size()<<" elements";

            if (tree_ret == NULL) {
                tree_ret = buildTree((treeid=="")?chaosNode:treeid, desc);
                brname=(treeid=="")?chaos::datasetTypeToHuman(channel):treeid;
                q.treeAllocated=true;
            } else {
                brname=(treeid=="")?(chaosNode+std::string(chaos::datasetTypeToHuman(channel))):treeid;

            }
            q.tree=tree_ret;

            std::vector < std::string > contained_key;
            boost::shared_ptr<chaos::common::data::CDataWrapper> cd = res[0];
            branch=createBranch(tree_ret,q,cd.get(),chaos::datasetTypeToHuman(channel),multi);

        } else {
            LAPP_<< "CHAOS no entries found from \""<<start<<"\" to \""<<end<<" on \""<<chaosNode<<"\"";
            return NULL;
        }

        if (tree_ret == NULL) {
            LERR_<< "[ "<<__PRETTY_FUNCTION__<<"]" << " cannot create tree on \""<<chaosNode;
            return tree_ret;
        }
        if(branch==NULL){
            delete tree_ret;
            return NULL;
        }
        q.page_uid = ret;
        q.page = pageLen;
        q.ctrl = ctrl;
        q.channel = channel;
        queries[tree_ret] = q;
        for (std::vector<boost::shared_ptr<chaos::common::data::CDataWrapper> >::iterator i =
             res.begin(); i != res.end(); i++) {
            addTree(q, i->get());
        }

        if(pageLen==0){
            queryFree(tree_ret);
        }


        return tree_ret;
    } catch (chaos::CException e) {
        LERR_<< "[ "<<__PRETTY_FUNCTION__<<"]" << "Exception on \""<<chaosNode<<"\""<< " errn:"<<e.errorCode<<" domain:"<<e.errorDomain<<" what:"<<e.what();
        return NULL;
    } catch (std::exception ee) {
        LERR_ << "[ "<<__PRETTY_FUNCTION__<<"]" << " Library Exception on \""<<chaosNode<<"\""<<" what:"<<ee.what();
        return NULL;
    } catch (...) {
        LERR_ << "[ "<<__PRETTY_FUNCTION__<<"]" << " Unexpected Exception on \""<<chaosNode;
        return NULL;
    }
}

TTree* queryChaosTree(const std::string&chaosNode,const std::string& start, const std::string&end, const int channel,const std::string& treeid, const std::string& desc,int pageLen) {
    return query_int(NULL, chaosNode,start,end, channel,treeid, desc,pageLen,true);
}
TTree* queryChaosTree(TTree* tree_ret, const std::string&chaosNode,
                      const std::string& start, const std::string&end, const int channel,
                      const std::string& treeid, const std::string& desc,int pageLen) {
    return query_int(tree_ret, chaosNode,start,end, channel,treeid, desc,pageLen,true);
}
TTree* queryChaosTreeSB(const std::string&chaosNode,
                        const std::string& start, const std::string&end, const int channel,
                        const std::string& treeid, const std::string& desc,int pageLen) {
    return query_int(NULL, chaosNode,start,end, channel,treeid, desc,pageLen,false);
}


bool queryNextChaosTree(TTree*tree) {
    std::map<TTree*, treeQuery_t>::iterator page = queries.find(tree);
    if (page != queries.end()) {
        ChaosController* ctrl = queries[tree].ctrl;
        int32_t uid = queries[tree].page_uid;
        uint32_t pagelen = queries[tree].page;
        if (ctrl) {
            std::vector<boost::shared_ptr<chaos::common::data::CDataWrapper> > res;
            do {
                uid = ctrl->queryNext(uid, res);
                LDBG_<<"ROOT querynext "<<uid<<" returned:"<<res.size();

                for (std::vector<
                     boost::shared_ptr<chaos::common::data::CDataWrapper> >::iterator i =
                     res.begin(); i != res.end(); i++) {
                    addTree(queries[tree],i->get());
                }
            } while ((pagelen--) && (uid > 0));

        } else {
            queries.erase(page);
            return false;
        }
        if (uid == 0) {
            LDBG_<<"ROOT paged query ended";
            queryFree(tree);

            return false;
        } else if(uid>0) {
            return true;
        } else {
            LERR_<<"ROOT paged query error";
          //  delete ctrl;
           // queries.erase(page);
            queryFree(tree);
            return false;
        }
    }
    LERR_<< "[ "<<__PRETTY_FUNCTION__<<"]" << "not paged query found for this TREE";
    return false;
}
bool queryFree(TTree*tree) {
    std::map<TTree*, treeQuery_t>::iterator page = queries.find(tree);
    if (page != queries.end()) {
        LDBG_<<" removing "<< page->second.nbranch<<" branches";
        delete [] page->second.branch;
        delete page->second.ctrl;

        queries.erase(page);
    }
}

