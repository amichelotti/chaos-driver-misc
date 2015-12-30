/* 
 * File:   testDataSetAttribute.cpp
 * Author: michelo
 * test from UI
 * Created on September 10, 2015, 11:24 AM
 */

#include <cstdlib>
#include <chaos/ui_toolkit/ChaosUIToolkit.h>

#include "ChaosDatasetAttribute.h"
#include "ChaosDatasetAttributeGroup.h"
#include "ChaosController.h"
#include "ChaosControllerGroup.h"
#include "ChaosDatasetAttributeSinchronizer.h"
using namespace std;

/*
 * 
 */
using namespace driver::misc;
int main(int argc, char** argv) {
    chaos::ui::ChaosUIToolkit::getInstance()->init(argc, argv);
    ChaosDatasetAttribute op1("LIBERA01/LIBERA_ACQUIRE0/VA");
    ChaosDatasetAttribute op2("LIBERA01/LIBERA_ACQUIRE0/VB");
    ChaosDatasetAttribute op3("LIBERA02/LIBERA_ACQUIRE0/VA");
    ChaosController libera1("LIBERA01/LIBERA_ACQUIRE0");
    ChaosController libera2("LIBERA01/LIBERA_ACQUIRE1");
    
    ChaosControllerGroup<ChaosController> group;
    group.add(libera1);
    
    ChaosDatasetAttributeSinchronizer s;
    uint32_t size;
    int32_t *dd=  (int32_t*)op3.get(&size);
    s.add(&op1);
    s.add(&op2);
    s.add(&op3);

     std::cout<<"Get Time:"<<op1.getInfo().tget<<" TS:"<<op1.getInfo().tstamp<<" VA:"<<(int32_t)op1<<" VB:"<<(int32_t)op2<<" dd("<<size<<"):"<<dd[0]<<std::endl;
     
     std::cout<<"sync at:"<<s.sync()<<std::endl;;
    return 0;
}

