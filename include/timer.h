/*
 * timer.h
 *
 *  Created on: Sep 7, 2018
 *      Author: wenhan
 */

#ifndef TIMER_H_
#define TIMER_H_

#include <string>
#include <time.h>

class Timer;

bool operator<=(const timespec &l, const timespec &r);

class TimerHandler
{
public:
        virtual ~TimerHandler() {

        }

        virtual void operator()(Timer &timer) = 0;
};


class Timer
{
public:
        Timer(int millisec, TimerHandler *callback = NULL) :
                cb(callback), val(millisec), active(false) {

        }

        virtual ~Timer() {

        }

        bool isActive() const {
                return active;
        }

        int interval() const {
                return val;
        }

        const timespec &shedTime() const {
                return beg;
        }

        const timespec &fireTime() const {
                return end;
        }

        void run() {
                if (cb) {
                        (*cb)(*this);
                }
        }

        void setInterval(int millisec) {
                val = millisec;
        }

        void connect(TimerHandler *functor) {
                cb = functor;
        }

        void start() {
                clock_gettime(CLOCK_MONOTONIC_COARSE, &beg);
                end.tv_sec = beg.tv_sec + val / 1000;
                end.tv_nsec = beg.tv_nsec + (val % 1000) * 1000000;

                if (end.tv_nsec >= 1000000000)
                {
                        ++end.tv_sec;
                        end.tv_nsec -= 1000000000;
                }

                active = true;
        }

        void stop() {
                active = false;
        }

        void loop();

private:
        TimerHandler *cb;
        timespec beg;
        timespec end;
        int val;
        bool active;
};

#endif /* TIMER_H_ */
