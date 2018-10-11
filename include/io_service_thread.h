/*
 * io_service_thread.h
 *
 *  Created on: Sep 7, 2018
 *      Author: wenhan
 */

#ifndef IO_SERVICE_THREAD_H_
#define IO_SERVICE_THREAD_H_

#include "client_session.h"
#include "sragent.h"
#include <srnethttp.h>
#include <thread>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <stdexcept>
#include <boost/lexical_cast.hpp>
#include "map.h"

using boost::asio::ip::tcp;

class ClientSession;

class ThreadPool {
public:
        explicit ThreadPool(size_t size, SrAgent& agent, const string& srv);

        ~ThreadPool();

        //Add new work item to the pool.
        template<class F>
        void Enqueue(F f);

        boost::asio::io_service& getIoService();

        Map<boost::thread::id, SrNetHttp*>& getHttpsPool() {
                return _thread_https_pool;
        }

private:
        boost::thread_group _workers;
        boost::asio::io_service _io_service;
        boost::asio::io_service::work _work;
        Map<boost::thread::id, SrNetHttp*> _thread_https_pool;
};
class IOServiceThread
{
        typedef boost::shared_ptr<ClientSession> session_ptr;	//智能指针
public:
        /* IOServiceThread构造函数 */
        IOServiceThread(SrAgent& agent, const int& port, SrNetHttp* http,
                        const string& srv, const int& tms);

        /* IOServiceThread析构函数 */
        ~IOServiceThread();

        /* 启动io_service */
        void StartHandleAccept();
        //void StartIOService();

        /* 接收设备的连接 */
        void ProcessAccept(const boost::system::error_code& error,
                           const session_ptr& session);

        void Start();

private:
//	io_service_pool _io_service_pool;
        ThreadPool _tp;
        SrAgent& _agent;
        //boost::asio::io_service _io_service; 	//创建boost::asio::io_service
        tcp::endpoint _endpoint;
        tcp::acceptor _acceptor;
        std::thread _t;
        //const int& _nThreads;
        std::vector<boost::shared_ptr<boost::thread>> m_listThread;
        SrNetHttp* _http;
        const int& _tms;
};

#endif /* IO_SERVICE_THREAD_H_ */
