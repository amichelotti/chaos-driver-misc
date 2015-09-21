/* 
 * File:   testDataSetAttribute.cpp
 * Author: michelo
 * test from UI
 * Created on September 10, 2015, 11:24 AM
 */

#include <cstdlib>
#include <chaos/ui_toolkit/ChaosUIToolkit.h>

#include "ChaosDatasetAttribute.h"
using namespace std;

/*
 * 
 */
int main(int argc, char** argv) {
    chaos::ui::ChaosUIToolkit::getInstance()->init(argc, argv);
    ChaosDatasetAttribute<double> op1("/LIBERA01/LIBERA_ACQUIRE0/VA");
     ChaosDatasetAttribute<double> op2("/LIBERA01/LIBERA_ACQUIRE0/VB");
    return 0;
}

