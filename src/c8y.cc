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

extern MBTYPES mbtypes;

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
                        s = "133," + did + ",\"DTU Child Device\"";
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

void C8yModel::setInteval(ClientSession* cs)
{

}

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
                        this->getModel(cs->_mbdevices._m[sid_].type);
                }

                const string& slave = cs->_mbdevices._m[sid_].slave;

                if (protocol == "TCP") { // tcp
                        cs->_tcp_slave_to_sid[slave] = sid_;
                } else { // rtu
                        cs->_rtu_slave_to_sid[slave] = sid_;
                }
        }

        _http->clear();
}

void C8yModel::getModel(const std::string& type)
{
        SrNetHttp* _http = *_nethttp;
        _http->clear();
        _http->post("309," + type);
        SmartRest sr(_http->response());

        for (SrRecord r = sr.next(); r.size(); r = sr.next()) {
                SSADebug("-->Trigger function: " + r.value(0));
                mbtypes.m_mod[r.value(0)](mbtypes, r);
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
                this->getModel(cs->_mbdevices._m[sid_].type);
        }

        const string& slave = cs->_mbdevices._m[sid_].slave;

        if (protocol == "TCP") { // tcp
                cs->_tcp_slave_to_sid[slave] = sid_;
        } else { // rtu
                cs->_rtu_slave_to_sid[slave] = sid_;
        }
}

void C8yModel::getModel(const std::string& type, ClientSession* cs)
{
        SrNetHttp* _http = cs->_https_pool[boost::this_thread::get_id()];
        _http->clear();
        _http->post("309," + type);
        SmartRest sr(_http->response());

        for (SrRecord r = sr.next(); r.size(); r = sr.next()) {
                SSADebug("-->Trigger function: " + r.value(0));
                mbtypes.m_mod[r.value(0)](mbtypes, r);
        }

        _http->clear();

#if 0
        for (auto& v: mbtypes._m[type].co) {
                for(auto& s: v){
                        cout << "-->: " << s << endl;
                }
		}

        for (auto& v: mbtypes._m[type].hr) {
                for(auto& s: v){
                        cout << "-->: " << s << endl;
                }
		}
#endif
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

int C8yModel::sendMsg(const std::string& sid, struct MbDevices& mbd)
{
        const string& type = mbd.type; // template xid
	//printf("mbd.hr_num---> %d , mbtypes._m[type].hr.size()---> %d\n",mbd.hr_num,mbtypes._m[type].hr.size());
        if (mbd.co_num != mbtypes._m[type].co.size()
            || mbd.di_num != mbtypes._m[type].di.size()
            || mbd.hr_num != mbtypes._m[type].hr.size()
            || mbd.ir_num != mbtypes._m[type].ir.size()) {
                return -1;
        }

        string basic_msg = "," + sid;
        if (mbtypes._m[type].is_server_time == "false") {
                basic_msg += "," + getServerTime();
        }


        SrNetHttp http(_agent.server() + "/s", type, _agent.auth());

        map<string, vector<string>>& tm = mbd.template_msg;

        /* send all model data */
        string send_msg = "";
        for (auto& pair_: tm) {
                const string& msg_id = pair_.first;
                string msg = msg_id + basic_msg;
                for (auto& value: pair_.second) {
                        msg += value;
                }
                send_msg += msg + "\n";
        }

        mbd.template_msg.clear();
        mbd.co_num = 0;
        mbd.di_num = 0;
        mbd.hr_num = 0;
        mbd.ir_num = 0;

        mbd.co.clear();
        mbd.di.clear();
        mbd.hr.clear();
        mbd.ir.clear();

        if (!send_msg.empty()) http.post(send_msg);

        return 0;
}


