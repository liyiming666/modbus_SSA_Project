/*
 * timer.cc
 *
 *  Created on: Sep 7, 2018
 *      Author: wenhan
 */


#include <unistd.h>
#include <iostream>

#include "timer.h"

using namespace std;

bool operator<=(const timespec &l, const timespec &r)
{
        return l.tv_sec == r.tv_sec ? l.tv_nsec <= r.tv_nsec : l.tv_sec <= r.tv_sec;
}

void Timer::loop()
{
        while (1) {
                const timespec ts = {val / 1000, 0};
                nanosleep(&ts, NULL);

                if (!this->isActive()) {
                    break;
                }

                this->run();
#if 0
		timespec now;
		clock_gettime(CLOCK_MONOTONIC_COARSE, &now);
		
		if (this->isActive()) {
			if (this->fireTime() <= now) {
				this->run();
	
				if (this->isActive()) {
					this->start();
				}
			}
		} else {		// 不活跃
			cout << "---------------_>thread exit" << endl;
			break;
		} 
#endif
        }
}


