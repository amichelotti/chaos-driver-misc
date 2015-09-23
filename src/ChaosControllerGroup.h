/* 
 * File:   ChaosControllerGroupSyncronizer.h
 * Author: michelo
 *
 * Created on September 2, 2015, 5:29 PM
 */

#ifndef ChaosControllerGroupSyncronizer_H
#define	ChaosControllerGroupSyncronizer_H
#include <map>
#include <string>
#include "ChaosController.h"

class ChaosControllerGroup:public ChaosController{
    
    typedef std::vector<ChaosController*> ccgrp_t;
    
    ccgrp_t group;
public:

    ChaosControllerGroup();
    virtual ~ChaosControllerGroup();
    /**
     * add an existing opject
     * 
     */
    void add(ChaosController&);   
    virtual int init();
    virtual int stop();
    virtual int start();
    virtual int deinit();
    /**
     * 
     * @return the state of the group, negative if the states are different
     */
    virtual int getState();
    virtual uint64_t getTimeStamp();
    virtual int executeCmd(command_t& cmd,bool wait);
};

#endif	/* ChaosControllerGroupSyncronizer_H */

