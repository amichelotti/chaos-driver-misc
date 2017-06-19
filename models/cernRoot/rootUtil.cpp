/* 
 * File:   testDataSetAttribute.cpp
 * Author: michelo
 * test from UI
 * Created on September 10, 2015, 11:24 AM
 */

#include <ChaosMetadataServiceClient/ChaosMetadataServiceClient.h>
#include "ChaosController.h"
#include "rootUtil.h"
using namespace std;
using namespace chaos::metadata_service_client;
#include "TROOT.h"
#include "TTree.h"

using namespace chaos::common::data;
using namespace driver::misc;
TTree* buildTree(const std::string& name,const std::string& desc,chaos::common::data::CDataWrapper*cd){
	TTree* tr=new TTree(name.c_str(),desc.c_str());
	std::vector<std::string> contained_key;
	cd->getAllKey(contained_key);
	for(std::vector<std::string>::iterator it =  contained_key.begin();it != contained_key.end();it++) {
			 switch(cd->getValueType(*it)) {
			  case CDataWrapperTypeNoType:
			            break;
			  case CDataWrapperTypeNULL:
			           break;
			  case CDataWrapperTypeBool:
				LDBG_<<"creating branch \""<<it->c_str()<<"\" Boolean";

				  tr->Branch(it->c_str(),(void*)cd->getRawValuePtr(*it),"O");
			       break;
			  case CDataWrapperTypeInt32:
				  tr->Branch(it->c_str(),(void*)cd->getRawValuePtr(*it),"I");
					LDBG_<<"creating branch \""<<it->c_str()<<"\" Int32";

			       break;
			   case CDataWrapperTypeInt64:
				   tr->Branch(it->c_str(),(void*)cd->getRawValuePtr(*it),"L");
					LDBG_<<"creating branch \""<<it->c_str()<<"\" Int64";

			       break;
			   case CDataWrapperTypeDouble:
				   tr->Branch(it->c_str(),(void*)cd->getRawValuePtr(*it),"D");
					LDBG_<<"creating branch \""<<it->c_str()<<"\" Double";

			       break;
			   case CDataWrapperTypeString:
				   tr->Branch(it->c_str(),(void*)cd->getRawValuePtr(*it),"C");
					LDBG_<<"creating branch \""<<it->c_str()<<"\" String";

			       break;
			   case CDataWrapperTypeBinary:
			       break;
			   case CDataWrapperTypeObject:
				   break;
			   case CDataWrapperTypeVector:
			       break;
			 }

	}
	return tr;
}

int addTree(TTree*tr,chaos::common::data::CDataWrapper*cd){
	std::vector<std::string> contained_key;
	cd->getAllKey(contained_key);
	for(std::vector<std::string>::iterator it =  contained_key.begin();it != contained_key.end();it++) {
		 TBranch *ret=tr->GetBranch(it->c_str());
		 if(ret){
			// LDBG_<<"adding \""<<it->c_str()<<" ptr 0x"<<std::hex<<(void*)cd->getRawValuePtr(*it)<<std::dec;
			 ret->SetAddress((void*)(cd->getRawValuePtr(*it)));
			 ret->Fill();
		 }
	}
	return 0;
}

TTree* queryChaosTree(const std::string&chaosNode,const std::string& start,const std::string&end,const int channel,const std::string treeid,int pageLen){
	try{
		TTree* tree_ret=NULL;
		std::vector<boost::shared_ptr<chaos::common::data::CDataWrapper> > res;
		ChaosController ctrl(chaosNode);
		int32_t ret = ctrl.queryHistory(start,end,channel,res,pageLen);
		int cnt=0;
		if(res.size()>0){
			tree_ret= buildTree(chaosNode,treeid,res[0].get());
		} else {
			LAPP_ << "CHAOS no entries found from \""<<start<<"\" to \""<<end<<" on \""<<chaosNode<<"\"";
			return NULL;
		}

		if(tree_ret==NULL){
			LERR_ << "[ "<<__PRETTY_FUNCTION__<<"]" << " cannot create tree on \""<<chaosNode;
			return NULL;
		}

		do{
			for(std::vector<boost::shared_ptr<chaos::common::data::CDataWrapper> >::iterator i=res.begin();i!=res.end();i++){
				addTree(tree_ret,i->get());
			}
			if(ret>0){
				ret=ctrl.queryNext(ret,res);
			}
		}while(ret>0);
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
