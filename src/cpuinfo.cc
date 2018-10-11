/*
 * cpu_info.cc
 *
 *  Created on: Sep 7, 2018
 *      Author: wenhan
 */

#include <string.h>
#include <string>
#include <iostream>
#include <srlogger.h>
#include "cpuinfo.h"
#include "util.h"
using namespace std;

CpuInfo::CpuInfo()
{

}

CpuInfo::~CpuInfo()
{

}

void CpuInfo::operator()(SrTimer &timer, SrAgent &agent)
{
        agent.send("107," + agent.ID());
//		+ "450,136464732," + getServerTime());
/*
 	int cpu_info = 0;
    if (timer.isActive())
            if (getSystemLoad(cpu_info))
                    agent.send("107," + agent.ID() + "\n"
                               + "401," + agent.ID() + "," + to_string(cpu_info));
*/
}


