//
//  MessProfileTimeInfo.h
//  chaos-mess
//
//  Created by andrea michelotti on 08/05/14.
//  Copyright (c) 2014 INFN. All rights reserved.
//

#ifndef chaos_mess_MessProfilePacketInfo_h
#define chaos_mess_MessProfilePacketInfo_h
#include <stdint.h>

struct MessTimeProfile {
    uint64_t cmd_arrival_time_us;//! local absolute microsecods of command arrival
    uint64_t end_cycle_us; // !! local absolute time in microsecons of start of new repetition cycle
    uint64_t prepare_time_us; //! local microseconds of time to prepare trasmission
    double cycle_sec; //!! cycles per second

    
    double cycle_us;//! duration of the whole cycle average
    double net_cycle_us;//! duration of the whole cycle minus the time to prepare test buffer average
    double cycle_sigma;
        

};
struct MessProfilePacketInfo {
    uint32_t uidx; //!packet uid counter
    uint32_t buffsize; //!packet buffer size
    uint32_t crc32;//! buffer crc32
    MessTimeProfile tprof;
};


#endif
