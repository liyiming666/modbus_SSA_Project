/*
 * integrate.h
 *
 *  Created on: Sep 7, 2018
 *      Author: wenhan
 */

#ifndef INTEGRATE_H_
#define INTEGRATE_H_

#include <sragent.h>

class Integrate: public SrIntegrate
{
public:
        Integrate(): SrIntegrate() {}
        virtual ~Integrate() {}
        virtual int integrate(const SrAgent &agent,
                              const std::string &srv, const std::string &srt);
};

#endif /* INTEGRATE_H_ */
