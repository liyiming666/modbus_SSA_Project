/*
 * modbus_p.h
 *
 *  Created on: Sep 7, 2018
 *      Author: wenhan
 */

#ifndef MODBUS_P_H_
#define MODBUS_P_H_

#include "define.h"
#include "client_session.h"

class ClientSession;

class ModbusP {
public:
        ModbusP(u8** data, u8**pos, ClientSession* cs);
        virtual ~ModbusP();

public:
        /* Check the message's type and length */
        virtual int checkMsg();
        virtual int parseMsg();
        virtual unsigned long getBuf();

        /* Check if the packet is a heartbeat packet  */
        int checkHB();

        /* Check if the packet is a modbus packet */
        int checkMP(const u32& len);

public:
//        std::string parseMsg();

        int parseCO();
        int parseDI();
        int parseHR();
        int parseIR();
public:
        void init();
        virtual int checkReg();

private:
        u8** _d;    // Secondary pointer, same as _data in the session object
        u8** _pos;
        u8* _data;
        ClientSession* _cs;
        Bool _tcp_flag;
};

#endif /* MODBUS_P_H_ */
