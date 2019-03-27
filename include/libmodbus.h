/*
 * libmodbus.h
 *
 *  Created on: Sep 7, 2018
 *      Author: wenhan
 */

#ifndef LIBMODBUS_H_
#define LIBMODBUS_H_

#include <string>

#include "define.h"
#include "modbus.h"
#include "modbus-private.h"

#include "util.h"
#include "ssa_logger.h"

class Modbus{
public:
        Modbus();

        ~Modbus();

        int init(const string& mode,  const int& slave);
	
	int readBat(const int& addr, const int& nb, u8* req, int func_code);

        int writeCo(int num, int nb, u8* req);
 		int writeHr(int num, int nb, u8* req);

public:
        modbus_t* rtuMode();
        modbus_t* tcpMode();

public:
        void print(u8* req, const int& len) { 
		SSADebug(getHexString(req, len)); 
	}
private:
        std::string _mode;
        int _slave;
        modbus_t* _ctx;
};

#endif /* LIBMODBUS_H_ */
