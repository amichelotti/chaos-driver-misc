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
typedef struct treeQuery{
  TTree* associated;
  uint32_t page_uid;
  int32_t channel;
  uint32_t page;
  bool treeAllocated;
  char* branchBuffer;
  int32_t size;
  std::string branchContent;
  std::string brname;
  ChaosController*ctrl;
  treeQuery(){associated=NULL;size=0;ctrl=NULL;branchBuffer=NULL;}
} treeQuery_t;

static std::map<TTree*, treeQuery_t> queries;
static std::map<std::string, ChaosController*> ctrls;

static int createBranch(treeQuery_t& query,chaos::common::data::CDataWrapper*cd) {
	std::stringstream varname;
	std::vector<std::string> contained_key;
	cd->getAllKey(contained_key);
	if(query.size>0){
		free(query.branchBuffer);
		query.branchBuffer=0;
	}
	query.size=0;

	for (std::vector<std::string>::iterator it = contained_key.begin();
			it != contained_key.end(); it++) {
		int found=0;

		int type_size = CDataWrapperTypeDouble;
		if (cd->isVector(*it)) {
			int size = 0;
			CMultiTypeDataArrayWrapper* da = cd->getVectorValue(*it);
			varname << *it;

			varname << "[" << da->size() << "]";
			found++;
			if (da->size()) {
				if (da->isDoubleElementAtIndex(0)) {
					type_size = CDataWrapperTypeDouble;
					query.size+=da->size()*sizeof(double);

				} else if (da->isInt32ElementAtIndex(0)) {
					type_size = CDataWrapperTypeInt32;
					query.size+=da->size()*sizeof(int32_t);

				} else if (da->isInt64ElementAtIndex(0)) {
					type_size = CDataWrapperTypeInt64;
					query.size+=da->size()*sizeof(int64_t);

				} else if (da->isStringElementAtIndex(0)) {
					type_size = CDataWrapperTypeString;
					query.size+=da->size()*(da->getStringElementAtIndex(0).size());

				}
			}
			LDBG_<<" BELE "<<varname<<" tot size:"<<query.size;

		} else {
			if((type_size==CDataWrapperTypeDouble )||(type_size==CDataWrapperTypeInt64)||(type_size==CDataWrapperTypeBool)){

				type_size = cd->getValueType(*it);
				query.size+=cd->getValueSize(*it);

				/*if((type_size==CDataWrapperTypeString)){
					int ret;
					if(!(ret=(cd->getValueSize(*it)%4))){
						query.size+=ret;
					}
				}*/
				LDBG_<<" BELE "<<*it<< " ele size:"<<cd->getValueSize(*it)<<" tot size:"<<query.size;
			}
		}
		switch (type_size) {
		case CDataWrapperTypeNoType:
			break;
		case CDataWrapperTypeNULL:
			break;
		case CDataWrapperTypeBool:
			varname << *it;
			varname << "/O";
			found++;
			break;
		case CDataWrapperTypeInt32:
			found++;
			varname << *it;
			varname << "/I";

			break;
		case CDataWrapperTypeInt64:
			found++;
			varname << *it;
			varname << "/L";

			break;
		case CDataWrapperTypeDouble:
			found++;
			varname << *it;
			varname << "/D";

			break;
		case CDataWrapperTypeString:
	//		found++;
	//		varname << *it;
	//		varname << "/C";

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
		if(found &&( (it+1)!= contained_key.end())){
			varname<<":";
		}
	}
	query.branchContent=varname.str();
	query.branchBuffer=(char*)malloc(query.size);
	query.associated->Branch(query.brname.c_str(), (void*)query.branchBuffer,varname.str().c_str());
	LDBG_<<"create ROOT BRANCH \""<<query.brname<<"\""<< "content:\""<<varname.str()<<"\" size:"<<query.size<<" address 0x"<<std::hex<<(uint64_t)query.branchBuffer<<std::dec;

	return query.size;
}
#if 0
static void createBranch(TTree* tr, const std::string&prefix,
		const std::string& key, chaos::common::data::CDataWrapper*cd) {
	std::string brname = prefix + "." + key;
	std::stringstream varname;
	varname << key;
	int type_size = CDataWrapperTypeDouble;
	if (cd->isVector(key)) {
		int size = 0;
		CMultiTypeDataArrayWrapper* da = cd->getVectorValue(key);
		varname << "[" << da->size() << "]";
		if (da->size()) {
			if (da->isDoubleElementAtIndex(0)) {
				type_size = CDataWrapperTypeDouble;
			} else if (da->isInt32ElementAtIndex(0)) {
				type_size = CDataWrapperTypeDouble;

			} else if (da->isInt64ElementAtIndex(0)) {
				type_size = CDataWrapperTypeInt64;
			} else if (da->isStringElementAtIndex(0)) {
				type_size = CDataWrapperTypeString;
			}
		}
	} else {
		type_size = cd->getValueType(key);
	}
	switch (type_size) {
	case CDataWrapperTypeNoType:
		break;
	case CDataWrapperTypeNULL:
		break;
	case CDataWrapperTypeBool:
		varname << "/O";
		break;
	case CDataWrapperTypeInt32:
		varname << "/I";

		break;
	case CDataWrapperTypeInt64:
		varname << "/L";

		break;
	case CDataWrapperTypeDouble:
		varname << "/D";

		break;
	case CDataWrapperTypeString:
		varname << "/C";

		break;
	case CDataWrapperTypeBinary:
		break;
	case CDataWrapperTypeObject:
		break;
	case CDataWrapperTypeVector:
		break;
	default:
		varname << "/D";

	}
	LDBG_<<"create ROOT BRANCH \""<<brname<<"\""<< "variable:\""<<varname.str()<<"\"";

	tr->Branch(brname.c_str(), (void*) cd->getRawValuePtr(key),
			varname.str().c_str());
	tr->Fill();
}
#endif
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

static int addTree(treeQuery_t& query, chaos::common::data::CDataWrapper*cd) {
	std::vector < std::string > contained_key;
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
							memcpy(query.branchBuffer+ptr,static_cast<void*>(&tmp),sizeof(tmp));
							ptr+=sizeof(tmp);
						} else if (da->isInt32ElementAtIndex(cnt)) {
							int32_t tmp=da->getInt32ElementAtIndex(cnt);
							memcpy(query.branchBuffer+ptr,static_cast<void*>(&tmp),sizeof(tmp));
							ptr+=sizeof(tmp);
						} else if (da->isInt64ElementAtIndex(cnt)) {
							int64_t tmp=da->getInt64ElementAtIndex(cnt);
							memcpy(query.branchBuffer+ptr,static_cast<void*>(&tmp),sizeof(tmp));
							ptr+=sizeof(tmp);

						} else if (da->isStringElementAtIndex(cnt)) {
							std::string tmp=da->getStringElementAtIndex(cnt);
							memcpy(query.branchBuffer+ptr,(void*)(tmp.c_str()),tmp.size());
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
				memcpy(query.branchBuffer+ptr,cd->getRawValuePtr(*it),cd->getValueSize(*it));
				ptr+=cd->getValueSize(*it);
				break;
			}
			}

		}
	
	}
	LDBG_<<" WRITE ELE "<<query.brname<<" tot size:"<<ptr <<" address 0x"<<std::hex<<(uint64_t)query.branchBuffer<<std::dec;;

	query.associated->Fill();
	return 0;
}
TTree* queryChaosTree(TTree* tree_ret, const std::string&chaosNode,
		const std::string& start, const std::string&end, const int channel,
		const std::string treeid, const std::string desc,int pageLen) {
	std::string brname;
	try {
		ChaosController* ctrl = NULL;
		std::vector<boost::shared_ptr<chaos::common::data::CDataWrapper> > res;
		treeQuery_t q;
		if (ctrls.find(chaosNode) != ctrls.end()) {
			ctrl = ctrls[chaosNode];
		} else {
			ctrl = new ChaosController(chaosNode);
			ctrls[chaosNode] = ctrl;
		}

		int32_t ret = ctrl->queryHistory(start, end, channel, res, pageLen);
		int cnt = 0;
		if (res.size() > 0) {
			if (tree_ret == NULL) {
				tree_ret = buildTree((treeid=="")?chaosNode:treeid, desc);
				q.treeAllocated = true;
				q.brname=(treeid=="")?chaos::datasetTypeToHuman(channel):treeid;
			} else {
				q.brname=(treeid=="")?(chaosNode+std::string(chaos::datasetTypeToHuman(channel))):treeid;

			}
		} else {
			LAPP_<< "CHAOS no entries found from \""<<start<<"\" to \""<<end<<" on \""<<chaosNode<<"\"";
			return NULL;
		}

		if (tree_ret == NULL) {
			LERR_<< "[ "<<__PRETTY_FUNCTION__<<"]" << " cannot create tree on \""<<chaosNode;
			return tree_ret;
		}
		q.associated=tree_ret;
		q.page_uid = ret;
		q.page = pageLen;
		q.ctrl = ctrl;
		q.channel = channel;
		queries[tree_ret] = q;
		createBranch(q,res[0].get());
		for (std::vector<boost::shared_ptr<chaos::common::data::CDataWrapper> >::iterator i =
				res.begin(); i != res.end(); i++) {
			addTree(q, i->get());
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

TTree* queryChaosTree(const std::string&chaosNode, const std::string& start,
		      const std::string&end, const int channel, const std::string& branchid,const std::string& desc,
		int pageLen) {
	return queryChaosTree((TTree*) NULL, chaosNode, start, end, channel, branchid,"",
			pageLen);

}

bool queryHasNextChaosTree(TTree*tree) {
	std::map<TTree*, treeQuery_t>::iterator page = queries.find(tree);
	if (page != queries.end()) {
		ChaosController* ctrl = queries[tree].ctrl;
		int32_t uid = queries[tree].page_uid;
		if (ctrl) {
			return ctrl->queryHasNext(uid);
		} else {
			queries.erase(page);
			return false;
		}
	}

	return false;
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
			LDBG_<<"ROOT paged query ended, free resources";
			delete ctrl;
			queries.erase(page);
			return false;
		} else if(uid>0) {
			return true;
		} else {
			LERR_<<"ROOT paged query error, free resources";
			delete ctrl;
			queries.erase(page);
			return false;
		}
	}
	LERR_<< "[ "<<__PRETTY_FUNCTION__<<"]" << "not paged query found for this TREE";

	return false;
}
bool queryFree(TTree*tree) {

}

