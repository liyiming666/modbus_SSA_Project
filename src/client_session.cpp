/*g
 * client_session.cc
 #include <time.h>
 *
 *  Created on: Sep 7, 2018
 *      Author: wenhan
 */

#include <iostream>
#include <exception>
#include <boost/thread/mutex.hpp>
#include <string.h>
#include <mutex>
#include <srlogger.h>
#include <srutils.h>
#include <time.h>

#include "define.h"
#include "ssa_logger.h"
//#include "map.h"

#include "libmodbus.h"
#include "srlogger.h"
#include "client_session.h"

using namespace std;

Map<string, ClientSession*> did_to_cs;
Map<string, ClientSession*> sid_to_cs;

//extern MBTYPES mbtypes;

using namespace SSADefine;

ClientSession::ClientSession(SrAgent& agent, boost::asio::io_service& ioservice,
        Map<boost::thread::id, SrNetHttp*>& https_pool, const int& tms)
        : _agent(agent), _http((SrNetHttp*)NULL), _did(""), _sid(""), _tid(0), _c8y(
                new C8yModel(agent, &_http)), _timer(5 * 1000, this), _mbdevices(agent,
                _mbtypes), _https_pool(https_pool), _sock(ioservice), _c_ip(""), _c_port(
                0), _modbus_p(new ModbusP(&_data, &_data_pos, this)), _deadline(
                ioservice), _deadline_timer(ioservice), _polling_rate(5), _receHeartBeat(
                FALSE), _tms(tms), _strand(ioservice), _is_close(FALSE), _count(0), _flag(
                0), _is_disconnect(FALSE)
{
        _data = new u8[1024];
        _data_pos = _data;

        _modbus_p->init();

        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL); //允许退出线程
        pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL); //设置立即取消
}

ClientSession::~ClientSession()
{
        SSAInfo("---------pthread join befor");
        pthread_join(_tid, NULL);
        SSAInfo("---------pthread join after");
        pthread_cancel(_tid);

        SSADebug(_did + "---> ~ClientSession");

        if (_data) {
                SSADebug(_did + "---> delete data buffer: ");
                delete[] _data;
                _data = nullptr;
                _data_pos = nullptr;
        }

        delete _modbus_p;
        delete _c8y;
        _modbus_p = nullptr;
        _c8y = nullptr;
}

void* ClientSession::func(void* arg)
{
        ClientSession* cs = (ClientSession*)arg;
        //_timer的值代表轮寻周期，初始化默认值为5秒
        //根据inclue/timer.h中的描述，_timer到时后执行回调函数this, 即ClientSession::operator()(Timer &timer)
        cs->_timer.start();
        pthread_testcancel();
        cs->_timer.loop();
        pthread_testcancel();

        return (void*)NULL;
}

void ClientSession::checkDeadline(const boost::system::error_code& error)
{
        _io_mutex.lock();

        if (!error) {
                if (_receHeartBeat) {
                        _receHeartBeat = FALSE;
                        this->setTimer();
                } else {
                        SSAInfo(_did + " check_deadline and socket close");
                        this->toClose();
                        // 定时器 设定为永不超时/不可用状态
                        _deadline.expires_at(boost::posix_time::pos_infin);
                }
        }

        _io_mutex.unlock();
}

//DTU下面可能包含多个slave设备，目前设置与每个slave设备最长通信时间为2秒，如果超时，则代表该slave设备出现问题，需要跳过。
//下面code中的if (!_mutex.try_lock())用于检测当前slave是否仍处于通信上锁状态，如果是，则报告" 通讯异常"错误，然后解锁，以便其它slave设备可以继续和ssa通信。
void ClientSession::checkSalveDeadline(const boost::system::error_code& error)
{
        if (!error) {
                _timeout_mutex.lock();

                if (_flag == 0) {
                        if (!_mutex.try_lock()) {
                                string alm = "312," + _current_sid
                                        + ",c8y_SlaveAlarm,MAJOR," + "slave: @"
                                        + _mbdevices._m[_current_sid].slave + " 通讯异常";
                                SSAInfo(alm);
                        }

                        _flag = 1;
                        _mutex.unlock();
                }

                //setTimer_1只需要在开始读每个slave设备的时候执行，确保每个slave设备的操作不要无限制的等下去
                //if (!_is_close) this->setTimer_1();
                _timeout_mutex.unlock();
        } else {
                SSADebug("---->: " + (string)__func__ + ": " + error.message());
        }

}

void ClientSession::toClose()
{
        if (!_is_disconnect) {
                _is_disconnect = TRUE;
                _timer.stop();
        }

        _strand.dispatch(bind(&ClientSession::Close, shared_from_this()));
}

void ClientSession::Close()
{
        SSAInfo(_did + " Close socket: ip: " + this->getClientInfo());

        if (!_did.empty()) {
                did_to_cs.erase(_did);
        }

        if (!_sid.empty()) {
                sid_to_cs.erase(_sid);
        }

        if (_sock.is_open()) {
                try {
                        boost::system::error_code ec;
                        _sock.cancel(ec);

                        if (!ec) {
                                SSAInfo("socket close: " + this->getClientInfo());
                                _sock.shutdown(tcp::socket::shutdown_both, ec);
                                SSAInfo("sock.close : " + this->getClientInfo());
                                _sock.close(ec);
                                _deadline.cancel();
                                _deadline_timer.cancel();
                                _mutex.unlock();
                        }
                } catch (exception& e) {
                        SSAError("Close exception:" + _did + " " + e.what());
                }
        }
}

void ClientSession::readReg(const unsigned long& len)
{
        try {
                _sock.async_read_some(boost::asio::buffer(_data_pos, len),
                        boost::bind(&ClientSession::onReadReg, shared_from_this(),
                                boost::asio::placeholders::error,
                                boost::asio::placeholders::bytes_transferred));
        } catch (exception& e) {
                SSAError("readReg exception |" + string(e.what()));
        }
}

void ClientSession::onReadReg(const boost::system::error_code& error,
        const size_t& bytes_transferred)
{
        _io_mutex.lock();

        try {
                _is_close = FALSE;
                _receHeartBeat = TRUE;

                if (error) {    // 设备主动关闭或socket被取消
                        SSAWarn(_did + ": read reg error and socket close");
                        _is_close = TRUE;
                } else {
                        if (!_did.empty() && (did_to_cs[_did] != this)) {
                                _is_close = TRUE;
                        } else {
                                boost::thread::id t_id = boost::this_thread::get_id();
                                _http = _https_pool[t_id];

                                if (_http) {
                                        _data_pos += bytes_transferred;
                                        int len = _modbus_p->checkReg();

                                        if (len > 0) { // not enough reg package
                                                readReg(len);
                                        } else if (len < 0) { // error
                                                _is_close = TRUE;
                                        } else { // regist package is correct
                                                if (!this->regDev()) { // fail
                                                        _is_close = TRUE;
                                                } else { // successful
                                                        _http->post("107," + _sid);
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
        } catch (exception& e) {
                SSAError("onReadReg exception |" + string(e.what()));
        }

        if (_is_close) {
                this->toClose();
        }

        _io_mutex.unlock();
}

void ClientSession::init()
{
        //this是func的传入参数arg
        //创建出子线程后，执行func
        int ret = pthread_create(&_tid, NULL, func, this);

        if (ret) {
                SSAError("create thread fail...");

        }

        //	ret = pthread_detach(_tid);
        //这里启动的是heart beat的定时器, 轮寻定时器通过clientsession中的_timer.setInterval进行设置
        this->setTimer();
        this->readReg(19);
}

void ClientSession::read(const unsigned long& len)
{
        try {
                if (!_sock.is_open()) {
                        SSADebug("==============================>"
                                + (string)__func__ + ": sock closed");
                } else {
                        _sock.async_read_some(boost::asio::buffer(_data_pos, len),
                                boost::bind(&ClientSession::onRead, shared_from_this(),
                                        boost::asio::placeholders::error,
                                        boost::asio::placeholders::bytes_transferred));
                }
        } catch (exception& e) {
                SSAError("Read exception: -----did: " + _did + "|" + e.what());
        }
}

void ClientSession::onRead(const boost::system::error_code& error,
        const size_t& bytes_transferred)
{
        _io_mutex.lock();

        try {
                _receHeartBeat = TRUE;
                _is_close = FALSE;

                if (error) {    // 设备主动关闭或socket被取消
                        SSAWarn(_did + ": read error and socket close----" + error.message());
                        _is_close = TRUE;
                } else {
                        try {
                                if (!_did.empty() && (did_to_cs[_did] != this)) {
                                        _is_close = TRUE;
                                } else {
                                        boost::thread::id t_id =
                                                boost::this_thread::get_id();
                                        _http = _https_pool[t_id];

                                        if (_http) {
                                                _data_pos += bytes_transferred;
                                                int len = _modbus_p->checkMsg();

                                                if (len > 0) { // not enough
                                                        read(len);
                                                } else if (len < -1) { // error
                                                        _is_close = TRUE;
                                                } else if (len == -1) { // heart beat
                                                        string hb_msg = "107," + _sid;
                                                        SSADebug(hb_msg);
                                                        _http->post(hb_msg);
                                                        memset(_data, 0, BUFSIZE);
                                                        _data_pos = _data;
                                                        read(3);
                                                } else { // modbus data
                                                        this->process();
                                                        memset(_data, 0, BUFSIZE);
                                                        _data_pos = _data;
                                                        read(3);
                                                }
                                        } else {
                                                SSADebug(_did + "_http: is NULL");
                                        }

                                        _http = (SrNetHttp*)NULL;
                                }
                        } catch (exception& e) {
                                SSAError("onRead_exception did: ---------->: "
                                        + _did + "|" + e.what());
                        }
                }
        } catch (exception& e) {
                SSAError("Close exception:" + _did + " " + e.what());
        }

        if (_is_close) {
                this->toClose();
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
                        this->_timer.setInterval(_polling_rate * 1000);
                } else {
                        _c8y->setModbusParms(_sid);
                }

                did_to_cs[_did] = this;
                sid_to_cs[_sid] = this;
                string msg = "134," + _sid + ",20,DTU," + _did + ",v1.0";
                SSADebug(msg);
                _http->post(msg);
                SSADebug(_did + "--->: Map size: " + to_string(did_to_cs.size()));
        }

        return ret;
}

void ClientSession::process()
{
        string hb_msg = "107," + _sid;
        SSADebug("send msg: " + hb_msg);
        _http->post(hb_msg);
        int ret = _modbus_p->parseMsg();
        ret = _c8y->sendMsg(_current_sid, _mbdevices._m[_current_sid], this);

        //process在onRead中被调用，这里的_mutex.unlock代表此时收到了DTU的回应，可以继续下面的通信，因此先解锁_mutex，提供机会供其它需要与DTU通信的线程获取这把锁
        _mutex.unlock();
        _count = 0;
}

void ClientSession::write(u8* w_buf, const u32& len)
{
        _io_mutex.lock();

        try {
                if (!_sock.is_open()) {
                        SSADebug((string)__func__ + ": sock closed");
                } else {
                        boost::asio::async_write(_sock, boost::asio::buffer(w_buf, len),
                                boost::bind(&ClientSession::onWrite, shared_from_this(),
                                        boost::asio::placeholders::error,
                                        boost::asio::placeholders::bytes_transferred));
                }
        } catch (exception& e) {
                SSAError("write exception |" + string(e.what()));
        }

        _io_mutex.unlock();
}

void ClientSession::onWrite(const boost::system::error_code& error,
        const size_t& bytes_transferred)
{
        try {
                _is_close = FALSE;
                SSADebug(_did + "----" + getClientInfo() + "--->write bytes: "
                                + to_string(bytes_transferred) + "|" + error.message());

                if (error) {
                        SSAWarn(_did + ": write error and socket close");
                        _is_close = TRUE;
                } else {
                        if (!_did.empty() && (did_to_cs[_did] != this)) {
                                _is_close = TRUE;
                        } else {
                                SSAInfo("write to " + _did + " sucessful|"
                                                + to_string(bytes_transferred)
                                                + " bytes");
                        }
                }
        } catch (exception& e) {
                SSAError("onWrite_exception did: ---------->: " + _did + "|" + e.what());
        }

        if (_is_close) {
                this->toClose();
        }
}

void ClientSession::operator()(Timer &timer)
{
        //依次查询该DTU下的所有slave设备
        for (auto& m : _mbdevices._m) {
                /*
                 设备轮询slave的时候，下发最后一个点之后，
                 for循环将会执行下一个slave，会导致_current_sid改变
                 */
                _mutex.lock();
                _mutex.unlock();
                if (_is_close) {
                        return;
                }

                //每次开始读取一个slave设备的数据时，都需要初始化_flag
                _timeout_mutex.lock();
                _flag = 0;
                _timeout_mutex.unlock();
                //设置与slave设备通信的超时时间，防止某个slave设备损坏，导致该DTU下所有的slave设备都无法与ssa通信
                this->setTimer_1();

                long t1, t2;
                t1 = getMilliseconds();

                _mbdevices.clear(m.first);

                string sid = m.first;
                string type = m.second.type;
                int slave = atoi(m.second.slave.c_str());
                int ret = 0;
                Modbus mb;

                _current_sid = sid;
                _current_type = type;
                //struct MbTypes& mt_ = _mbtypes._m[type];
                SSAInfo("Modbus Poll: " + getClientInfo() + "|"
                        + _did + ": slave @" + m.second.slave);

                if (m.second.slave.empty()) {
                        SSADebug("slave is empty---" + sid + "|" + this->getClientInfo());
                        continue;
                }

                if (!m.second.addr.empty()) { // modbus tcp
                        if (-1 == mb.init("TCP", slave)) {
                                continue;
                        }
                } else {        // modbus rtu
                        if (-1 == mb.init("RTU", slave)) {
                                continue;
                        }
                }

                map<int, map<string, vector<map<int, int>>>&>::iterator it =
                        _mbtypes.TYPE_COL.begin();

                //依次查询某个slave设备下所有的寄存器
                for (; it != _mbtypes.TYPE_COL.end(); it++) {
                        int func_code = it->first;	//表示的是不同的功能码

                        if (!func_code) {
                                SSADebug(sid + "--> read co");
                        } else if (func_code == 1) {
                                SSADebug(sid + "--> read di");
                        } else if (func_code == 2) {
                                SSADebug(sid + "--> read hr");
                        } else {
                                SSADebug(sid + "--> read ir");
                        }

                        map<string, vector<map<int, int>>>& collection = it->second;
                        map<int, int>& current_map = collection[type][func_code];

                        //依次查询某种寄存器所有地址的操作
                        for (auto& v : current_map) {
                                //正常情况下，unlock在onRead->process中给出，即收到DTU的回应后，继续下一个点的轮询
                                //异常情况下，会等到checkSalveDeadline的超时处理，在其中unlock
                                _mutex.lock();
#if 1
                                //（_flag==1）代表当前slave已超时，因此应该跳过当前slave的所有后续操作
                                if (_flag == 1) {
                                        SSADebug("_flag==1, pass all reading of current slave");
                                        _mutex.unlock();

                                        break;
                                }
#endif
                                u8 req[1024] = {0};
                                ret = mb.readBat(v.first, v.second, req, func_code + 1);
                                _current_num = to_string(v.first);

                                if (_is_close) {
                                        return;
                                }

                                this->write(req, ret);
                        }
                }

                t2 = getMilliseconds();
                SSADebug("DTU: " + _did + "|slave: " + sid
                        + "---Modbus Poll time: " + to_string(t2 - t1));
        }
}
