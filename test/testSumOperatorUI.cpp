/* 
 * File:   testDataSetAttribute.cpp
 * Author: michelo
 * test from UI
 * Created on September 10, 2015, 11:24 AM
 */

#include <cstdlib>
#include <ChaosMetadataServiceClient/ChaosMetadataServiceClient.h>

#include "ChaosDatasetAttribute.h"
#include "ChaosDatasetAttributeGroup.h"
#include "ChaosController.h"
#include "ChaosControllerGroup.h"
#include "ChaosDatasetAttributeSinchronizer.h"
using namespace std;
using namespace chaos::metadata_service_client;

/*
 * 
 */
using namespace driver::misc;
int main(int argc, char** argv) {



	ChaosMetadataServiceClient::getInstance()->init(argc,argv);
	std::vector<std::string> accum;
	std::vector<ChaosDatasetAttribute*> elems;
	std::vector<ChaosController*> ctrls;

	int cnt=0;
	 accum.push_back("ACCUMULATOR/BPM/BPSA2001");
	 accum.push_back("ACCUMULATOR/BPM/BPBA1002");
	 accum.push_back("ACCUMULATOR/BPM/BPBA2001");
	 accum.push_back("ACCUMULATOR/BPM/BPBA4001");
	 accum.push_back("ACCUMULATOR/BPM/BPBA4002");
	 accum.push_back("ACCUMULATOR/BPM/BPBA3001");
	 accum.push_back("ACCUMULATOR/BPM/BPBA3002");
	 accum.push_back("ACCUMULATOR/BPM/BPBA2002");
	 accum.push_back("ACCUMULATOR/BPM/BPSA4001");
	 accum.push_back("ACCUMULATOR/BPM/BPSA1001");
	 accum.push_back("ACCUMULATOR/BPM/BPBA1001");
	 accum.push_back("ACCUMULATOR/BPM/BPSA3001");
	 accum.push_back("ACCUMULATOR/BPM/BPSA2001");
	 accum.push_back("ACCUMULATOR/BPM/BPBA1002");
	 accum.push_back("ACCUMULATOR/BPM/BPBA2001");
	 accum.push_back("ACCUMULATOR/BPM/BPBA4001");
	 accum.push_back("ACCUMULATOR/BPM/BPBA4002");
	 accum.push_back("ACCUMULATOR/BPM/BPBA3001");
	 accum.push_back("ACCUMULATOR/BPM/BPBA3002");
	 accum.push_back("ACCUMULATOR/BPM/BPBA2002");
	 accum.push_back("ACCUMULATOR/BPM/BPSA4001");
	 accum.push_back("ACCUMULATOR/BPM/BPSA1001");
	 accum.push_back("ACCUMULATOR/BPM/BPBA1001");
	 accum.push_back("ACCUMULATOR/BPM/BPSA3001");
	 ChaosControllerGroup<ChaosController> group;
	 ChaosDatasetAttributeSinchronizer s;

	 for(std::vector<std::string>::iterator i=accum.begin();i!=accum.end();i++){
		 ChaosDatasetAttribute*d =new ChaosDatasetAttribute(*i + "/VA");
		 elems.push_back(d);
		 ChaosController *p=new ChaosController(*i);
		 ctrls.push_back(p);
		 group.add(p);
		 s.add(d);
	 }
    
	 for(int i=0;i<accum.size();i++){
	     std::cout<<"Get Time:"<<elems[i]->getInfo().tget<<" TS:"<<elems[i]->getInfo().tstamp<<" VA:"<<(int32_t)*elems[i]<<std::endl;

	 }


     
     std::cout<<"sync at:"<<s.sync()<<std::endl;;
    return 0;
}

