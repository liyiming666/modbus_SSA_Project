/*
 * libmodbus.cc
 *
 *  Created on: Sep 7, 2018
 *      Author: wenhan
 */


#include "modbus-tcp-private.h"
#include "modbus-rtu-private.h"

#include "libmodbus.h"

using namespace std;

extern const modbus_backend_t _modbus_tcp_backend;
extern const modbus_backend_t _modbus_rtu_backend;

Modbus::Modbus()
        : _ctx(NULL)
{

}

Modbus::~Modbus()
{
        if (_ctx) {
                free(_ctx->backend_data);
                free(_ctx);
                _ctx->backend_data = NULL;
                _ctx = NULL;
        }
}

int Modbus::init(const string& mode,  const int& slave)
{
        _mode = mode;
        _slave = slave;

        if (_mode == "TCP") {
                _ctx = this->tcpMode();
        } else if(_mode == "RTU") {
//                _ctx = modbus_new_rtu("/dev/ttyS0", 9600, 'N', 8, 1);
                _ctx = this->rtuMode();
        }

        if (!_ctx) return -1;   // return -1 when alloc memory failed.

        return modbus_set_slave(_ctx, _slave);
}

modbus_t* Modbus::tcpMode()
{
        modbus_t* ctx = (modbus_t*)malloc(sizeof(modbus_t));

        if (!ctx) return (modbus_t*)NULL;

        _modbus_init_common(ctx);
        ctx->slave = MODBUS_TCP_SLAVE;
        ctx->backend = &_modbus_tcp_backend;
        ctx->backend_data = (modbus_tcp_t *)malloc(sizeof(modbus_tcp_t));

        return ctx;
}

modbus_t* Modbus::rtuMode()
{
        modbus_t* ctx = (modbus_t*)malloc(sizeof(modbus_t));

        if (!ctx) return (modbus_t*)NULL;

        _modbus_init_common(ctx);
        ctx->backend = &_modbus_rtu_backend;
        ctx->backend_data = (modbus_rtu_t *)malloc(sizeof(modbus_rtu_t));

        return ctx;
}

int Modbus::readCo(const int& addr, const int& nb, u8* req)
{
        int req_length = _ctx->backend->build_request_basis(
                _ctx, MODBUS_FC_READ_COILS, addr-1, nb, req);
        int ret =  _ctx->backend->send_msg_pre(req, req_length);
        this->print(req, ret);
        return ret;
}

int Modbus::readDi(const int& addr, const int& nb, u8* req)
{
        int req_length = _ctx->backend->build_request_basis(
                _ctx, MODBUS_FC_READ_DISCRETE_INPUTS, addr-1, nb, req);
        int ret =  _ctx->backend->send_msg_pre(req, req_length);
        this->print(req, ret);
        return ret;
}

int Modbus::readHr(const int& addr, const int& nb, u8* req)
{
        int req_length = _ctx->backend->build_request_basis(
                _ctx, MODBUS_FC_READ_HOLDING_REGISTERS, addr-1, nb, req);
        int ret =  _ctx->backend->send_msg_pre(req, req_length);
        this->print(req, ret);
        return ret;
}

int Modbus::readIr(const int& addr, const int& nb, u8* req)
{
        int req_length = _ctx->backend->build_request_basis(
                _ctx, MODBUS_FC_READ_INPUT_REGISTERS, addr-1, nb, req);
        int ret =  _ctx->backend->send_msg_pre(req, req_length);
        this->print(req, ret);
        return ret;
}

int Modbus::writeCo(int num, int nb, u8* req)
{
 		int ret = _ctx->backend->build_request_basis(_ctx, MODBUS_FC_WRITE_SINGLE_COIL, num-1, nb?0xFF00:0, req);
        	ret =  _ctx->backend->send_msg_pre(req, ret);
        this->print(req, ret);
		return ret;
}

int Modbus::writeHr(int num, int nb, u8* req)
{
 		int ret = _ctx->backend->build_request_basis(_ctx, MODBUS_FC_WRITE_SINGLE_REGISTER, num-1, nb, req);
        	ret =  _ctx->backend->send_msg_pre(req, ret);
        this->print(req, ret);
 		return ret;
}


