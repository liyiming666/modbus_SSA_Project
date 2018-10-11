/*
 * client_session.h
 *
 *  Created on: Sep 7, 2018
 *      Author: wenhan
 */

#ifndef CLIENT_SESSION_H_
#define CLIENT_SESSION_H_

#pragma once

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <mutex>
#include <pthread.h>
#include <string>
#include <srnethttp.h>
#include <stdio.h>

#include "srlogger.h"

#include "define.h"
#include "util.h"
#include "map.h"
#include "timer.h"

#include "modbus_p.h"
#include "modbus_model.h"
#include "c8y.h"

using namespace std;
using boost::asio::ip::tcp;
using boost::asio::ip::address;
using boost::asio::deadline_timer;

class ModbusP;
class C8yModel;

class ClientSession : public TimerHandler,
                      public boost::enable_shared_from_this<ClientSession>

{
public:
        /* 构造函数：创建与设备的通讯 */

        ClientSession(SrAgent& agent,
                      boost::asio::io_service& ioservice,
                      Map<boost::thread::id, SrNetHttp*>& https_pool,
                      const int& tms);

        /* 析构函数 */
        ~ClientSession();

        /* 获得连接客户端的socket */
        tcp::socket& socket() { return _sock; }

        /* Get current DTU's sid */
        string sid() { return _sid; }

        /* Get current DTU's did */
        string& did() { return _did; }
	
	std::mutex& getMutex() { return _read_mutex; }

	boost::asio::io_service& ioService() { return _io_service; }

        /* Set heartbeat timing */
        inline void setTimer() {
                // 设置recv 超时时间，此处会触发 check_deadline
                _deadline.expires_from_now(boost::posix_time::seconds(_tms));
                _deadline.async_wait(boost::bind(&ClientSession::checkDeadline,
                                                 shared_from_this(),
                                                 boost::asio::placeholders::error));
        }

        /* Detect heartbeat */
        void checkDeadline(const boost::system::error_code& error);

        /* Save DTU device's socket-info in this session */
        void setClientInfo() {
                boost::asio::ip::tcp::endpoint remote_ep = _sock.remote_endpoint();
                boost::asio::ip::address remote_ad = remote_ep.address();
                _c_ip = remote_ad.to_string();
                _c_port = remote_ep.port();
        }

        /* Get DTU device's socket-info from this session */
        string getClientInfo() const {
                return _c_ip + ":" + to_string(_c_port);
        }

        /* Close socket and clear all global structures */
        void Close(Map<string, ClientSession*>& did_to_cs);

        void init();

        /* 异步读取数据 */
        void read(const unsigned long& len);

        /* Register to read the asynchronous event of the registration package */
        void readReg(const unsigned long& len);

        /* Register an asynchronous event that sends data */
        void write(u8* w_buf, const u32& len);

        /* Set the operation-id for the current DTU device's operation */
        void setOptId(const string& opt_id) { _opt_id = opt_id; }

public:
        /* Pthread callback function */
        static void* func(void* arg);

        /* Timer-based timing overload function, executed by pthread,
           implements polling request modbus data
        */
        virtual void operator()(Timer &timer);

private:
        /* Callback function to read the asynchronous event of the
           registration package
        */
        void onReadReg(const boost::system::error_code& error,
                       const size_t& bytes_transferred);

        /* Callback function for reading asynchronous events of packets
           and heartbeat packets
        */
        void onRead(const boost::system::error_code& error,
                    const size_t& bytes_transferred);

        /* Callback function for asynchronously sending events */
        void onWrite(const boost::system::error_code& error,
                     const size_t& bytes_transferred);

        /* Parsing the registration package */
        u32 parseRegPkg();

        /* Regist the device from Quark Platform */
        Bool regDev();

public:
        SrAgent& _agent;

        SrNetHttp* _http;

        std::string _did;
        std::string _sid;

        pthread_t _tid;         // thread id

        C8yModel* _c8y;         // communicate with platform
        Timer _timer;           // thread's timer

        MBDEVICES _mbdevices;   // Device model in modbus database

        /*
          After receiving the data, according to the corresponding relationship,
          find the sid of the device to be uploaded.
          Differentiate according to different protocols.
        */
        Map<std::string, std::string> _tcp_slave_to_sid;
        Map<std::string, std::string> _rtu_slave_to_sid;

        Map<boost::thread::id, SrNetHttp*>& _https_pool;

        std::string _current_num; // Currently processed address
        std::string _current_type; // Currently processed model
        std::string _current_sid;  // Currently processed childDevice's sid

private:
        tcp::socket _sock;
        boost::asio::io_service &_io_service;
        string _c_ip;
        u16 _c_port;
        u8* _data;
        u8* _data_pos;
        string _opt_id;

        u32 _conn_status;     // 0:connected;1:ready to disconnect;2:disconnected

        ModbusP* _modbus_p;

        Bool _stopped;
        deadline_timer _deadline;
        std::mutex _io_mutex;
        std::mutex _mb_mutex;
        std::mutex _read_mutex;
        int _timer_count;
        const int& _tms;

        Bool _mb_flag;          // TRUE:allow write;  FALSE:allow read
        Bool _poll_flag;        // TRUE:allow poll ;  FALSE:allow handle

        u8 _read_num;
        u8 _write_num;

        int _polling_rate;
        int _transmit_rate;

        Bool _read_flag;
        Bool _write_flag;
};

#endif /* CLIENT_SESSION_H_ */
