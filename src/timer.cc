/*
 * timer.cc
 *
 *  Created on: Sep 7, 2018
 *      Author: wenhan
 */


#include <unistd.h>

#include "timer.h"

bool operator<=(const timespec &l, const timespec &r)
{
        return l.tv_sec == r.tv_sec ? l.tv_nsec <= r.tv_nsec : l.tv_sec <= r.tv_sec;
}

void Timer::loop()
{
        while (1) {
                const timespec ts = {val / 1000, 0};
                nanosleep(&ts, NULL);
                if (cb) {
                        (*cb)(*this);
                }
        }
}


