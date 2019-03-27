/*
 * agent_msg_handler.cc
 *
 *  Created on: Sep 7, 2018
 *      Author: wenhan
 */

#include <unordered_map>
#include "srlogger.h"
#include "agent_msg_handler.h"
#include "client_session.h"
#include "map.h"
#include "ssa_logger.h"
#include "libmodbus.h"
#include "modbus_model.h"

extern Map<string, ClientSession*> did_to_cs;
extern Map<string, ClientSession*> sid_to_cs;


//extern unbat(u16 d, int& sb, int& value);

AgentMsgHandler::AgentMsgHandler(SrAgent& agent, SrNetHttp* http)
        : _agent(agent), _http(http)
{
        this->init();
}
/* 析构函数 */
AgentMsgHandler::~AgentMsgHandler()
{
}
void AgentMsgHandler::init()
{
        _agent.addMsgHandler(816, this);
        _agent.addMsgHandler(832, this);
        _agent.addMsgHandler(847, this);
        _agent.addMsgHandler(848, this);
        _agent.addMsgHandler(833, this);
        _agent.addMsgHandler(834, this);
        _agent.addMsgHandler(817, this);
/*
  _agent.addMsgHandler(816, this);
  _agent.addMsgHandler(817, this);
  _agent.addMsgHandler(821, this);
  _agent.addMsgHandler(822, this);
  _agent.addMsgHandler(823, this);
  _agent.addMsgHandler(824, this);
  _agent.addMsgHandler(825, this);
  _agent.addMsgHandler(826, this);
  _agent.addMsgHandler(827, this);
  _agent.addMsgHandler(828, this);
  _agent.addMsgHandler(829, this);
  _agent.addMsgHandler(830, this);
  _agent.addMsgHandler(831, this);
  _agent.addMsgHandler(832, this);
  _agent.addMsgHandler(833, this);
  _agent.addMsgHandler(834, this);
  _agent.addMsgHandler(835, this);
  _agent.addMsgHandler(836, this);
  _agent.addMsgHandler(839, this);
  _agent.addMsgHandler(830, this);
  _agent.addMsgHandler(847, this);
  _agent.addMsgHandler(848, this);
  _agent.addMsgHandler(849, this);
  _agent.addMsgHandler(851, this);
*/
}
void AgentMsgHandler::operator()(SrRecord &r, SrAgent &agent)
{
        if (r.value(0) == "832" || r.value(0) == "847") {
                ClientSession* cs = sid_to_cs[r.value(6)];
                string ops_id = r.value(8);
                if (!cs) {
                        string msg = "304," + ops_id + ",设备不在线";
                        SSADebug("=====>: " + msg);
                        _http->post(msg);
                } else {
			cs->_http = getHttp();
                        cs->_c8y->addDevices(r, cs);
                        string msg = "303," + ops_id + ",EXECUTING\n"
                                + "303," + ops_id + ",SUCCESSFUL\n";
                        SSADebug("=====>: " + msg);
                        _http->post(msg);
                }
        } else if (r.value(0) == "833" || r.value(0) == "834") {
                _http->post("303," + r.value(2) + ",EXECUTING\n");
                //两个设置值的模板
                string sid = r.value(3);
                for(auto& v : did_to_cs){
                        ClientSession* cs = v.second; //取到每一个session
                        for(auto& mbd: cs->_mbdevices._m){
                                if (sid == mbd.first) { // find correct session
					//for (int i=0; i<255; i++){
					for (long i=0; i<999999999; i++){
						if(cs->getMutex().try_lock()){
							this->setValue(cs->sid(), r, mbd);
							//下发的逻辑问题 文本
							_http->post("303," + r.value(2) + ",SUCCESSFUL\n");
							return;
						}
						//const timespec ts ={0, 7000000};
						//nanosleep(&ts, NULL);
					}
					string msg = "304," + r.value(2) + ",系统繁忙请稍后再试";
					SSADebug("=====>: " + msg);
					_http->post(msg);
					/*
					if(!cs->getMutex().try_lock()){
						string msg = "304," + r.value(2) + ",系统繁忙请稍后再试";
						SSADebug("=====>: " + msg);
						_http->post(msg);
						return;
					}
                                       	//cs->getMutex().lock();
					//SSAInfo("msg_hander if getMutex().lock()------");
                                        this->setValue(cs->sid(), r, mbd);
					*/
				}
                        }
                }
		//下发的逻辑问题 文本
                _http->post("303," + r.value(2) + ",SUCCESSFUL\n");
        } else if (r.value(0) == "817") {
		string ops_id = r.value(2);
                int polling_rate = atoi(r.value(3).c_str());
                int transmit_rate = atoi(r.value(4).c_str());
                const string& sid = r.value(5);
                string msg = "303," + ops_id + ",EXECUTING\n";
                SSADebug("=====>: " + msg);
                _http->post(msg);
                ClientSession* cs = sid_to_cs[sid];
		if (!cs) {
			string msg = "304," + ops_id + ",设备不在线";
			SSADebug("=====>: " + msg);
			_http->post(msg);
		} else {
	                cs->_timer.setInterval(polling_rate*1000);
        	        _http->post("321," + sid + "," + to_string(polling_rate)
        	                    + "," + to_string(transmit_rate));
	
        	        msg = "303," + r.value(2) + ",SUCCESSFUL\n";
	                SSADebug("=====>: " + msg);
                	_http->post(msg);
		}
        }
}

void AgentMsgHandler::setValue(string sid, SrRecord& r, pair<string, struct MbDevices> mbd)
{
        int addr = atoi(r.value(4).c_str());
        int slave =atoi(mbd.second.slave.c_str());
        Modbus mb;

        if (!mbd.second.addr.empty()) {
                if (-1 == mb.init("TCP", slave)) {
                        return;
                }
        } else {        // modbus rtu
                if (-1 == mb.init("RTU", slave)) {
                        return;
                }
        }

        u8 req[MODBUS_MAX_READ_REGISTERS];
        int ret = 0;

        if (r.value(0) == "833"){
                int new_value =atoi(r.value(5).c_str());
                ret = mb.writeCo(addr, new_value, req);
        } else {
                // 834,1,869433,546820,1,0,16,100
                int number = atoi(r.value(7).c_str());
                ret = mb.writeHr(addr, number, req);
        }

        ClientSession* cs = sid_to_cs[sid];
	
	if (cs) cs->write(req, ret);
	else cout << "==================cs is null" << endl;
}
