//
//  CmdCalcTrxDelay.cpp
//  chaos-mess
//
//  Created by Claudio Bisegni on 27/02/14.
//  Copyright (c) 2014 INFN. All rights reserved.
//

#include "CmdCalcBandwidth.h"
#include <boost/crc.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/random/random_device.hpp>

using namespace chaos;

using namespace chaos::common::data;

using namespace chaos::cu::control_manager::slow_command;
namespace chaos_batch = chaos::common::batch_command;


CmdCalcBandwidth::CmdCalcBandwidth()  {
  buffer = NULL;
  size =0;
  repeat=0;
  counter=0;
  start = 0;
  samples=0;
  repetition_stats=NULL;
  setFeatures(chaos_batch::features::FeaturesFlagTypes::FF_SET_SCHEDULER_DELAY, (uint64_t)0);

}

CmdCalcBandwidth::~CmdCalcBandwidth() {
  /*if(buffer) free(buffer);
    buffer = 0;*/
  if(repetition_stats){
    free(repetition_stats);
    repetition_stats=NULL;
  }

}

// return the implemented handler
uint8_t CmdCalcBandwidth::implementedHandler() {
  return  chaos_batch::HandlerType::HT_Set | chaos_batch::HandlerType::HT_Acquisition;

}

// Start the command execution
void CmdCalcBandwidth::setHandler(CDataWrapper *data) {

  if(!data) {
    std::cout<<"## empty data"<<std::endl;
    return;
  }

  hdr = getAttributeCache()->getRWPtr<MessProfilePacketInfo>(DOMAIN_OUTPUT, "profile");
  memset((void*)hdr,0,sizeof(hdr));

  boost::posix_time::ptime time = boost::posix_time::microsec_clock::local_time();
  hdr->tprof.cmd_arrival_time_us = time.time_of_day().total_microseconds();
  ts_tag = data->getUInt64Value("ts_tag");
  size = data->getUInt32Value(CmdCalcBandwidth_BYTES_PARAM_KEY);
  //enalrge cache
  getAttributeCache()->setOutputAttributeNewSize("buffer", size);

  hdr->buffsize = size;
  hdr->uidx= 0;
  hdr->ts_tag = ts_tag;
  hdr->tprof.cycle_sigma=0;
  buffer = getAttributeCache()->getRWPtr<uint8_t>(DOMAIN_OUTPUT, "buffer");
  repeat = data->getUInt32Value(CmdCalcBandwidth_REPEAT_PARAM_KEY);
  repetition_stats=(int64_t*)realloc(repetition_stats,repeat*sizeof(int64_t));
  assert(repetition_stats);
  LAPP_<<"=== Start Command bytes:"<<size<<" repeat:"<<repeat<<endl;
  getAttributeCache()->setOutputAttributeNewSize("buffer",size);
  assert(buffer);
  //start =1;
  counter = 0;
  oldtime =0 ;
  samples=0;
  avg_cycle =0;
  avg_net_cycle=0;
  getAttributeCache()->setOutputDomainAsChanged();
  // initialize counter
  // acquiredData->addBinaryValue("profile", (char*)&hdr, sizeof(hdr));
  //    pushDataSet(acquiredData);
  //  usleep(500000); // be sure ui is in the main loop



}

void CmdCalcBandwidth::acquireHandler() {

  if((counter<repeat)) {
    //		r_lock->lock();

    st_prep= boost::posix_time::microsec_clock::local_time();
    st_time=st_prep.time_of_day().total_microseconds();
    hdr->uidx=counter;
    if(counter==0){
      start_cycle=st_time;
    }
#ifdef CHECK_BUFFER
    boost::crc_32_type crc_calc;
    boost::random::random_device r;
    r.generate(buffer, buffer+size);
    crc_calc.process_bytes(buffer, size);
    hdr->crc32 =crc_calc.checksum();
#endif
    end_prep= boost::posix_time::microsec_clock::local_time();
    prep_time = ( end_prep - st_prep).total_microseconds();
    hdr->tprof.prepare_time_us = prep_time;
    if(oldtime>0){


      uint64_t tm=st_time - oldtime;
      avg_cycle += tm;
      repetition_stats[samples]=tm;
      samples++;
      avg_net_cycle +=tm - prep_time;

      LDBG_<<"counter:"<<counter<<" cycle avg:"<<hdr->tprof.cycle_us;
      if(counter == (repeat-1)){

	if(samples>0){
	  int cnt;
	  uint64_t tot_time=end_prep.time_of_day().total_microseconds() -start_cycle;                            double val=0;

	  hdr->tprof.net_cycle_us=avg_net_cycle/samples;
	  hdr->tprof.cycle_us=(double)avg_cycle/samples;
	  for(cnt=0;cnt<samples;cnt++){
	    val+=pow(((double)repetition_stats[cnt] - hdr->tprof.cycle_us),2);
	    //	    LDBG_<<" val:"<<repetition_stats[cnt];
	  }

	  val=val/samples;
	  hdr->tprof.cycle_sigma=sqrt(val);
	  hdr->tprof.cycle_sec=repeat*1000000.0/tot_time;

	  LAPP_<<"=== [tag:"<<ts_tag<<"] End Avg cycle :"<<hdr->tprof.cycle_us<<" Sigma:"<<hdr->tprof.cycle_sigma<<" cycle/s "<<hdr->tprof.cycle_sec<<" tot us "<<avg_cycle <<" tot bytes "<<size*repeat;

	}
      }

    }
    oldtime=st_time;
    hdr->tprof.end_cycle_us=end_prep.time_of_day().total_microseconds();
    getAttributeCache()->setOutputDomainAsChanged();

    //	r_lock->unlock();
    //force changed domain
  } else {
    /*   int cnt;
	 double val=0;
	 if(samples>0){
	 for(cnt=0;cnt<samples;cnt++){
	 val+=pow((double)(repetition_stats[cnt] - (int64_t)hdr->tprof.cycle_us),2);
	 }
	 val=val/samples;
	 hdr->tprof.cycle_sigma=sqrt(val);
	 // LAPP_<<" Avg:"<<hdr->tprof.cycle_us<<" Sigma:"<<hdr->tprof.cycle_sigma;

	 }*/
    BC_END_RUNNIG_PROPERTY;

  }
  counter++;

}

