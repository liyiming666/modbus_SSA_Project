/*
 * client_session.cc
 *
 *  Created on: Sep 7, 2018
 *      Author: wenhan
 */

#include <iostream>
#include <boost/thread/mutex.hpp>
#include <string.h>
#include <mutex>
#include <srlogger.h>
#include <srutils.h>

#include "define.h"
#include "ssa_logger.h"
//#include "map.h"

#include "libmodbus.h"
#include "srlogger.h"
#include "client_session.h"

using namespace std;

Map<string, ClientSession*> did_to_cs;
Map<string, ClientSession*> sid_to_cs;

extern MBTYPES mbtypes;

using namespace SSADefine;

ClientSession::ClientSession(SrAgent& agent,
                             boost::asio::io_service& ioservice,
                             Map<boost::thread::id, SrNetHttp*>& https_pool,
                             const int& tms)
        : _agent(agent), _http((SrNetHttp*)NULL), _did(""), _sid(""), _tid(0),
          _c8y(new C8yModel(agent, &_http)), _timer(5*1000, this),
          _mbdevices(agent), _https_pool(https_pool), _sock(ioservice),
          _io_service(ioservice), _c_ip(""), _c_port(0), _conn_status(CONNECTED),
          _modbus_p(new ModbusP(&_data, &_data_pos, this)),
          _stopped(FALSE), _deadline(ioservice), _timer_count(0), _tms(tms),
          _mb_flag(TRUE), _poll_flag(TRUE), _read_num(0), _write_num(0),
          _polling_rate(5), _transmit_rate(100), _read_flag(TRUE), _write_flag(TRUE)
{
        _data = new u8[1024];
        _data_pos = _data;

        _modbus_p->init();

        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL); //允许退出线程
        pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL); //设置立即取消
}

ClientSession::~ClientSession()
{
        cout << "---> ~ClientSession" << endl;
        if (_data) {
                SSADebug(_did + "---> delete data buffer: " + this->getClientInfo());
                SSADebug(_did + ": map size: " + to_string(did_to_cs.size()));
                delete _data;
                _data = nullptr;
                _data_pos = nullptr;
                _deadline.cancel();
                //pthread_exit(&retval);
        }
        delete _modbus_p;
        delete _c8y;
}

void*  ClientSession::func(void* arg)
{
        ClientSession* cs = (ClientSession*)arg;
        cs->_timer.start();
        cs->_timer.loop();

        return (void*)NULL;
}

void ClientSession::checkDeadline(const boost::system::error_code& error)
{
        _io_mutex.lock();
        if (!error) {
                if (_timer_count == 1) {
                        _timer_count = 0;
                        this->setTimer();
                } else {
                        if (!_stopped) { // check 是存已标识并等待程序退出
                                SSAInfo(_did + " check_deadline and socket close");
                                this->Close(did_to_cs);
                                // 定时器 设定为永不超时/不可用状态
                                _deadline.expires_at(boost::posix_time::pos_infin);
                        }
                }
        }
        _io_mutex.unlock();
}

void ClientSession::Close(Map<string, ClientSession*>& did_to_cs)
{
        if (!_stopped) {
                if (!_did.empty()) did_to_cs.erase(_did, this);
                if (!_sid.empty()) sid_to_cs.erase(_sid, this);
                SSAInfo(_did + " Close socket: ip: " + this->getClientInfo());
                if (_sock.is_open()) {
                        try {
                                if (_conn_status == CONNECTED) {
                                        boost::system::error_code ec;
                                        _sock.cancel(ec);
                                        if (!ec) {
                                                _sock.shutdown(tcp::socket::shutdown_both);

                                                _sock.close();
						pthread_cancel(_tid);
						int retval;
        				        pthread_join(_tid, (void**)&retval);
                                        }
                                        _conn_status = DISCONNECTED;
                                } else {
                                        SSAInfo(_did + " _connect_status is DISCONNECTED");
                                }
                        } catch (std::exception& e) {
                                SSAInfo("--->exception:" + _did + " " + e.what());
                        }
                }
                _stopped = TRUE;
        }
}

void ClientSession::readReg(const unsigned long& len)
{
        _timer_count = 1;
        _sock.async_read_some(
                boost::asio::buffer(_data_pos, len),
                boost::bind(&ClientSession::onReadReg,
                            shared_from_this(),
                            boost::asio::placeholders::error,
                            boost::asio::placeholders::bytes_transferred));
}

void ClientSession::onReadReg(const boost::system::error_code& error,
                              const size_t& bytes_transferred)
{
        _io_mutex.lock();
        u32 is_close = FALSE;

        if (error) {    // 设备主动关闭或socket被取消
                SSAWarn(_did + ": read reg error and socket close");
                is_close = TRUE;
        } else {

                if (_stopped) {
                        SSADebug("---->" + _did + " timeout handled...!");
                } else {

                        if (!_did.empty() && (did_to_cs[_did] != this)) {
                                is_close = TRUE;
                        } else {
                                boost::thread::id t_id =
                                        boost::this_thread::get_id();
                                _http = _https_pool[t_id];

                                if (_http) {
                                        _data_pos += bytes_transferred;
                                        int len = _modbus_p->checkReg();

                                        if (len > 0) { // not enough reg package
                                                readReg(len);
                                        } else if (len < 0) { // error
                                                is_close = TRUE;
                                        } else { // regist package is correct
                                                if (!this->regDev()) { // fail
                                                        is_close = TRUE;
                                                } else { // successful
                                                        _c8y->getChildDevices(_sid, this);
                                                        memset(_data, 0, BUFSIZE);
                                                        _data_pos = _data;
                                                        read(3);
                                                }
                                        }
                                }
                                _http = (SrNetHttp*)NULL;
                        }
                }
        }

        if (is_close) {
                this->Close(did_to_cs);
        }
        _io_mutex.unlock();
}

void ClientSession::init()
{
        int ret = pthread_create(&_tid, NULL, func, this);
        if (ret) {
                SSAError("create thread fail...");

        }

        this->setTimer();
        this->readReg(19);
}

void ClientSession::read(const unsigned long& len)
{
	_read_mutex.lock();	
        _timer_count = 1;
        _sock.async_read_some(
                boost::asio::buffer(_data_pos, len),
                boost::bind(&ClientSession::onRead,
                            shared_from_this(),
                            boost::asio::placeholders::error,
                            boost::asio::placeholders::bytes_transferred));
}

void ClientSession::onRead(const boost::system::error_code& error,
                           const size_t& bytes_transferred)
{
	_read_mutex.unlock();
        _io_mutex.lock();
        u32 is_close = FALSE;
        if (error) {    // 设备主动关闭或socket被取消
                SSAWarn(_did + ": read error and socket close");
                is_close = TRUE;
        } else {
        		_read_flag = TRUE;

                if (_stopped) {
                        SSADebug("---->" + _did + " timeout handled...!");
                } else {
                        if (!_did.empty() && (did_to_cs[_did] != this)) {
                                is_close = TRUE;
                        } else {
                                boost::thread::id t_id =
                                        boost::this_thread::get_id();
                                _http = _https_pool[t_id];
                                if (_http) {
                                        _data_pos += bytes_transferred;
                                        int len = _modbus_p->checkMsg();


                                        if (len > 0) {
                                                read(len);
                                        } else if (len < -1) {
                                                is_close = TRUE;
                                        } else if (len == -1) {
                                                memset(_data, 0, BUFSIZE);
                                                _data_pos = _data;
                                                read(3);
                                        } else {
                                                _mb_mutex.lock();
                                                _mb_flag = TRUE;
                                                _read_num++;
                                                int ret = _modbus_p->parseMsg();
                                                if (!ret)
                                                        ret = _c8y->sendMsg(
                                                                _current_sid,
                                                                _mbdevices._m[_current_sid]);
                                                else ret = 0;
                                                if (!ret && _read_num == _write_num) _poll_flag = TRUE;
                                                _write_flag = TRUE;
                                                _mb_mutex.unlock();
                                                memset(_data, 0, BUFSIZE);
                                                _data_pos = _data;
                                                read(3);
					}
                                } else {
                                        SSADebug(_did + "_http: is NULL");
                                }
                                _http = (SrNetHttp*)NULL;
                        }
                }
        }

        _read_flag = FALSE;

        if (is_close) {
                this->Close(did_to_cs);
        }
        _io_mutex.unlock();
	
}

Bool ClientSession::regDev()
{
		Bool ret = TRUE;
		_sid = _c8y->getDevInfo(_did);

		if (_sid.empty()) {
				ret = FALSE;
		} else {
				vector<int> v = _c8y->getProperties(_sid);
				if (v.size()) {
					this->_polling_rate = v[0];
					this->_transmit_rate = v[1];
				} else {
		                        _c8y->setModbusParms(_sid);
				}

				did_to_cs[_did] = this;
                sid_to_cs[_sid] = this;
				string msg = "134," + _sid + ",5,DTU,"
                        + _did + ",v1.0\n" + "107," + _sid;
                SSADebug(msg);
				_agent.send(msg);
				SSADebug(_did + "--->: map size: " + to_string(did_to_cs.size()));
		}

		return ret;
}

void ClientSession::write(u8* w_buf, const u32& len)
{
        boost::asio::async_write(
                _sock, boost::asio::buffer(w_buf, len),
                boost::bind(
                        &ClientSession::onWrite,
                        shared_from_this(),
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred));
}

void ClientSession::onWrite(const boost::system::error_code& error,
                            const size_t& bytes_transferred)
{
        u32 is_close = FALSE;

	cout << _did << "|" << getClientInfo() << "----->onWrite: " << bytes_transferred << "|" << error << endl;

        if (error) {
                SSAWarn(_did + ": write error and socket close");
                is_close = TRUE;
        } else {
                if (_stopped) {
                        SSADebug("---->" + _did + " timeout handled...!");
                } else {
                        if (!_did.empty() && (did_to_cs[_did] != this)) {
                                is_close = TRUE;
                        } else {
                                SSAInfo("write to " + _did + " sucessful|"
                                        + to_string(bytes_transferred) + " bytes");
                        }
                }
        }

        if (is_close) this->Close(did_to_cs);
}

void ClientSession::operator()(Timer &timer)
{
        for (auto& m: _mbdevices._m) {
		_mbdevices.clear(m.first);
                for (int times = 0; times < 255; times++) {
                        if (_poll_flag) break;
                        const timespec ts = {0, 10};
                        nanosleep(&ts, NULL);
                }

                if (_poll_flag) {
                        _mb_mutex.lock();
                        _poll_flag = FALSE;
                        _mb_mutex.unlock();
                } else {
                        _mb_mutex.lock();
                        _poll_flag = TRUE;
                        _mb_mutex.unlock();
                        continue;
                }

                string sid = m.first;
                string type = m.second.type;
                int slave = atoi(m.second.slave.c_str());
                int ret = 0;
                Modbus mb;

                _current_sid = sid;
                _current_type = type;
                struct MbTypes& mt_ = mbtypes._m[type];
                SSAInfo("Modbus Poll: "+ getClientInfo() +"|" + _did + ": slave @" + m.second.slave);
		

                if (!m.second.addr.empty()) { // modbus tcp
                        if (-1 == mb.init("TCP", slave)) continue;
                } else {        // modbus rtu
                        if (-1 == mb.init("RTU", slave)) continue;
                }

                SSADebug(sid + "--> read co");
                map<int, int>& co_num_map = mbtypes.TYPENUM[_current_type][mbtypes.co_pos];
                for (auto& v: co_num_map) {
                        int flag = 0;		// 判断是否在有限时间内，接收到对应的响应报文
                        u8 req[MODBUS_MAX_READ_BITS];
                        ret = mb.readCo( v.first, v.second, req);
                        for (int i = 0; i < 10000000; i++){
                                if (_mb_flag) {
                                        _mb_mutex.lock();
                                        _mb_flag = FALSE;
                                        flag = 1;
                                        _write_flag = FALSE;
                                        _mb_mutex.unlock();
                                        _write_num++;
                                        _current_num = to_string(v.first);
                                        this->write(req, ret);
                                        break;
                                }
                                const timespec ts = {0, 1};
                                nanosleep(&ts, NULL);
                        }
                        if (! flag) break;		// 在有限时间内，如果没有接收到响应报文，认为这个点超时
                }

                for (int i = 0; i < 10000000000; i++){	// 解决读取最后一个点，导致轮询陷入死循环的问题。
                	if (_write_flag) break;
                    const timespec ts = {0, 1};
                    nanosleep(&ts, NULL);
                }

		if (! _mb_flag) {	// 如果在有限时间内，依然没有收到报文，认为本次轮询失败，重新轮询
			_mb_flag = TRUE;
			_mb_mutex.lock();
			_poll_flag = TRUE;
			_mb_mutex.unlock();
			//报警文本可能会改
			string text = _current_num + "," + to_string(slave) +",不存在";
			string msg = "312," + _current_sid + ",c8y_ModbusAvailabilityAlarm,MAJOR," + text;
			continue;
		}

                SSADebug(sid + "--> read di");
                map<int, int>& di_num_map = mbtypes.TYPENUM[_current_type][mbtypes.di_pos];
                for (auto& v: di_num_map) {
                        int flag = 0;
                        u8 req[MODBUS_MAX_READ_BITS];
                        ret = mb.readDi( v.first, v.second, req);
                        for (int i = 0; i < 10000000; i++){
                                if (_mb_flag) {
                                        _mb_mutex.lock();
                                        _mb_flag = FALSE;
                                        flag = 1;
                                        _mb_mutex.unlock();
                                        _write_num++;
                                        _current_num = to_string(v.first);
                                        this->write(req, ret);
                                        _write_flag = FALSE;
                                        break;
                                }
                                const timespec ts = {0, 1};
                                nanosleep(&ts, NULL);
                        }
                        if (! flag) break;
                }

                for (int i = 0; i < 1000000000; i++) {	// 解决读取最后一个点，导致轮询陷入死循环的问题。
                	if (_write_flag) break;
                	const timespec ts = {0, 1};
                	nanosleep(&ts, NULL);
                }

                if (! _mb_flag) {	// 如果在有限时间内，依然没有收到报文，认为本次轮询失败，重新轮询
                	_mb_flag = TRUE;
                	_mb_mutex.lock();
                	_poll_flag = TRUE;
                	_mb_mutex.unlock();
			//报警文本可能会改
			string text = _current_num + "," + to_string(slave) +",不存在";
			string msg = "312," + _current_sid + ",c8y_ModbusAvailabilityAlarm,MAJOR," + text;
                	continue;
                }

                SSADebug(sid + "--> read hr");
                map<int, int>& hr_num_map = mbtypes.TYPENUM[_current_type][mbtypes.hr_pos];
                for (auto& v: hr_num_map) {
                        int flag = 0;
                        u8 req[MODBUS_MAX_READ_REGISTERS];
                        ret = mb.readHr( v.first, v.second, req);
                        for (int i = 0; i < 10000000; i++){
                                if (_mb_flag) {
                                        _mb_mutex.lock();
                                        _mb_flag = FALSE;
                                        flag = 1;
                                        _mb_mutex.unlock();
                                        _write_num++;
                                        _current_num = to_string(v.first);
                                        this->write(req, ret);
                                        _write_flag = FALSE;
                                        break;
                                }
                                const timespec ts = {0, 1};
                                nanosleep(&ts, NULL);
                        }
                        if (! flag) break;
                }

                for (int i = 0; i < 1000000000; i++) {	// 解决读取最后一个点，导致轮询陷入死循环的问题。
                	if (_write_flag) break;
                	const timespec ts = {0, 1};
                	nanosleep(&ts, NULL);
                }

                if (! _mb_flag) {	// 如果在有限时间内，依然没有收到报文，认为本次轮询失败，重新轮询
                	_mb_flag = TRUE;
                	_mb_mutex.lock();
                	_poll_flag = TRUE;
                	_mb_mutex.unlock();
			//报警文本可能会改
			string text = _current_num + "," + to_string(slave) +",不存在";
			string msg = "312," + _current_sid + ",c8y_ModbusAvailabilityAlarm,MAJOR," + text;
                	continue;
                }

                SSADebug(sid + "--> read ir");
                map<int, int>& ir_num_map = mbtypes.TYPENUM[_current_type][mbtypes.ir_pos];

                for (auto& v: ir_num_map) {
                        int flag = 0;
                        u8 req[MODBUS_MAX_READ_REGISTERS];
                        ret = mb.readIr(v.first, v.second, req);
                        for (int i = 0; i < 10000000; i++){
                                if (_mb_flag) {
                                        _mb_mutex.lock();
                                        _mb_flag = FALSE;
                                        flag = 1;
                                        _mb_mutex.unlock();
                                        _write_num++;
                                        _current_num = to_string(v.first);
                                        this->write(req, ret);
                                        _write_flag = FALSE;
                                        break;
                                }
                                const timespec ts = {0, 1};
                                nanosleep(&ts, NULL);
                        }
                        if (! flag) break;
                }

                for (int i = 0; i < 10000000; i++) {	// 解决读取最后一个点，导致轮询陷入死循环的问题。
                	if (_write_flag) break;
                	const timespec ts = {0, 1};
                	nanosleep(&ts, NULL);
                }

                if (! _mb_flag) {	// 如果在有限时间内，依然没有收到报文，认为本次轮询失败，重新轮询
                	_mb_flag = TRUE;
                	_mb_mutex.lock();
                	_poll_flag = TRUE;
                	_mb_mutex.unlock();
			//报警文本可能会改
			string text = _current_num + "," + to_string(slave) +",不存在";
			string msg = "312," + _current_sid + ",c8y_ModbusAvailabilityAlarm,MAJOR," + text;
                	continue;
                }

        }
}


