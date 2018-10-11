/*
 * modbus_model.cc
 *
 *  Created on: Sep 7, 2018
 *      Author: wenhan
 */

#include <math.h>

#include "modbus_model.h"

MBTYPES mbtypes;

MBTYPES::MBTYPES(void)
{
        m_mod["817"] = &MBTYPES::saveConfigure;
		m_mod["821"] = &MBTYPES::addCoil;
		m_mod["822"] = &MBTYPES::addCoilAlarm;
		m_mod["824"] = &MBTYPES::addCoilEvent;
		m_mod["825"] = &MBTYPES::addRegister;
		m_mod["826"] = &MBTYPES::addRegisterAlarm;
		m_mod["827"] = &MBTYPES::addRegisterMeasurement;
		m_mod["828"] = &MBTYPES::addRegisterEvent;
        m_mod["829"] = &MBTYPES::setServerTime;
		m_mod["830"] = &MBTYPES::addCoilStatus;
		m_mod["831"] = &MBTYPES::addRegisterStatus;
		m_mod["833"] = &MBTYPES::setCoil;
        m_mod["834"] = &MBTYPES::setRegister;
		m_mod["839"] = &MBTYPES::setmbtype;
		m_mod["840"] = &MBTYPES::setmbtype;
}

void MBTYPES::addCoil(SrRecord &r){ //821
		string number = r.value(2);
		vector<string> v = {number, "", "", ""};
		int n = atoi(number.c_str());
		if(r.value(3) == "false"){
                	_m[_current_type].co.push_back(v);
			map<int, int>& co_num_map = TYPENUM[_current_type][mbtypes.co_pos];
			int length = co_num_map.size();
			printf("---%d\n",length);
			map <int, int>::reverse_iterator it = co_num_map.rbegin();
			if (!length) {
				co_num_map.insert(pair<int,int>(n,1));
			}else if (n != it->first + it->second) {
				co_num_map.insert(pair<int,int>(n,1));
			} else {
				it->second = it->second+1;
			}
		}else{
                	_m[_current_type].di.push_back(v);
			map<int, int>& di_num_map = TYPENUM[_current_type][mbtypes.di_pos];
			int length = di_num_map.size();
			map <int, int>::reverse_iterator it = di_num_map.rbegin();
			if (!length || n != it->first + it->second) {
				di_num_map.insert(pair<int,int>(n,1));
			}else{
				it->second = it->second+1;
			}
		}
}


void MBTYPES::addCoilAlarm(SrRecord &r){ //822
		string number = r.value(2);
		string alarm = r.value(3);
		if(r.value(4) == "false"){
                for (auto& v:  _m[_current_type].co) {
                        if (v[_m[_current_type].number] == r.value(2)) {
                                v[_m[_current_type].alarm] = alarm;
                                break;
                        }
                }
		}
		else {
                for (auto& v: _m[_current_type].di) {
                        if (v[_m[_current_type].number] == r.value(2)) {
                                v[_m[_current_type].alarm] = alarm;
                                break;
                        }
                }
		}
}

void MBTYPES::addCoilEvent(SrRecord &r){ //824
		string number = r.value(2);
		string event = r.value(3);
		if(r.value(4) == "false"){
                for (auto& v:  _m[_current_type].co) {
                        if (v[_m[_current_type].number] == r.value(2)) {
                                v[_m[_current_type].event] = event;
                                break;
                        }
                }
		}
		else {
                for (auto& v: _m[_current_type].di) {
                        if (v[_m[_current_type].number] == r.value(2)) {
                                v[_m[_current_type].event] = event;
                                break;
                        }
                }
		}
}

void MBTYPES::addCoilStatus(SrRecord &r){ //830
		if (r.value(4) == "false")
                for (auto& v: _m[_current_type].co) {
                        if (v[_m[_current_type].number] == r.value(2)) {
                                v[_m[_current_type].status] = "100";
                                break;
                        }
                }
		else {
                for (auto& v: _m[_current_type].di) {
                        if (v[_m[_current_type].number] == r.value(2)) {
                                v[_m[_current_type].status] = "100";
                                break;
                        }
                }
		}
}

void MBTYPES::saveConfigure(SrRecord &r){ //817
		printf("saveConfigure\n");

}

void MBTYPES::addRegister(SrRecord &r){ //825
		printf("addRegister\n");
		string number = r.value(2);
		string startBit = r.value(3);
		string noBits = r.value(4);
		int f = atoi(r.value(5).c_str());
		int a = atoi(r.value(6).c_str());
		int c = atoi(r.value(7).c_str());
		int n = atoi(r.value(2).c_str());
		string factor = to_string((double)f/a/pow(10, c));
		string s = r.value(9);
		cout << "=====>factor: " << factor <<endl;
		vector<string> v = {number, "", "", "", startBit, noBits, factor, "", s};
		cout << "=====>r.value(9): " << s <<endl;
		

		if(r.value(8) == "false"){
                	_m[_current_type].hr.push_back(v);
			map<int, int>& hr_num_map = TYPENUM[_current_type][mbtypes.hr_pos];
			int length = hr_num_map.size();
			map <int, int>::reverse_iterator it = hr_num_map.rbegin();
			if (!length) {
				hr_num_map.insert(pair<int,int>(n,1));
			}else if (n > it->first + it->second) {
				hr_num_map.insert(pair<int,int>(n,1));
			} else if (n == it->first + it->second) {
				it->second = it->second+1;
			}
		}else{
                	_m[_current_type].ir.push_back(v);
			map<int, int>& ir_num_map = TYPENUM[_current_type][mbtypes.ir_pos];
			int length = ir_num_map.size();
			map <int, int>::reverse_iterator it = ir_num_map.rbegin();
			if (!length) {
				ir_num_map.insert(pair<int,int>(n,1));
			}else if (n > it->first + it->second) {
				ir_num_map.insert(pair<int,int>(n,1));
			} else if (n == it->first + it->second) {
				it->second = it->second+1;
			}
		}
}

void MBTYPES::addRegisterAlarm(SrRecord &r){ //826
		printf("addRegisterAlarm\n");
		string number = r.value(2);
		string startBit = r.value(3);
		string alarm = r.value(4);
		if (r.value(5) == "false") {
                for(auto& v: _m[_current_type].hr){
                        if(v[_m[_current_type].number] == number &&
				v[_m[_current_type].startBits] == startBit) {
                                v[_m[_current_type].alarm] = alarm;
                                break;
                        }
                }
		} else {
                for(auto& v: _m[_current_type].ir){
                        if(v[_m[_current_type].number] == number &&
				v[_m[_current_type].startBits] == startBit) {
                                v[_m[_current_type].alarm] = alarm;
                                break;
                        }
                }

		}
}

void MBTYPES::addRegisterMeasurement(SrRecord &r){ //827
		printf("addRegisterMeasurement\n");
		string number = r.value(2);
		string startBit = r.value(3);
		string measurement = r.value(4);
		if(r.value(5) == "false"){
                for(auto& v: _m[_current_type].hr){
                        if(v[_m[_current_type].number] == number &&
				v[_m[_current_type].startBits] == startBit) {
                                v[_m[_current_type].measurement] = measurement;
                                break;
                        }
                }
		}
		else{
                for(auto& v: _m[_current_type].ir){
                        if(v[_m[_current_type].number] == number &&
				v[_m[_current_type].startBits] == startBit) {
                                v[_m[_current_type].measurement] = measurement;
                                break;
                        }
                }
		}

}

void MBTYPES::addRegisterEvent(SrRecord &r){ //828
		printf("addRegisterEvent\n");
		string number = r.value(2);
		string startBit = r.value(3);
		string event = r.value(4);
		if(r.value(5) == "false"){
                for(auto& v: _m[_current_type].hr){
                        if(v[_m[_current_type].number] == number &&
				v[_m[_current_type].startBits] == startBit) {
                                v[_m[_current_type].event] = event;
                                break;
                        }
                }
		}
		else{
                for(auto& v: _m[_current_type].ir){
                        if(v[_m[_current_type].number] == number &&
				v[_m[_current_type].startBits] == startBit) {
                                v[_m[_current_type].event] = event;
                                break;
                        }
                }
		}
}

void MBTYPES::addRegisterStatus(SrRecord &r){ //831
		printf("addRegisterStatus\n");
		string number = r.value(2);
		string startBit = r.value(3);
		if(r.value(5) == "false"){
                for(auto& v: _m[_current_type].hr){
                        if(v[_m[_current_type].number] == number &&
				v[_m[_current_type].startBits] == startBit) {
                                v[_m[_current_type].status] = "101";
                                break;
                        }
                }
		}
		else{
                for(auto& v: _m[_current_type].ir){
                        if(v[_m[_current_type].number] == number &&
				v[_m[_current_type].startBits] == startBit) {
                                v[_m[_current_type].status] = "101";
                                break;
                        }
                }
		}
}

void MBTYPES::setCoil(SrRecord &r){ //833
		printf("setCoil\n");

}
void MBTYPES::setRegister(SrRecord &r){ //834
		printf("setRegister\n");

}
void MBTYPES::setmbtype(SrRecord &r){ //839
		printf("setmbtype\n");
		_current_type = r.value(2);
}

void MBTYPES::setServerTime(SrRecord& r) // 829
{
        this->_m[_current_type].is_server_time = r.value(2);
}

void MBTYPES::operator()(SrRecord &r, SrAgent &agent)
{
		printf("MBTYPES operator()\n");
		m_mod[r.value(0)](*this,r);
}

MBDEVICES::MBDEVICES(SrAgent& agent)
        :_agent(agent)
{
		m_mod["816"] = &MBDEVICES::addDevice;
		m_mod["847"] = &MBDEVICES::addDevice;
		m_mod["832"] = &MBDEVICES::addDevice;
		m_mod["848"] = &MBDEVICES::addDevice;
}

int MBDEVICES::addDevice(SrRecord &r){ //816 847 832 848
		string slave = r.value(3);
		string sid = r.value(4);
		string s_type;
		size_t pos = r.value(5).find("/inventory/managedObjects/");

		if (pos != string::npos) {
                pos += sizeof("/inventory/managedObjects/")-1;
                s_type = r.value(5).substr(pos, r.value(5).size()-pos);
		}

		cout << "stype: " << s_type << endl;
		string addr;
		string ops_id;

		if (r.size() == 8) {
                ops_id = r.value(7);
		} else {
                ops_id = "";
		}

		if (r.value(2) == "TCP") {
                addr = r.value(2);
        } else {
                addr = "";
		}

		struct MbDevices m;
		_m.insert(pair<string, struct MbDevices>(sid, m));
		_m[sid].addr = addr;
		_m[sid].slave = slave;
		_m[sid].type = s_type;
		_m[sid].ops_id = ops_id;

		if (mbtypes._m.find(s_type) == mbtypes._m.end()) {
                struct MbTypes m;
                mbtypes._m.insert(pair<string, struct MbTypes>(s_type, m));
		for (int i = 0; i < 4; i++) {
                map<int, int> m;
                mbtypes.TYPENUM[s_type].push_back(m);
		}
                return 0;
		} else {
                return 1;
        }
}
void MBDEVICES::clear(const string& sid){
	_m[sid].template_msg.clear();
        _m[sid].co_num = 0;
	_m[sid].di_num = 0;
	_m[sid].hr_num = 0;
	_m[sid].ir_num = 0;
	_m[sid].co.clear();
	_m[sid].di.clear();
	_m[sid].hr.clear();
	_m[sid].ir.clear();
}
void MBDEVICES::operator()(SrRecord &r ,SrAgent &agent)
{
		printf("MBDEVICES operator()\n");
		m_mod[r.value(0)](*this, r);
}


