/*
 * modbus_model.cc
 *
 *  Created on: Sep 7, 2018
 *      Author: wenhan
 */

#include <math.h>

#include "modbus_model.h"
#include "ssa_logger.h"

//MBTYPES mbtypes;

MBTYPES::MBTYPES(void)
        : TYPE_COL( { {0, TYPENUM}, {1, TYPENUM}, {2, TYPENUM}, {3, TYPENUM}}),
          _mt(nullptr)
{
        //m_mod["817"] = &MBTYPES::saveConfigure;
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
        /*
         TYPE_COL[0] = TYPENUM;
         TYPE_COL[1] = TYPENUM;
         TYPE_COL[2] = TYPENUM;
         TYPE_COL[3] = TYPENUM;
         */
}

//821
void MBTYPES::addCoil(SrRecord &r)
{
        string number = r.value(2);

        // number, alarm, event, status, alarm_condition, event_condition
        vector<string> v = {number, "", "", "", "", ""};

        int n = atoi(number.c_str());

        if (r.value(3) == "false") {
                _m[_current_type].co.push_back(v);
                map<int, int>& co_num_map = TYPENUM[_current_type][co_pos];
                int length = co_num_map.size();
                map<int, int>::reverse_iterator it = co_num_map.rbegin();
                if (!length) {
                        co_num_map.insert(pair<int, int>(n, 1));
                } else if (n != it->first + it->second) {
                        co_num_map.insert(pair<int, int>(n, 1));
                } else {
                        it->second = it->second + 1;
                }
        } else {
                _m[_current_type].di.push_back(v);
                map<int, int>& di_num_map = TYPENUM[_current_type][di_pos];
                int length = di_num_map.size();
                map<int, int>::reverse_iterator it = di_num_map.rbegin();
                if (!length || n != it->first + it->second) {
                        di_num_map.insert(pair<int, int>(n, 1));
                } else {
                        it->second = it->second + 1;
                }
        }
}

//822
void MBTYPES::addCoilAlarm(SrRecord &r)
{
        string number = r.value(2);
        string alarm = r.value(3);
        string alarm_condition = r.value(5);

        if (r.value(4) == "false") {
                for (auto& v : _m[_current_type].co) {
                        if (v[_m[_current_type].number] == r.value(2)) {
                                v[_m[_current_type].alarm] = alarm;
                                v[_m[_current_type].alarm_condition] = alarm_condition;

                                break;
                        }
                }
        } else {
                for (auto& v : _m[_current_type].di) {
                        if (v[_m[_current_type].number] == r.value(2)) {
                                v[_m[_current_type].alarm] = alarm;
                                v[_m[_current_type].alarm_condition] = alarm_condition;

                                break;
                        }
                }
        }
}

//824
void MBTYPES::addCoilEvent(SrRecord &r)
{
        string number = r.value(2);
        string event = r.value(3);
        string event_condition = r.value(5);

        if (r.value(4) == "false") {
                for (auto& v : _m[_current_type].co) {
                        if (v[_m[_current_type].number] == r.value(2)) {
                                v[_m[_current_type].event] = event;
                                v[_m[_current_type].event_condition] = event_condition;

                                break;
                        }
                }
        } else {
                for (auto& v : _m[_current_type].di) {
                        if (v[_m[_current_type].number] == r.value(2)) {
                                v[_m[_current_type].event] = event;
                                v[_m[_current_type].event_condition] = event_condition;

                                break;
                        }
                }
        }
}

//830
void MBTYPES::addCoilStatus(SrRecord &r)
{
        if (r.value(4) == "false") for (auto& v : _m[_current_type].co) {
                if (v[_m[_current_type].number] == r.value(2)) {
                        v[_m[_current_type].status] = "100";

                        break;
                }
        } else {
                for (auto& v : _m[_current_type].di) {
                        if (v[_m[_current_type].number] == r.value(2)) {
                                v[_m[_current_type].status] = "100";

                                break;
                        }
                }
        }
}

#if 0
void MBTYPES::saveConfigure(SrRecord &r) { //817
        printf("saveConfigure\n");

}
#endif

//825
void MBTYPES::addRegister(SrRecord &r)
{
        SSADebug("addRegister");
        string number = r.value(2);
        string startBit = r.value(3);
        string noBits = r.value(4);
        int f = atoi(r.value(5).c_str());
        int a = atoi(r.value(6).c_str());
        int c = atoi(r.value(7).c_str());
        int n = atoi(r.value(2).c_str());
        string factor = to_string((double)f / a / pow(10, c));
        string is_signed = r.value(9);
        string _is_float_type = r.value(10);

        string weight = r.value(11);
        string bias = r.value(12);
        string point = r.value(13);
	string analyticalType = r.value(14);

        /*
         * number, alarm, event, status, sb, nb, factor, measurement, is_signed, is_float_type, is_forward,
         *      weight, bias, point, analyticalType
         */
        vector<string> v = {number, "", "", "", startBit, noBits, factor, "", is_signed,
                _is_float_type, "", weight, bias, point, analyticalType};
        int bytes = atoi(noBits.c_str()) / 16;

        if (r.value(8) == "false") {
                _m[_current_type].hr.push_back(v);
                map<int, int>& hr_num_map = TYPENUM[_current_type][hr_pos];
                int length = hr_num_map.size();
                map<int, int>::reverse_iterator it = hr_num_map.rbegin();

                if (it != hr_num_map.rend()) {
                }
                if (hr_num_map.size() && it->second >= 125) {
                        length = 0;
                }

                if (!length) {
                        hr_num_map.insert(pair<int, int>(n, bytes));
                } else if (n > it->first + it->second) {
                        hr_num_map.insert(pair<int, int>(n, bytes));
                } else if (n == it->first + it->second) {
                        it->second = it->second + bytes;
                } else {
                        if (n == it->first + it->second - 1) {
                                it->second = it->second + bytes;
                        } else {
                                SSADebug("寄存器不是连续的。");
                        }

                }
        } else {
                _m[_current_type].ir.push_back(v);
                map<int, int>& ir_num_map = TYPENUM[_current_type][ir_pos];
                int length = ir_num_map.size();
                map<int, int>::reverse_iterator it = ir_num_map.rbegin();

                if (ir_num_map.size() && it->second >= 125) {
                        length = 0;
                }

                if (!length) {
                        ir_num_map.insert(pair<int, int>(n, bytes));
                } else if (n > it->first + it->second) {
                        ir_num_map.insert(pair<int, int>(n, bytes));
                } else if (n == it->first + it->second) {
                        it->second = it->second + bytes;
                } else {
                        if (n == it->first + it->second - 1) {
                                it->second = it->second + bytes;
                        } else {
                                SSADebug("寄存器不是连续的。");
                        }
                }

        }
}

//826
void MBTYPES::addRegisterAlarm(SrRecord &r)
{
        SSADebug("addRegisterAlarm");
        string number = r.value(2);
        string startBit = r.value(3);
        string alarm = r.value(4);

        if (r.value(5) == "false") {
                for (auto& v : _m[_current_type].hr) {
                        if (v[_m[_current_type].number] == number
                                && v[_m[_current_type].startBits] == startBit) {
                                v[_m[_current_type].alarm] = alarm;

                                break;
                        }
                }
        } else {
                for (auto& v : _m[_current_type].ir) {
                        if (v[_m[_current_type].number] == number
                                && v[_m[_current_type].startBits] == startBit) {
                                v[_m[_current_type].alarm] = alarm;

                                break;
                        }
                }

        }
}

//827
void MBTYPES::addRegisterMeasurement(SrRecord &r)
{
        SSADebug("addRegisterMeasurement");
        string number = r.value(2);
        string startBit = r.value(3);
        string measurement = r.value(4);
        string is_forward = r.value(6);

        if (r.value(5) == "false") {
                for (auto& v : _m[_current_type].hr) {
                        if (v[_m[_current_type].number] == number
                                && v[_m[_current_type].startBits] == startBit) {
                                v[_m[_current_type].measurement] = measurement;
                                v[_m[_current_type].is_forward] = is_forward;

                                break;
                        }
                }
        } else {
                for (auto& v : _m[_current_type].ir) {
                        if (v[_m[_current_type].number] == number
                                && v[_m[_current_type].startBits] == startBit) {
                                v[_m[_current_type].measurement] = measurement;
                                v[_m[_current_type].is_forward] = is_forward;

                                break;
                        }
                }
        }

}

//828
void MBTYPES::addRegisterEvent(SrRecord &r)
{
        SSADebug("addRegisterEvent");
        string number = r.value(2);
        string startBit = r.value(3);
        string event = r.value(4);

        if (r.value(5) == "false") {
                for (auto& v : _m[_current_type].hr) {
                        if (v[_m[_current_type].number] == number
                                && v[_m[_current_type].startBits] == startBit) {
                                v[_m[_current_type].event] = event;

                                break;
                        }
                }
        } else {
                for (auto& v : _m[_current_type].ir) {
                        if (v[_m[_current_type].number] == number
                                && v[_m[_current_type].startBits] == startBit) {
                                v[_m[_current_type].event] = event;

                                break;
                        }
                }
        }
}

//831
void MBTYPES::addRegisterStatus(SrRecord &r)
{
        SSADebug("addRegisterStatus");
        string number = r.value(2);
        string startBit = r.value(3);

        if (r.value(5) == "false") {
                for (auto& v : _m[_current_type].hr) {
                        if (v[_m[_current_type].number] == number
                                && v[_m[_current_type].startBits] == startBit) {
                                v[_m[_current_type].status] = "101";

                                break;
                        }
                }
        } else {
                for (auto& v : _m[_current_type].ir) {
                        if (v[_m[_current_type].number] == number
                                && v[_m[_current_type].startBits] == startBit) {
                                v[_m[_current_type].status] = "101";

                                break;
                        }
                }
        }
}

//833
void MBTYPES::setCoil(SrRecord &r)
{
        SSADebug("setCoil");
}

//834
void MBTYPES::setRegister(SrRecord &r)
{
        SSADebug("setRegister");
}

//839
void MBTYPES::setmbtype(SrRecord &r)
{
        printf("setmbtype: + %s\n", r.value(0).c_str());
        _current_type = r.value(2);

        if (r.value(0) == "839") {		// 839
                _m[_current_type].co.clear();
                _m[_current_type].di.clear();
        } else {				// 840
                _m[_current_type].hr.clear();
                _m[_current_type].ir.clear();
        }

        _m[_current_type].is_server_time = "false";
}

// 829
void MBTYPES::setServerTime(SrRecord& r)
{
        this->_m[_current_type].is_server_time = r.value(2);
}

void MBTYPES::operator()(SrRecord &r, SrAgent &agent)
{
        SSADebug("MBTYPES operator()");
        m_mod[r.value(0)](*this, r);
}

MBDEVICES::MBDEVICES(SrAgent& agent, MBTYPES& mbtypes)
        : _agent(agent), _mbtypes(mbtypes)
{
        m_mod["816"] = &MBDEVICES::addDevice;
        m_mod["847"] = &MBDEVICES::addDevice;
        m_mod["832"] = &MBDEVICES::addDevice;
        m_mod["848"] = &MBDEVICES::addDevice;
}

//816 847 832 848
int MBDEVICES::addDevice(SrRecord &r)
{
        string slave = r.value(3);
        string sid = r.value(4);
        string name = r.value(6);
        string s_type;
        size_t pos = r.value(5).find("/inventory/managedObjects/");

        if (pos != string::npos) {
                pos += sizeof("/inventory/managedObjects/") - 1;
                s_type = r.value(5).substr(pos, r.value(5).size() - pos);
        }

        SSADebug("stype: " + s_type);
        string addr;
        string ops_id;

        if (r.size() == 9) {
                ops_id = r.value(8);
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
        _m[sid].name = name;
        _m[sid].addr = addr;
        _m[sid].slave = slave;
        _m[sid].type = s_type;
        _m[sid].ops_id = ops_id;

        if (_m.find(s_type) != _m.end()) {
                map<string, vector<map<int, int>>>::iterator it = _mbtypes.TYPENUM.find(
                        s_type);
                _mbtypes.TYPENUM.erase(s_type);
        }

        struct MbTypes mt;
        _mbtypes._m[s_type] = mt;

        for (int i = 0; i < 4; i++) {
                map<int, int> m;
                _mbtypes.TYPENUM[s_type].push_back(m);
        }

        return 0;
}

void MBDEVICES::clear(const string& sid)
{
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

void MBDEVICES::operator()(SrRecord &r, SrAgent &agent)
{
        SSADebug("MBDEVICES operator()");
        m_mod[r.value(0)](*this, r);
}

