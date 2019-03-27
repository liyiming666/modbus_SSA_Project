/*
 * c8y.cc
 *
 *  Created on: Sep 7, 2018
 *      Author: wenhan
 */


#include "c8y.h"
#include "ssa_logger.h"

using namespace std;

#define Q(x) ",\"\""#x"\"\""
#define Q2(x) "\"\""#x"\"\""

const char *ops = ",\"" Q2(c8y_Command) Q(c8y_ModbusDevice) Q(c8y_SetRegister)
Q(c8y_ModbusConfiguration) Q(c8y_SerialConfiguration) Q(c8y_SetCoil)
Q(c8y_LogfileRequest) Q(c8y_RemoteAccessConnect) "\"";

//extern MBTYPES mbtypes;

C8yModel::C8yModel(SrAgent& agent, SrNetHttp** http)
        : _agent(agent), _nethttp(http)
{

}

C8yModel::~C8yModel()
{

}

string C8yModel::getDevInfo(const string& did)
{
        SrNetHttp* _http = *_nethttp;
		string sid = "";
        _http->clear();
		string s = "300," + did;
        SSADebug(s);
        if (_http->post(s) <= 0) {
				SSADebug("Http.post: " + s + " failed");
                return "";
        }

        SmartRest sr(_http->response());
        SrRecord r = sr.next();

		while (r.size()) {
                if (r.size() && r.value(0) == "50") {
                        _http->clear();
                        s = "133," + did + ",\"modbus_DTU\"";
                        SSADebug(s);
                        if (_http->post(s) <= 0) {
                                SSADebug("Http.post: " + s + " failed");
                                return "";
                        }
                        SmartRest sr_1(_http->response());
                        SrRecord r_1 = sr_1.next();
                        while (r_1.size()) {
                                if (r_1.size() == 3 && r_1.value(0) == "801") {
                                        sid = r_1.value(2);
                                        s = "302," + sid + "," + did;
                                        SSADebug(s);
                                        if (_http->post(s) <= 0) {
                                                SSADebug("Http.post: " + s + " failed");
                                                return "";
                                        }
                                        _http->clear();
                                        break;
                                }
                                r_1 = sr_1.next();
                        }
                } else if (r.size() == 3 && r.value(0) == "800") {
                        sid = r.value(2);
                        _http->clear();
                        break;
                }
                r = sr.next();
		}

		if (!sid.empty()) {		// 正确获取到sid
                s = "132," + _agent.ID() + "," + sid;
                SSADebug(s);
				_http->post(s);

				SSAInfo(did + " sysId: " + sid);
		} else {
				SSAInfo(did + " get sid failed!");
		}

		return sid;
}

#if 0
void C8yModel::setInteval(ClientSession* cs)
{

}
#endif

void C8yModel::getChildDevices(const std::string& sid, ClientSession* cs)
{
        SrNetHttp* _http = *_nethttp;
        _http->clear();
        _http->post("323," + sid);
        SmartRest sr(_http->response());

        for (SrRecord r = sr.next(); r.size(); r = sr.next()) {
                const string& sid_ = r.value(4);
                const string& protocol = r.value(2);

                if (!cs->_mbdevices.addDevice(r)) {
                        this->getModel(cs->_mbdevices._m[sid_].type, cs);
                }

                const string& slave = cs->_mbdevices._m[sid_].slave;

                if (protocol == "TCP") { // tcp
                        cs->_tcp_slave_to_sid[slave] = sid_;
                } else { // rtu
                        cs->_rtu_slave_to_sid[slave].push_back(sid_);
                }
        }

        _http->clear();
}

void C8yModel::getModel(const std::string& type, ClientSession* cs)
{
        SrNetHttp* _http = *_nethttp;
        _http->clear();
        _http->post("309," + type);
        SmartRest sr(_http->response());

        for (SrRecord r = sr.next(); r.size(); r = sr.next()) {
                SSADebug("-->Trigger function: " + r.value(0));
                cs->_mbtypes.m_mod[r.value(0)](cs->_mbtypes, r);
        }

        _http->clear();
}

void C8yModel::setModbusParms(const std::string& sid)
{
        SrNetHttp* _http = *_nethttp;
        string msg = "327," + sid + ops + " \n" +
                "321," + sid + ",5,3600\n"
                + "335," + sid + ",9600,8,N,1\n";
        SSADebug("Http POST: " + msg);
        _http->post(msg);
        _http->clear();
}

void C8yModel::addDevices(SrRecord& r, ClientSession* cs)
{
        const string& sid_ = r.value(4);
        const string& protocol = r.value(2);

        if (!cs->_mbdevices.addDevice(r)) {
                this->getModel(cs->_mbdevices._m[sid_].type, cs);
        }

        const string& slave = cs->_mbdevices._m[sid_].slave;

        if (protocol == "TCP") { // tcp
                cs->_tcp_slave_to_sid[slave] = sid_;
        } else { // rtu
                cs->_rtu_slave_to_sid[slave].push_back(sid_);
        }
}

vector<int> C8yModel::getProperties(const std::string& sid)
{
	vector<int> v;
	SrNetHttp* _http = *_nethttp;
	if (_http->post("309," + sid) <= 0) {
		SSADebug(sid + " get Modbus Configuration fail!");
		return v;
	}

	SmartRest sr(_http->response());
	SrRecord r = sr.next();

	while (r.size()) {
		if (r.size() == 4 && r.value(0) == "900") {
			v.push_back(atoi(r.value(2).c_str()));
			v.push_back(atoi(r.value(3).c_str()));
		}

		r = sr.next();
	}

	_http->clear();

	return v;
}

int C8yModel::sendMsg(const std::string& sid, struct MbDevices& mbd, ClientSession* cs)
{
        const string& type = mbd.type; // template xid

	cout << "mbd.hr_num" << mbd.hr_num << "| _mbtypes._m[type].hr.size() :" <<cs->_mbtypes._m[type].hr.size() << endl;
        if (mbd.co_num != cs->_mbtypes._m[type].co.size()
            || mbd.di_num != cs->_mbtypes._m[type].di.size()
            || mbd.hr_num != cs->_mbtypes._m[type].hr.size()
            || mbd.ir_num != cs->_mbtypes._m[type].ir.size()) {
		//cout << "mbd.hr_num" << mbd.hr_num << "| _mbtypes._m[type].hr.size() :" <<cs->_mbtypes._m[type].hr.size() << endl;
		SSAInfo(cs->getClientInfo() + "---@" + mbd.slave 
                        + ": Wrong number of template counts!");
                return -1;
        }

        SrNetHttp http(_agent.server() + "/s", type, _agent.auth());

        map<string, vector<string>>& tm = mbd.template_msg; 
	map<string, vector<string>>& fm = mbd.forward_msg;
        /* send all model data */
        string send_msg = "";
	string s = "";
	string forward_s = "";
	string forward_send_msg = "";
	string is_server_time = cs->_mbtypes._m[type].is_server_time;
        for (auto& pair_: tm) {
		const string& msg_id = pair_.first;
               	string msg = msg_id + "," + sid; 
		s += msg;
		if (msg_id != "101" && msg_id != "100") {
        		if (is_server_time == "false") {
        		        msg += "," + getServerTime();
        		}
                 }
                for (auto& value: pair_.second) {
                        msg += value;
			s += value;
                }
		
                send_msg += msg + "\n";
		s += "\n";
        }
	for (auto& pair_: fm) {
		const string& msg_id = pair_.first;
		string msg = msg_id + "," + cs->_sid;
		forward_s += msg;
        	if (is_server_time == "false") {
        	        msg += "," + getServerTime();
        	}
                for (auto& value: pair_.second) {
                        msg += value;
			forward_s += value;
                }
		forward_send_msg += msg + "\n";
		forward_s += "\n";
		
	}


        mbd.template_msg.clear();
        mbd.forward_msg.clear();
        mbd.co_num = 0;
        mbd.di_num = 0;
        mbd.hr_num = 0;
        mbd.ir_num = 0;

        mbd.co.clear();
        mbd.di.clear();
        mbd.hr.clear();
        mbd.ir.clear();

        if (!send_msg.empty() || !forward_send_msg.empty()) {
		if (cs->_msg != s) { 
			cs->_msg = s;
			SSAInfo(cs->getClientInfo() + "---@" 
				+ mbd.slave + "___HTTP Post msg:\n" + send_msg);
		http.post(send_msg);
		} else {
			SSAInfo(cs->getClientInfo() + "---@"
               			+ mbd.slave + "___HTTP Post msg: 数据点无变化");
		}

		if (cs->_forward_msg != forward_s) { 
			cs->_forward_msg = forward_s;
			SSAInfo(cs->getClientInfo() + "---@" 
				+ cs->_sid + "___HTTP Post msg:\n" + forward_send_msg);
			http.post(forward_send_msg);
		} else {
			SSAInfo(cs->getClientInfo() + "---@ "
               			+ cs->_sid + "--->slave: " + mbd.slave + "___HTTP Post msg: 数据点无变化");
		}
	}

        return 0;
}
