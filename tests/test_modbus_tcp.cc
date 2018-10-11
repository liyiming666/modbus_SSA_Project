
#include <iostream>

#include "modbus.h"
#include "modbus-private.h"
#include "modbus-tcp-private.h"
#include "modbus-rtu-private.h"

using namespace std;

extern const modbus_backend_t _modbus_tcp_backend;
extern const modbus_backend_t _modbus_rtu_backend;

typedef uint8_t u8;

class Modbus{
public:
        Modbus();

        ~Modbus();

        int init(const string& mode,  const int& slave);

        int readCo(const int& addr, const int& nb, u8* req);
        int readDi(const int& addr, const int& nb, u8* req);
        int readHr(const int& addr, const int& nb, u8* req);
        int readIr(const int& addr, const int& nb, u8* req);

public:
        modbus_t* rtuMode();
        modbus_t* tcpMode();

private:
        std::string _mode;
        int _slave;
        modbus_t* _ctx;
};


modbus_t* newRtu()
{
        modbus_t* ctx = (modbus_t*)malloc(sizeof(modbus_t));
        _modbus_init_common(ctx);

        ctx->backend = &_modbus_rtu_backend;
        ctx->backend_data = (modbus_rtu_t *)malloc(sizeof(modbus_rtu_t));
        modbus_rtu_t *ctx_rtu = (modbus_rtu_t *)ctx->backend_data;

        ctx_rtu->confirmation_to_ignore = FALSE;

        modbus_set_slave(ctx, 1);

        uint8_t req[MODBUS_MAX_READ_BITS];
        int req_length = ctx->backend->build_request_basis(
                ctx, MODBUS_FC_READ_COILS, 0, 1, req);
        int ret =  ctx->backend->send_msg_pre(req, req_length);

        for (int i = 0; i < ret; i++) {
                printf("%02x ", req[i]);
        }
        printf("\n");




        return ctx;
}


Modbus::Modbus()
        : _ctx(NULL)
{

}

Modbus::~Modbus()
{
        cout << "~modbus" << endl;
        if (_ctx) {
                cout << "free _ctx" << endl;
                free(_ctx->backend_data);
                free(_ctx);
                _ctx->backend_data = NULL;
                _ctx = NULL;
                // modbus_close(_ctx);
                // modbus_free(_ctx);
                cout << "end free _ctx" << endl;

                _ctx = NULL;
                cout << "ctx is null" << endl;
        }
        cout << "end ~modbus" << endl;
}

int Modbus::init(const string& mode,  const int& slave)
{
        _mode = mode;
        _slave = slave;

//         if (_mode == "TCP") {
//                 _ctx = this->tcpMode();
//         } else if(_mode == "RTU") {
// //                _ctx = modbus_new_rtu("/dev/ttyS0", 9600, 'N', 8, 1);
//                 _ctx = this->rtuMode();
//         }

//         if (!_ctx) return -1;   // return -1 when alloc memory failed.

//         return modbus_set_slave(_ctx, _slave);
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
        return ret;
}

int Modbus::readDi(const int& addr, const int& nb, u8* req)
{
        int req_length = _ctx->backend->build_request_basis(
                _ctx, MODBUS_FC_READ_DISCRETE_INPUTS, addr-1, nb, req);
        int ret =  _ctx->backend->send_msg_pre(req, req_length);
        return ret;
}

int Modbus::readHr(const int& addr, const int& nb, u8* req)
{
        int req_length = _ctx->backend->build_request_basis(
                _ctx, MODBUS_FC_READ_HOLDING_REGISTERS, addr-1, nb, req);
        int ret =  _ctx->backend->send_msg_pre(req, req_length);
        return ret;
}

int Modbus::readIr(const int& addr, const int& nb, u8* req)
{
        int req_length = _ctx->backend->build_request_basis(
                _ctx, MODBUS_FC_READ_INPUT_REGISTERS, addr-1, nb, req);
        int ret =  _ctx->backend->send_msg_pre(req, req_length);
        return ret;
}






int main(void)
{
        Modbus mb;
        if (-1 == mb.init("RTU", 1)) return -1;
        // u8 req[MODBUS_MAX_READ_BITS];
        // int ret = mb.readCo(1, 1, req);



        return 0;
}
