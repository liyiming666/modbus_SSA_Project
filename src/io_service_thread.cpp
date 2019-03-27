/*
 * io_service_thread.cc
 *
 *  Created on: Sep 7, 2018
 *      Author: wenhan
 */

#include "io_service_thread.h"
#include "srlogger.h"
#include "ssa_logger.h"
#include <iostream>
#include <mutex>

#include "map.h"
#include "srnethttp.h"

using namespace std;

ThreadPool::ThreadPool(size_t size, SrAgent& agent, const string& srv)
        : _work(_io_service)
{
        for (size_t i = 0; i < size; ++i) {
                SrNetHttp* http = new SrNetHttp(agent.server() + "/s", srv, agent.auth());
                boost::thread::id t_id =
                        _workers.create_thread(
                                boost::bind(&boost::asio::io_service::run, &_io_service))->get_id();
                _thread_https_pool[t_id] = http;
        }

        boost::thread::id t_id = boost::this_thread::get_id();
        _thread_https_pool[t_id] = new SrNetHttp(agent.server() + "/s", srv,
                agent.auth());
}

ThreadPool::~ThreadPool()
{
        _io_service.stop();
        _workers.join_all();
}

//Add new work item to the pool.
template<class F>
void ThreadPool::Enqueue(F f)
{
        _io_service.post(f);
}

boost::asio::io_service& ThreadPool::getIoService()
{
        return _io_service;
}

IOServiceThread::IOServiceThread(SrAgent& agent, const int& port, SrNetHttp* http,
        const string& srv, const int& tms)
        : _tp(2, agent, srv), _agent(agent), _endpoint(tcp::v4(), port), _acceptor(
                _tp.getIoService(), _endpoint), _http(http), _tms(tms)
{

        StartHandleAccept();
}

IOServiceThread::~IOServiceThread()
{

}

void IOServiceThread::StartHandleAccept()
{
        session_ptr new_session(
                new ClientSession(_agent, _tp.getIoService(), _tp.getHttpsPool(), _tms));
        _acceptor.async_accept(new_session->socket(),
                boost::bind(&IOServiceThread::ProcessAccept, this,
                        boost::asio::placeholders::error, new_session));
}

void IOServiceThread::Start()
{
//	_io_service_pool.start();
//	_io_service_pool.join();
}

void IOServiceThread::ProcessAccept(const boost::system::error_code& error,
        const session_ptr& session)
{
        if (!error) {
                try {
                        SSAInfo("a client has been connected");
                        session->setClientInfo();
                        SSAInfo(session->getClientInfo());
                        session->init();
                        session_ptr new_session(
                                new ClientSession(_agent, _tp.getIoService(),
                                        _tp.getHttpsPool(), _tms));
                        _acceptor.async_accept(new_session->socket(),
                                boost::bind(&IOServiceThread::ProcessAccept, this,
                                        boost::asio::placeholders::error, new_session));
                } catch (boost::system::system_error const& e) {
                        SSAInfo("Warning: could not connect : " + string(e.what()));
                }
        } else {
                SSADebug("connection error");
        }
}

