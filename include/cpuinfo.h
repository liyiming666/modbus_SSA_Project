/*
 * cpu_info.h
 *
 *  Created on: Sep 7, 2018
 *      Author: wenhan
 */

#ifndef CPU_INFO_H_
#define CPU_INFO_H_

#include "sragent.h"
#include "srtimer.h"

class CpuInfo: public SrTimerHandler
{
public:
        CpuInfo();
        virtual ~CpuInfo();
        virtual void operator()(SrTimer &timer, SrAgent &agent);
};

#endif /* CPU_INFO_H_ */
