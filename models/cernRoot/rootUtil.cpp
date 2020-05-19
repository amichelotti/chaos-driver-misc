/*
 * File:   testDataSetAttribute.cpp
 * Author: michelo
 * test from UI
 * Created on September 10, 2015, 11:24 AM
 */

#include "rootUtil.h"
#include <driver/misc/core/ChaosController.h>
#include <stdlib.h>
using namespace std;
using namespace chaos::metadata_service_client;
#include "TROOT.h"
#include "TTree.h"
#include <algorithm> // std::min
#define ROOTERR ERR_LOG(rootUtil) 

#define ROOTDBG DBG_LOG(rootUtil)
using namespace chaos::common::data;
using namespace driver::misc;
/*typedef struct branchAlloc {
  char *branchBuffer;
  int32_t size;
  std::string branchContent;
  std::string brname;
  branchAlloc() {
    size = 0;
    branchBuffer = NULL;
  }
  ~branchAlloc() {
    if ((size > 0) && (branchBuffer)) {
      //  ROOTDBG<<" removing breanch:'"<<brname<<"' size:"<<size<<" content
      //  type:"<<branchContent;free(branchBuffer);
    }
  }

} branchAlloc_t;
*/
/***************/
chaosBranch::chaosBranch(TTree *par, const std::string &key,
                         const chaos::common::data::CDataWrapper &cd,const std::string& brsuffix) {
  cdkey=key;
  name = (brsuffix.size())?(brsuffix + "."+ key):key;
  is_vector = false;
  size = 0;
  vector_size = 0;
  chaosSubType = chaos::DataType::TYPE_UNDEFINED;
  chaosType = chaos::DataType::TYPE_DOUBLE;
  parent = par;
  data_element_size = 0;
  boost::regex r("[\\[\\]\\(\\)\\{\\}]+");
  if(boost::regex_search(name,r)){
      throw chaos::CException(-1, "Skipping creation of:"+name+" contains invalid characters", __PRETTY_FUNCTION__);
 
   }
  if (!cd.hasKey(key)) {
    throw chaos::CException(-1, "not found key:" + key, __PRETTY_FUNCTION__);
  }
  std::stringstream varname;
  if (cd.isVector(key)) {
    is_vector = true;
    ChaosSharedPtr<CMultiTypeDataArrayWrapper> da = cd.getVectorValue(key);
    // if(!multiple)

    varname << name << "[__" << key << "__]"; // array lenght must be dynamic
    if (da->size()) {
      vector_size = da->size();
      if (da->isDoubleElementAtIndex(0)) {
        chaosType = chaos::DataType::TYPE_DOUBLE;
        size += da->size() * sizeof(double);
        data_element_size = sizeof(double);
        varname << "/D";
      } else if (da->isInt32ElementAtIndex(0)) {
        chaosType = chaos::DataType::TYPE_INT32;
        size += da->size() * sizeof(int32_t);
        data_element_size = sizeof(int32_t);
        varname << "/I";
      } else if (da->isInt64ElementAtIndex(0)) {
        chaosType = chaos::DataType::TYPE_INT64;
        size += da->size() * sizeof(int64_t);
        data_element_size = sizeof(int64_t);
        varname << "/L";
      } else if (da->isStringElementAtIndex(0)) {
        chaosType = chaos::DataType::TYPE_STRING;
        size += da->size() * (da->getStringElementAtIndex(0).size());
        data_element_size = da->getStringElementAtIndex(0).size();
        varname << "/C";

      } else if (da->isBoolElementAtIndex(0)) {
        chaosType = chaos::DataType::TYPE_STRING;
        size += da->size() * (da->getStringElementAtIndex(0).size());
        data_element_size = da->getStringElementAtIndex(0).size();
        varname << "/O";
      }
    }
    // ROOTDBG<<" BELE Vector "<<varname.str()<<" tot
    // size:"<<query[branch_counter].size;

  } else {
    chaosType = cd.getValueType(key);
    size += cd.getValueSize(key);
    // ROOTDBG<<" BELE "<<*it<< " ele size:"<<cd->getValueSize(*it)<<"
    // type:"<<type_size<<" tot size:"<<query[branch_counter].size;
    data_element_size = cd.getValueSize(key);
    if ((chaosType == chaos::DataType::TYPE_DOUBLE) ||
        (chaosType == chaos::DataType::TYPE_INT32) ||
        (chaosType == chaos::DataType::TYPE_INT64) ||
        (chaosType == chaos::DataType::TYPE_BOOLEAN)) {

      //  //ROOTDBG<<" BELE "<<*it<< " ele size:"<<cd->getValueSize(*it)<<" tot
      //  size:"<<query[branch_counter].size;
    }

    switch (chaosType) {

    case chaos::DataType::TYPE_BOOLEAN:

      varname << name << "/O";

      break;
    case chaos::DataType::TYPE_INT32:
      varname << name << "/I";

      break;
    case chaos::DataType::TYPE_INT64:
      varname << name << "/L";
      break;
    case chaos::DataType::TYPE_DOUBLE:
      varname << name << "/D";

      break;
    case chaos::DataType::TYPE_BYTEARRAY: {
      varname << name;
      is_vector = true;
      switch (cd.getBinarySubtype(key)) {
      case (chaos::DataType::SUB_TYPE_BOOLEAN):
        varname << "[__" << name << "__]/O";
        vector_size = size / sizeof(bool);
        data_element_size = sizeof(bool);
        break;
        //! Integer char bit length
      case chaos::DataType::SUB_TYPE_CHAR:
      case chaos::DataType::SUB_TYPE_MIME:
        varname << "[__" << name << "__]/B";
        vector_size = size / sizeof(char);
        data_element_size = sizeof(char);

        break;
        //! Integer 8 bit length
      case chaos::DataType::SUB_TYPE_INT8:
        varname << "[__" << name << "__]/b";
        vector_size = size / sizeof(int8_t);
        data_element_size = sizeof(int8_t);

        break;
        //! Integer 16 bit length
      case chaos::DataType::SUB_TYPE_INT16:
        varname << "[__" << name << "__]/s";
        vector_size = size / sizeof(int16_t);
        data_element_size = sizeof(int16_t);

        break;
        //! Integer 32 bit length
      case chaos::DataType::SUB_TYPE_INT32:
        varname << "[__" << name << "__]/I";
        vector_size = size / sizeof(int32_t);
        data_element_size = sizeof(int32_t);

        break;
      case chaos::DataType::SUB_TYPE_UNSIGNED:
        varname << "[__" << name << "__]/i";
        vector_size = size / sizeof(int32_t);
        data_element_size = sizeof(int32_t);

        break;
      
        //! Integer 64 bit length
      case chaos::DataType::SUB_TYPE_INT64:
        varname << "[__" << name << "__]/L";
        vector_size = size / sizeof(int64_t);
        data_element_size = sizeof(int64_t);

        break;
        //! Double 64 bit length
      case chaos::DataType::SUB_TYPE_DOUBLE:
        varname << "[__" << name << "__]/D";
        vector_size = size / sizeof(double);
        data_element_size = sizeof(double);

        break;
      }
      break;
    }

    case chaos::DataType::TYPE_STRING:
      varname << name << "/C";

      break;

    default:
      break;
    }
  }
  rootType = varname.str();
  if (size >= 0) {
    ptr = malloc((size==0)?1:size);
    if (is_vector) {
      
      std::string lenname = std::string("__") + name + std::string("__");
      std::string lentype = lenname + "/I";

      if(parent->Branch(lenname.c_str(), &vector_size, lentype.c_str())==NULL){
              throw chaos::CException(-1, "cannot create vector branch len key:" + lenname, __PRETTY_FUNCTION__);

      } else {
     /*    ROOTDBG << "create ROOT  BRANCH \"" << lenname << "\""
              << " vect size:" << vector_size << " " << lentype << " ptr:0x"
              << std::hex << &vector_size << " of size:" << std::dec << size
              << " element size:" << sizeof(int32_t);
              */
      }
      
      if(parent->Branch(name.c_str(), ptr, rootType.c_str())==NULL){
        throw chaos::CException(-1, "cannot create vector branch key:" + name + "rootType:"+rootType, __PRETTY_FUNCTION__);

      }

     /* ROOTDBG << "create ROOT Vector BRANCH \"" << name << "\""
              << " vect size:" << vector_size << " " << rootType << " ptr:0x"
              << std::hex << ptr << " of size:" << std::dec << size
              << " element size:" << data_element_size;
  */
    } else {
      parent->Branch(name.c_str(), ptr, rootType.c_str());
    /*  ROOTDBG << "create ROOT  BRANCH \"" << name << "\""
              << " " << rootType << " ptr:0x" << std::hex << ptr
              << " of size:" << size << " element size:" << data_element_size;
              */
     if(parent->Branch(name.c_str(), ptr, rootType.c_str())==NULL){
        throw chaos::CException(-1, "cannot create branch key:" + name, __PRETTY_FUNCTION__);

      }
    }

  } else {
    throw chaos::CException(-2, "zero size for key:" + name,
                            __PRETTY_FUNCTION__);
  }
}
int chaosBranch::realloc(int newsize) {
  void *old_ptr = ptr;
  ptr = std::realloc(ptr, newsize);
  if (ptr != old_ptr) {
    parent->SetBranchAddress(name.c_str(), ptr);
  }
  size = newsize;
  return size;
}
chaosBranch::~chaosBranch() { free(ptr); }
bool chaosBranch::add(const chaos::common::data::CDataWrapper &cd) {
  if (!cd.hasKey(cdkey)) {
    LERR_<<"branch:"<<name<<" cannot process key '"<<cdkey<<"' not present in:"<<cd.getJSONString();
   return false;
  }
  if (cd.isVector(cdkey)) {
    ChaosSharedPtr<CMultiTypeDataArrayWrapper> da = cd.getVectorValue(cdkey);
    if (da->size()) {
      if (da->size() > vector_size) {
        realloc(data_element_size * da->size());
      }
      vector_size = da->size();
    }
    for (int cnt = 0; cnt < da->size(); cnt++) {
      uint32_t siz;
      uint64_t addr=(uint64_t)ptr;
      addr+=(data_element_size * cnt);
      memcpy((void*)addr ,
             (void *)da->getRawValueAtIndex(cnt, siz), data_element_size);
    }
  } else {
    if (cd.getValueSize(cdkey) > size) {
      realloc(cd.getValueSize(cdkey));
    }
    memcpy(ptr, (void *)cd.getRawValuePtr(cdkey), cd.getValueSize(cdkey));
  }
  return true;
}

ChaosToTree::ChaosToTree(TTree *rot,const std::string&_brsuffix) : root(rot),brsuffix(_brsuffix) {}

ChaosToTree::ChaosToTree(const ::std::string &treename,const std::string&_brsuffix)
    : root(NULL), tname(treename),brsuffix(_brsuffix) {
  root = new TTree(treename.c_str(), "");
}
int ChaosToTree::addData(const chaos::common::data::CDataWrapper &cd) {
  if (branches.size() == 0) {
    std::vector<std::string> contained_key;
    cd.getAllKey(contained_key);
    for (std::vector<std::string>::iterator i = contained_key.begin();
         i != contained_key.end(); i++) {
      try {
        boost::regex r("[\\[\\]\\(\\)\\{\\}]+");
        if(boost::regex_match(*i,r)){
                  ROOTERR<<"Skipping creation of:"<<*i<<" contains invalid characters:";
          continue;
        } else {
          chaosBranch *br = new chaosBranch(root, *i, cd,brsuffix);
          branches[*i] = ChaosSharedPtr<chaosBranch>(br);
        }
      } catch (chaos::CException&e) {
        ROOTERR<<"creating branch:"<<*i<<" error:"<<e.errorDomain<< " msg:"<<e.errorMessage;
      }
    }
  }
  for (branch_map_t::iterator i = branches.begin(); i != branches.end(); i++) {
    if(i->second->add(cd)==false){
      ROOTERR<<"Error adding "+i->first;
    }
  }
 root->Fill();

  return 0;
}
TTree *ChaosToTree::getTree() { return root; }
ChaosToTree::~ChaosToTree() {
  // delete root; //removed by tfile
   for (branch_map_t::iterator i = branches.begin(); i != branches.end(); i++) {
    i->second.reset();
  }
}

/*************/
typedef struct treeQuery {
  uint32_t page_uid;
  int32_t channel;
  uint32_t page;
  bool treeAllocated;
  TTree *tree;
  ChaosToTree *branch;
  int nbranch;
  ChaosController *ctrl;
  treeQuery() {
    ctrl = NULL;
    branch = NULL;
    treeAllocated = false;
    tree = NULL;
    nbranch = 0;
  }
} treeQuery_t;



static std::map<TTree *, treeQuery_t> queries;
static TTree *buildTree(const std::string &name, const std::string &desc) {
  TTree *tr = new TTree(name.c_str(), desc.c_str());
  // ROOTDBG<<"create ROOT TREE \""<<name<<"\""<< " desc:\""<<desc<<"\"
  // tree:"<<std::hex<<(void*)tr<<std::dec;

  if (tr == NULL) {
    ROOTERR << "[ " << __PRETTY_FUNCTION__ << "]"
          << " cannot create tree  \"" << name << "\"";

    return NULL;
  }

 
  return tr;
}
// static std::map<std::string, ChaosController*> ctrls;
// create a unique branch with all dataset
/*
branchAlloc_t *createBranch(TTree *tr, treeQuery &q,
                            const chaos::common::data::CDataWrapper *cd,
                            const std::string &brname, bool multiple = true) {
  std::stringstream varname;
  std::vector<std::string> contained_key;
  int disable_dump = 0;
  cd->getAllKey(contained_key);
  branchAlloc_t *query = NULL;
  int branch_counter = 0;
  std::string branch_prefix;
  if (contained_key.size() == 0) {
    return NULL;
  }
  if (multiple) {
    query = new branchAlloc_t[contained_key.size()];
    q.nbranch = contained_key.size();

  } else {
    query = new branchAlloc_t[1];
    q.nbranch = 1;
    branch_prefix = ""; // brname+std::string("__");
  }
  // ROOTDBG<<" creating "<<q.nbranch<<" branches";

  q.branch = query;
  if (query[0].size > 0) {
    free(query[0].branchBuffer);
    query[0].branchBuffer = 0;
  }
  query[0].size = 0;

  for (std::vector<std::string>::iterator it = contained_key.begin();
       it != contained_key.end(); it++, branch_counter++) {
    disable_dump = 0;
    int found = 0;
    if (multiple == false) {
      branch_counter = 0;
    } else {
      varname.str(branch_prefix);
      if (query[branch_counter].size > 0) {
        free(query[branch_counter].branchBuffer);
        query[branch_counter].branchBuffer = 0;
      }
      query[branch_counter].size = 0;
    }
    chaos::DataType::DataType type_size = chaos::DataType::TYPE_DOUBLE;
    if (cd->isVector(*it)) {
      int size = 0;
      ChaosSharedPtr<CMultiTypeDataArrayWrapper> da = cd->getVectorValue(*it);
      // if(!multiple)
      varname << *it;

      varname << "[" << da->size() << "]";
      found++;
      if (da->size()) {
        if (da->isDoubleElementAtIndex(0)) {
          type_size = chaos::DataType::TYPE_DOUBLE;
          query[branch_counter].size += da->size() * sizeof(double);

        } else if (da->isInt32ElementAtIndex(0)) {
          type_size = chaos::DataType::TYPE_INT32;
          query[branch_counter].size += da->size() * sizeof(int32_t);

        } else if (da->isInt64ElementAtIndex(0)) {
          type_size = chaos::DataType::TYPE_INT64;
          query[branch_counter].size += da->size() * sizeof(int64_t);

        } else if (da->isStringElementAtIndex(0)) {
          type_size = chaos::DataType::TYPE_STRING;
          query[branch_counter].size +=
              da->size() * (da->getStringElementAtIndex(0).size());
        }
      }
      // ROOTDBG<<" BELE Vector "<<varname.str()<<" tot
      // size:"<<query[branch_counter].size;

    } else {
      type_size = cd->getValueType(*it);
      query[branch_counter].size += cd->getValueSize(*it);
      // ROOTDBG<<" BELE "<<*it<< " ele size:"<<cd->getValueSize(*it)<<"
      // type:"<<type_size<<" tot size:"<<query[branch_counter].size;

      if ((type_size == chaos::DataType::TYPE_DOUBLE) ||
          (type_size == chaos::DataType::TYPE_INT32) ||
          (type_size == chaos::DataType::TYPE_INT64) ||
          (type_size == chaos::DataType::TYPE_BOOLEAN)) {

        //  //ROOTDBG<<" BELE "<<*it<< " ele size:"<<cd->getValueSize(*it)<<"
        //  tot size:"<<query[branch_counter].size;
      }
    }
    switch (type_size) {

    case chaos::DataType::TYPE_BOOLEAN:
      found++;
      //   if(!multiple)
      varname << *it;
      varname << "/O";

      break;
    case chaos::DataType::TYPE_INT32:
      found++;
      //     if(!multiple)
      varname << *it;
      varname << "/I";

      break;
    case chaos::DataType::TYPE_INT64:
      found++;
      // if(!multiple)
      varname << *it;
      varname << "/L";

      break;
    case chaos::DataType::TYPE_DOUBLE:
      found++;
      // if(!multiple)
      varname << *it;
      varname << "/D";

      break;
    case chaos::DataType::TYPE_BYTEARRAY: {
      int binsize = cd->getValueSize(*it);
      varname << *it;

      switch (cd->getBinarySubtype(*it)) {
      case (chaos::DataType::SUB_TYPE_BOOLEAN):
        varname << "[" << binsize / sizeof(bool) << "]/O";

        break;
        //! Integer char bit length
      case chaos::DataType::SUB_TYPE_CHAR:
        varname << "[" << binsize / sizeof(char) << "]/B";

        break;
        //! Integer 8 bit length
      case chaos::DataType::SUB_TYPE_INT8:
        varname << "[" << binsize / sizeof(int8_t) << "]/b";

        break;
        //! Integer 16 bit length
      case chaos::DataType::SUB_TYPE_INT16:
        varname << "[" << binsize / sizeof(int16_t) << "]/s";

        break;
        //! Integer 32 bit length
      case chaos::DataType::SUB_TYPE_INT32:
        varname << "[" << binsize / sizeof(int32_t) << "]/I";

        break;
        //! Integer 64 bit length
      case chaos::DataType::SUB_TYPE_INT64:
        varname << "[" << binsize / sizeof(int64_t) << "]/L";

        break;
        //! Double 64 bit length
      case chaos::DataType::SUB_TYPE_DOUBLE:
        varname << "[" << binsize / sizeof(double) << "]/D";

        break;
      }
      break;
    }

    case chaos::DataType::TYPE_STRING:
      disable_dump = 1;
      //               found++;
      //               varname << *it;
      //           varname << "/C";

      break;

    default:
      break;
    }
    if ((multiple == false) && (found && ((it + 1) != contained_key.end()))) {
      varname << ":";
    }
    if (multiple && (disable_dump == 0)) {
      std::stringstream ss;
      ss << brname << "." << (*it);
      query[branch_counter].branchContent = varname.str();
      query[branch_counter].branchBuffer =
          (char *)malloc(query[branch_counter].size);
      query[branch_counter].brname = ss.str();
      tr->Branch(query[branch_counter].brname.c_str(),
                 (void *)query[branch_counter].branchBuffer,
                 varname.str().c_str());
      // ROOTDBG<<"create ROOT BRANCH \""<<query[branch_counter].brname<<"\""<<
      // " content:\""<<varname.str()<<"\" size:"<<query[branch_counter].size<<"
      // address
      // 0x"<<std::hex<<(uint64_t)query[branch_counter].branchBuffer<<std::dec;
    }
  }
  if (multiple == false) {
    query[0].brname = brname;
    query[0].branchContent = varname.str();
    query[0].branchBuffer = (char *)malloc(query[0].size);
    tr->Branch(brname.c_str(), (void *)query[0].branchBuffer,
               varname.str().c_str());
    // ROOTDBG<<"create ROOT BRANCH \""<<query[0].brname<<"\""<<
    // "content:\""<<varname.str()<<"\" size:"<<query[0].size<<" address
    // 0x"<<std::hex<<(uint64_t)query[0].branchBuffer<<std::dec;
  }

  return query;
}



static int addTree(treeQuery_t &q,
                   const chaos::common::data::CDataWrapper *cd) {
  std::vector<std::string> contained_key;
  branchAlloc_t *query = q.branch;
  int branch_counter = 0;
  int type_size = 0;
  cd->getAllKey(contained_key);
  int ptr = 0;
  for (std::vector<std::string>::iterator it = contained_key.begin();
       it != contained_key.end(); it++) {
    int maxsize = query[branch_counter].size;

    if (cd->isVector(*it)) {
      int size = 0;
      ChaosSharedPtr<CMultiTypeDataArrayWrapper> da = cd->getVectorValue(*it);

      for (int cnt = 0; cnt < da->size(); cnt++) {
        if (da->isDoubleElementAtIndex(cnt)) {
          double tmp = da->getDoubleElementAtIndex(cnt);
          memcpy(query[branch_counter].branchBuffer + ptr,
                 static_cast<void *>(&tmp), sizeof(tmp));
          ptr += sizeof(tmp);
          maxsize -= sizeof(tmp);
          ;
        } else if (da->isInt32ElementAtIndex(cnt)) {
          int32_t tmp = da->getInt32ElementAtIndex(cnt);
          memcpy(query[branch_counter].branchBuffer + ptr,
                 static_cast<void *>(&tmp), sizeof(tmp));
          ptr += sizeof(tmp);
          maxsize -= sizeof(tmp);
          ;

        } else if (da->isInt64ElementAtIndex(cnt)) {
          int64_t tmp = da->getInt64ElementAtIndex(cnt);
          memcpy(query[branch_counter].branchBuffer + ptr,
                 static_cast<void *>(&tmp), sizeof(tmp));
          ptr += sizeof(tmp);
          maxsize -= sizeof(tmp);
          ;

        } else if (da->isStringElementAtIndex(cnt)) {
          std::string tmp = da->getStringElementAtIndex(cnt);
          memcpy(query[branch_counter].branchBuffer + ptr,
                 (void *)(tmp.c_str()), std::min((int)tmp.size(), maxsize));
          ptr += tmp.size();
          maxsize -= sizeof(tmp);
          ;
        }
      }
    } else {
      // ROOTDBG<<"ELE "<<*it<<" size:"<<cd->getValueSize(*it);
      switch (cd->getValueType(*it)) {
      case chaos::DataType::TYPE_DOUBLE:
      case chaos::DataType::TYPE_INT64:
      case chaos::DataType::TYPE_BOOLEAN:
      case chaos::DataType::TYPE_INT32:
      case chaos::DataType::TYPE_BYTEARRAY: {
        memcpy(query[branch_counter].branchBuffer + ptr,
               cd->getRawValuePtr(*it),
               std::min((int)cd->getValueSize(*it), maxsize));
        ptr += cd->getValueSize(*it);
        maxsize -= cd->getValueSize(*it);
        ;

        break;
      }
      }
      // memcpy(query[branch_counter].branchBuffer+ptr,cd->getRawValuePtr(*it),cd->getValueSize(*it));
      // ptr+=cd->getValueSize(*it);
    }
    if (maxsize < 0) {
      cout << maxsize << " corrupt:" << cd->getJSONString() << std::endl;
    }
    if ((branch_counter + 1) < q.nbranch) {
      // if multiple branch buffer is different, offset to zero
      ptr = 0;
      branch_counter++;
    }
  }
  q.tree->Fill();
  return 0;
}
*/
static TTree *
query_int(TTree *tree_ret, const std::string &chaosNode,
          const std::string &start, const std::string &end, const int channel,
          const std::string treeid, const std::string desc, int pageLen,
          bool multi, uint64_t runid, uint64_t pckid,
          const std::vector<std::string> tags = std::vector<std::string>()) {
  std::string brname;
  try {
    treeQuery_t q;
    ChaosToTree *branch = NULL;
    q.treeAllocated = false;

    ChaosController *ctrl = NULL;
    std::vector<boost::shared_ptr<chaos::common::data::CDataWrapper>> res;
  
    ctrl = new ChaosController(chaosNode);

    int32_t ret = ctrl->queryHistory(start, end, runid, pckid, tags, channel,
                                     res, ChaosStringSet(), pageLen);
    int cnt = 0;
    if (res.size() > 0) {
      // ROOTDBG<<"Query result size with pagelen "<<pageLen<<" is
      // "<<res.size()<<" elements";

      if (tree_ret == NULL) {
        tree_ret = buildTree((treeid == "") ? chaosNode : treeid, desc);
        brname = (treeid == "") ? chaos::datasetTypeToHuman(channel) : treeid;
        q.treeAllocated = true;
      } else {
        brname =
            (treeid == "")
                ? (chaosNode + std::string(chaos::datasetTypeToHuman(channel)))
                : treeid;
      }
      q.tree = tree_ret;

      std::vector<std::string> contained_key;
      boost::shared_ptr<chaos::common::data::CDataWrapper> cd = res[0];
      if(cd.get()){
        branch = new ChaosToTree(tree_ret,brname);
        if (branch == NULL) {
                ROOTERR << "Cannot create branches";

            delete tree_ret;
            return NULL;
        }
        branch->addData(*cd.get());
        q.branch=branch;
      } else {
          delete branch;
          
           ROOTERR << "Invalid data found";
      return NULL;
      }
      /*createBranch(tree_ret, q, cd.get(),
                            chaos::datasetTypeToHuman(channel), multi);*/

    } else {
      LAPP_ << "CHAOS no entries found from \"" << start << "\" to \"" << end
            << " on \"" << chaosNode << "\"";
      return NULL;
    }

    if (tree_ret == NULL) {
      ROOTERR << "[ " << __PRETTY_FUNCTION__ << "]"
            << " cannot create tree on \"" << chaosNode;
      return tree_ret;
    }
    
    q.page_uid = ret;
    q.page = pageLen;
    q.ctrl = ctrl;
    q.channel = channel;
    queries[tree_ret] = q;
    for (std::vector<
             boost::shared_ptr<chaos::common::data::CDataWrapper>>::iterator i =
             res.begin()+1;
         i != res.end(); i++) {
      //addTree(q, i->get());
        branch->addData(*(i->get()));
    }

    if (pageLen == 0) {
      queryFree(tree_ret);
    }

    return tree_ret;
  } catch (chaos::CException& e) {
    ROOTERR << "[ " << __PRETTY_FUNCTION__ << "]"
          << "Exception on \"" << chaosNode << "\""
          << " errn:" << e.errorCode << " domain:" << e.errorDomain
          << " what:" << e.what();
    return NULL;
  } catch (std::exception& ee) {
    ROOTERR << "[ " << __PRETTY_FUNCTION__ << "]"
          << " Library Exception on \"" << chaosNode << "\""
          << " what:" << ee.what();
    return NULL;
  } catch (...) {
    ROOTERR << "[ " << __PRETTY_FUNCTION__ << "]"
          << " Unexpected Exception on \"" << chaosNode;
    return NULL;
  }
}

static TTree *
query_int(TTree *tree_ret, const std::string &chaosNode,
          const std::string &start, const std::string &end, const int channel,
          const std::string treeid, const std::string desc, int pageLen,
          bool multi,
          const std::vector<std::string> tags = std::vector<std::string>()) {

  return query_int(tree_ret, chaosNode, start, end, channel, treeid, desc,
                   pageLen, multi, 0, 0, tags);
}
TTree *queryChaosTree(const std::string &chaosNode, const std::string &start,
                      const std::string &end,
                      const std::vector<std::string> &tags, const int channel,
                      const std::string &treeid, const std::string &desc,
                      int pageLen) {
  return query_int(NULL, chaosNode, start, end, channel, treeid, desc, pageLen,
                   true, tags);
}
TTree *queryChaosTree(const std::string &chaosNode,
                      const std::vector<std::string> &tags,
                      const uint64_t runid, const int channel,
                      const std::string &treeid, const std::string &desc,
                      int pageLen) {

  return query_int(NULL, chaosNode, "0", "-1", channel, treeid, desc, pageLen,
                   true, tags);
}

TTree *queryChaosTree(const std::string &chaosNode, const std::string &start,
                      const std::string &end, const int channel,
                      const std::string &treeid, const std::string &desc,
                      int pageLen) {
  return query_int(NULL, chaosNode, start, end, channel, treeid, desc, pageLen,
                   true);
}
TTree *queryChaosTree(TTree *tree_ret, const std::string &chaosNode,
                      const std::string &start, const std::string &end,
                      const int channel, const std::string &treeid,
                      const std::string &desc, int pageLen) {
  return query_int(tree_ret, chaosNode, start, end, channel, treeid, desc,
                   pageLen, true);
}
TTree *queryChaosTreeSB(const std::string &chaosNode, const std::string &start,
                        const std::string &end, const int channel,
                        const std::string &treeid, const std::string &desc,
                        int pageLen) {
  return query_int(NULL, chaosNode, start, end, channel, treeid, desc, pageLen,
                   false);
}

bool queryNextChaosTree(TTree *tree) {
  std::map<TTree *, treeQuery_t>::iterator page = queries.find(tree);
  // ROOTDBG<<"ROOT check next tree:0x"<<std::hex<<tree<<std::dec;

  if (page != queries.end()) {
    ChaosController *ctrl = queries[tree].ctrl;
    int32_t uid = queries[tree].page_uid;
    uint32_t pagelen = queries[tree].page;
    int totcnt = 0, last_size;
    ;
    // ROOTDBG<<"ROOT retriving uid:"<<uid<<" pagelen:"<<pagelen;
    if (ctrl) {
      do {
        std::vector<boost::shared_ptr<chaos::common::data::CDataWrapper>> res;

        uid = ctrl->queryNext(uid, res);
        last_size = res.size();
        totcnt += last_size;
        // ROOTDBG<<"ROOT querynext uid:"<<uid<<" returned:"<<last_size<<"
        // tot:"<<totcnt;

        for (std::vector<boost::shared_ptr<chaos::common::data::CDataWrapper>>::
                 iterator i = res.begin();
             i != res.end(); i++) {
                 queries[tree].branch->addData(*(i->get()));
          //addTree(queries[tree], i->get());
        }

      } while ((--pagelen) && (uid > 0));
      // ROOTDBG<<"ROOT querynext uid:"<<uid<<" last returned:"<<last_size<<"
      // tot:"<<totcnt;
    } else {
      // ROOTDBG<<"NO Controller associated";

      queries.erase(page);
      return false;
    }
    if (uid == 0) {
      // ROOTDBG<<"ROOT paged query ended:"<<std::hex<<tree<<std::dec;
      queryFree(tree);

      return false;
    } else if (uid > 0) {
      return true;
    } else {
      ROOTERR << "ROOT paged query error";
      //  delete ctrl;
      // queries.erase(page);
      // queryFree(tree);
      return false;
    }
  }
  ROOTERR << "not paged query found for this TREE";
  return false;
}
bool queryFree(TTree *tree) {
  std::map<TTree *, treeQuery_t>::iterator page = queries.find(tree);
  // ROOTDBG<<" Freeing tree:"<<std::hex<<tree<<std::dec;
  if (page != queries.end()) {

    // ROOTDBG<<" removing "<< page->second.nbranch<<" branches";
    delete page->second.branch;
    delete page->second.ctrl;
    queries.erase(page);

    return true;
  }
  // ROOTDBG<<" Not found tree:"<<std::hex<<tree<<std::dec;

  return false;
}

#include "TBufferJSON.h"
void treeToCDataWrapper(chaos::common::data::CDataWrapper &dst,
                        const TTree *val) {
  TString json = TBufferJSON::ConvertToJSON(val);
  dst.setSerializedJsonData(json.Data());
}

void treeToCDataWrapper(chaos::common::data::CDataWrapper &dst,
                        const std::string &key, const TTree *val) {
  TString json = TBufferJSON::ConvertToJSON(val);
  chaos::common::data::CDataWrapper tmp;
  tmp.setSerializedJsonData(json.Data());
  dst.addCSDataValue(key, tmp);
}
TTree *getTreeFromCDataWrapper(const chaos::common::data::CDataWrapper &src,
                               const std::string &name,
                               const std::string &brname) {
  TTree *tr = new TTree(name.c_str(),"");
  ChaosToTree c2t(tr,brname);
  c2t.addData(src);
  return tr;
}

void initChaosRoot() { ROOTDBG << "initializing ChaosRoot"; }
