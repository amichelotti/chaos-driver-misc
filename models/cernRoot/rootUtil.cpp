/* 
 * File:   testDataSetAttribute.cpp
 * Author: michelo
 * test from UI
 * Created on September 10, 2015, 11:24 AM
 */

#include <ChaosMetadataServiceClient/ChaosMetadataServiceClient.h>
#include <driver/misc/core/ChaosController.h>
#include "rootUtil.h"
using namespace std;
using namespace chaos::metadata_service_client;
#include "TROOT.h"
#include "TTree.h"

using namespace chaos::common::data;
using namespace driver::misc;
static void createBranch(TTree* tr,const std::string&prefix,const std::string& key, chaos::common::data::CDataWrapper*cd){
	std::string brname=prefix+"."+key;
	std::stringstream varname;
	varname<<key;
	int type_size=CDataWrapperTypeDouble;
	if(cd->isVector(key)){
		int size=0;
		CMultiTypeDataArrayWrapper* da=cd->getVectorValue(key);
		varname<<"["<<da->size()<<"]";
		if(da->size()){
			if(da->isDoubleElementAtIndex(0)){
				type_size=CDataWrapperTypeDouble;
			} else if(da->isInt32ElementAtIndex(0)){
				type_size=CDataWrapperTypeDouble;

			} else if(da->isInt64ElementAtIndex(0)){
				type_size=CDataWrapperTypeInt64;
			} else if(da->isStringElementAtIndex(0)){
				type_size=CDataWrapperTypeString;
			}
		}
	} else {
		type_size=cd->getValueType(key);
	}
		switch(type_size) {
				  case CDataWrapperTypeNoType:
				            break;
				  case CDataWrapperTypeNULL:
				           break;
				  case CDataWrapperTypeBool:
					varname<<"/O";
				      break;
				  case CDataWrapperTypeInt32:
					  varname<<"/I";


				       break;
				   case CDataWrapperTypeInt64:
						  varname<<"/L";


				       break;
				   case CDataWrapperTypeDouble:
						  varname<<"/D";



				       break;
				   case CDataWrapperTypeString:
						  varname<<"/C";


				       break;
				   case CDataWrapperTypeBinary:
				       break;
				   case CDataWrapperTypeObject:
					   break;
				   case CDataWrapperTypeVector:
				       break;
				   default:
						  varname<<"/D";

		}
		 LDBG_<<"create ROOT BRANCH \""<<brname<<"\""<< "variable:\""<<varname<<"\"";

		  tr->Branch(brname.c_str(),(void*)cd->getRawValuePtr(key),varname.str().c_str());
		  tr->Fill();
}

TTree* buildTree(const std::string& name,const std::string& desc,const std::string &prefix,chaos::common::data::CDataWrapper*cd){
	TTree* tr=new TTree(name.c_str(),desc.c_str());
	 LDBG_<<"create ROOT TREE \""<<name<<"\""<< "desc:\""<<desc<<"\"";

	std::vector<std::string> contained_key;
	cd->getAllKey(contained_key);
	if(tr==NULL){
		LERR_ << "[ "<<__PRETTY_FUNCTION__<<"]" << " cannot create tree  \""<<name<<"\"";

		return NULL;
	}
	for(std::vector<std::string>::iterator it =  contained_key.begin();it != contained_key.end();it++) {

		createBranch(tr,prefix,*it,cd);
	}
	return tr;
}

int addTree(TTree*tr,const std::string &prefix,chaos::common::data::CDataWrapper*cd){
	std::vector<std::string> contained_key;

	cd->getAllKey(contained_key);
	for(std::vector<std::string>::iterator it =  contained_key.begin();it != contained_key.end();it++) {
		std::string brname=prefix+"."+*it;

		 TBranch *ret=tr->GetBranch(brname.c_str());
		 if(ret){
			// LDBG_<<"adding \""<<it->c_str()<<" ptr 0x"<<std::hex<<(void*)cd->getRawValuePtr(*it)<<std::dec;
			 ret->SetAddress((void*)(cd->getRawValuePtr(*it)));
		 } else {
			 createBranch(tr,prefix,*it,cd);
		 }
		 tr->Fill();
	}
	return 0;
}
typedef struct{
	uint32_t page_uid;
	int32_t channel;
	uint32_t page;
	ChaosController*ctrl;
} treeQuery_t;
static std::map<TTree*,treeQuery_t> pagedqueries;
static std::map<std::string,ChaosController* > ctrls;
TTree* queryChaosTree(TTree* tree_ret,const std::string&chaosNode,const std::string& start,const std::string&end,const int channel,const std::string treeid,int pageLen){
	try{
			ChaosController* ctrl=NULL;
			std::vector<boost::shared_ptr<chaos::common::data::CDataWrapper> > res;
			if(ctrls.find(chaosNode)!=ctrls.end()){
				ctrl=ctrls[chaosNode];
			} else {
				ctrl = new ChaosController(chaosNode);
				ctrls[chaosNode]=ctrl;
			}

			int32_t ret = ctrl->queryHistory(start,end,channel,res,pageLen);
			int cnt=0;
			if(res.size()>0){
				if(tree_ret==NULL){
					tree_ret= buildTree(chaosNode,treeid,chaos::datasetTypeToHuman(channel),res[0].get());
				}
			} else {
				LAPP_ << "CHAOS no entries found from \""<<start<<"\" to \""<<end<<" on \""<<chaosNode<<"\"";
				return NULL;
			}

			if(tree_ret==NULL){
				LERR_ << "[ "<<__PRETTY_FUNCTION__<<"]" << " cannot create tree on \""<<chaosNode;
				return tree_ret;
			}
			for(std::vector<boost::shared_ptr<chaos::common::data::CDataWrapper> >::iterator i=res.begin();i!=res.end();i++){
				addTree(tree_ret,chaos::datasetTypeToHuman(channel),i->get());
			}
			if(pageLen>0){
				treeQuery_t q;
				q.page_uid=ret;
				q.page=pageLen;
				q.ctrl=ctrl;
				q.channel=channel;
				pagedqueries[tree_ret]=q;
			}
			return tree_ret;
		} catch (chaos::CException e) {
			LERR_ << "[ "<<__PRETTY_FUNCTION__<<"]" << "Exception on \""<<chaosNode<<"\""<< " errn:"<<e.errorCode<<" domain:"<<e.errorDomain<<" what:"<<e.what();
			return NULL;
		} catch (std::exception ee) {
			LERR_ << "[ "<<__PRETTY_FUNCTION__<<"]" << " Library Exception on \""<<chaosNode<<"\""<<" what:"<<ee.what();
			return NULL;
		} catch (...) {
			LERR_ << "[ "<<__PRETTY_FUNCTION__<<"]" << " Unexpected Exception on \""<<chaosNode;
			return NULL;
		}
}

TTree* queryChaosTree(const std::string&chaosNode,const std::string& start,const std::string&end,const int channel,const std::string treeid,int pageLen){
	return queryChaosTree((TTree*)NULL,chaosNode,start,end,channel,treeid,pageLen);

}

bool queryHasNextChaosTree(TTree*tree){
	std::map<TTree*,treeQuery_t>::iterator page=pagedqueries.find(tree);
	if(page!=pagedqueries.end()){
			ChaosController* ctrl=pagedqueries[tree].ctrl;
			int32_t uid=pagedqueries[tree].page_uid;
			if(ctrl){
				return ctrl->queryHasNext(uid);
			} else {
				pagedqueries.erase(page);
				return false;
			}
	}

		return false;
}

bool queryNextChaosTree(TTree*tree){
	std::map<TTree*,treeQuery_t>::iterator page=pagedqueries.find(tree);
	if(page!=pagedqueries.end()){
		ChaosController* ctrl=pagedqueries[tree].ctrl;
		int32_t uid=pagedqueries[tree].page_uid;
		uint32_t pagelen=pagedqueries[tree].page;
		if(ctrl){
			std::vector<boost::shared_ptr<chaos::common::data::CDataWrapper> > res;
			do{
			  uid=ctrl->queryNext(uid,res);
			  LDBG_<<"ROOT querynext "<<uid<<" returned:"<<res.size();

			  for(std::vector<boost::shared_ptr<chaos::common::data::CDataWrapper> >::iterator i=res.begin();i!=res.end();i++){
			    addTree(tree,chaos::datasetTypeToHuman(pagedqueries[tree].channel),i->get());
			  }
			} while((pagelen-- )&& (uid>0));

		} else {
			pagedqueries.erase(page);
			return false;
		}
		if(uid==0){
			 LDBG_<<"ROOT paged query ended, free resources";
			 delete ctrl;
			 pagedqueries.erase(page);
			 return false;
		} else if(uid>0){
			return true;
		} else {
			 LERR_<<"ROOT paged query error, free resources";
			 delete ctrl;
			pagedqueries.erase(page);
			return false;
		}
	}
	LERR_ << "[ "<<__PRETTY_FUNCTION__<<"]" << "not paged query found for this TREE";

	return false;
}
