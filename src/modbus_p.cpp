/*
 * modbus_p.cc
 *
 *  Created on: Sep 7, 2018
 *      Author: wenhan
 */

#include <string.h>
#include <string>
#include <iterator>

#include "modbus_p.h"
#include "ssa_logger.h"

using namespace std;

#define HEARTBEAT_SIZE      3

#define REGIST_SIZE         19
#define REGIST_HEADER_SIZE  4
#define HEADER_LEN          3
#define MB_TCP_HEADER_LEN   7
#define CRC_LEN             2

//extern MBTYPES mbtypes;

// 默认为rtu模式
ModbusP::ModbusP(u8** p, u8**pos, ClientSession* cs)
        : _d(p), _pos(pos), _data(*p), _cs(cs), _tcp_flag(SSADefine::FALSE)
{

}

ModbusP::~ModbusP()
{

}

void ModbusP::init()
{
        _data = *_d;
}

int ModbusP::checkReg()
{
        int ret_len = 0;
        u32 len = *_pos - _data;

        if (len < REGIST_SIZE) {
                ret_len = REGIST_SIZE - len;
        } else {
                SSAInfo(_cs->getClientInfo() + "|" + getHexString(_data, len));

                if (strncmp((char*)_data, "HTLZ", REGIST_HEADER_SIZE)) {
                        SSAWarn("invalid regist package's flag");
                        ret_len = -1;
                } else {
                        // 防止设备连线后，先发心跳包导致取出的设备id是ETUNG
                        if (NULL != strstr((char*)_data + 4, "HTLZ")) {
                                SSAInfo(
                                        "receive incorrect regist did: "
                                                + string((char*)_data));
                                ret_len = -1;
                        } else {		// 注册包发送正确
                                ret_len = 0;
                                _cs->_did = string((char*)_data).substr(
                                REGIST_HEADER_SIZE,
                                REGIST_SIZE - REGIST_HEADER_SIZE);

                                SSAInfo(_cs->_did + " connect:" + _cs->getClientInfo());
                        }
                }
        }

        return ret_len;
}

int ModbusP::checkMsg()
{
        u32 len = *_pos - _data;

        if (len < HEADER_LEN) { // read header, compare is tcp or rtu
                return (HEADER_LEN - len);
        } else {
                if (checkHB()) {
                        SSAInfo(
                                _cs->getClientInfo() + "|" + _cs->_did
                                        + "--->heart beat.");
                        _cs->_http->post("107," + _cs->_sid);
                        return -1;
                } else {
                        return checkMP(len);
                }
        }
}

int ModbusP::checkHB()
{
        if (!strncmp((char*)_data, "LZH", HEARTBEAT_SIZE)) {
                SSADebug(getHexString(_data, (*_pos - _data)));
                return 1;
        } else {
                return 0;
        }
}

int ModbusP::checkMP(const u32& len)
{
        if (_data[1] == 0x05 || _data[1] == 0x06) {
                if (len < 5 + 3) return (5 + 3 - len);
                else return 0;
        }

        if (_data[2] != 0x00) { // rtu msg
                _tcp_flag = SSADefine::FALSE;
                u8 data_len = _data[2];

                if (_data[1] > 0x80) {	//表示modbus的错误码，一共收5字节
                        if (len == 5) {
                                //crc校验
                                u16 str = crc16(_data, len - 2);
                                u16 crc = (_data[len - 1] << 8) + _data[len - 2];
                                if (crc == str) {
                                        SSADebug(
                                                _cs->getClientInfo()
                                                        + "返回modbus错误功能码,-数据校验成功");
                                        return 0;
                                } else {
                                        SSADebug(
                                                _cs->getClientInfo()
                                                        + "返回modbus错误功能码,-数据校验失败");
                                        return -2;
                                }
                        } else {
                                return 5 - len;
                        }

                } else { 		//表示功能码为 0x01-0x04
                        if (len < ((u32)data_len + CRC_LEN + HEADER_LEN)) {
                                return (data_len + CRC_LEN - (len - HEADER_LEN));
                        } else {
                                SSADebug(
                                        _cs->getClientInfo() + " receive: "
                                                + getHexString(_data, len));

                                u16 str = crc16(_data, len - 2);
                                u16 crc = (_data[len - 1] << 8) + _data[len - 2];
                                if (crc == str) {
                                        SSADebug(_cs->getClientInfo() + "-数据校验成功");
                                        return 0;
                                } else {
                                        SSADebug(_cs->getClientInfo() + "-数据校验失败");
                                        return -2;
                                }
                        }
                }
        } else {        // tcp msg
                _tcp_flag = SSADefine::TRUE;
                if (len < MB_TCP_HEADER_LEN) { // read tcp header
                        return (MB_TCP_HEADER_LEN - len);
                } else { // read tcp body
                        u16 data_len = (_data[4] << 8) + _data[5];

                        if (len < data_len) {
                                return (data_len - len);
                        } else {
                                /* 此处应有对modbus rtu的校验 */

                                return 0;
                        }
                }
        }
}

int ModbusP::parseMsg()
{
        int ret = 0;
        SSADebug(
                "=========: " + _cs->getClientInfo() + "|" + _cs->_did + " receive: "
                        + getHexString(_data, (*_pos - _data)));

        if (_tcp_flag) {       // tcp msg

        } else {                // rtu msg
                u8 fc = 0x00;
                if (_tcp_flag) {
                        fc = _data[7];
                } else {
                        fc = _data[1];
                }

                switch (fc) {
                case 0x01:      // read co
                        ret = this->parseCO();

                        break;
                case 0x02:      // read di
                        ret = this->parseDI();

                        break;
                case 0x03:      // read hr
                        ret = this->parseHR();

                        break;
                case 0x04:      // read ir
                        ret = this->parseIR();

                        break;
                default:
                        ret = -1;
                        break;
                }
        }

        return ret;
}

unsigned long ModbusP::getBuf()
{
        return 0;
}

string ModbusP::getSid()
{
        u8 d = 0x00;
        u8 slave = 0x00;
        string sid = "";

        if (_tcp_flag) {
                //d = _data[9];
                slave = _data[6];
                sid = _cs->_tcp_slave_to_sid[to_string(slave)];
        } else {
                //d = _data[3];
                slave = _data[0];
                vector<std::string>::iterator it =
                        _cs->_rtu_slave_to_sid[to_string(slave)].begin();
                for (; it != _cs->_rtu_slave_to_sid[to_string(slave)].end(); it++) {
                        if (*it == _cs->_current_sid) {
                                return *it;
                        }
                }
        }

        return sid;
}

int ModbusP::parseCO()
{
        u8 d = 0x00;
        u8 slave = 0x00;
        string sid = getSid();

        int cn = atoi(_cs->_current_num.c_str());
        struct MbTypes& t = _cs->_mbtypes._m[_cs->_mbdevices._m[sid].type];
        map<int, int>&co =
                _cs->_mbtypes.TYPENUM[_cs->_mbdevices._m[sid].type][_cs->_mbtypes.co_pos];
        const int& count = co[cn];
        int len = _data[2];

        u32 data = 0x00;
        u8* p = _data + 3;

        for (int i = 0; i < len; i++) {
                data += *p << (i * 8);
                p += 1;
        }
        for (int j = 0; j < count; j++) {
                d = (data >> j) & 0x01;
                for (auto& v : t.co) {
                        if (v[t.number] == to_string(cn + j)) { // upload specific coil status
                                _cs->_mbdevices._m[sid].co_num += 1;
                                map<string, vector<string>>& mp =
                                        _cs->_mbdevices._m[sid].co;
                                map<string, vector<string>>& tm =
                                        _cs->_mbdevices._m[sid].template_msg;

                                const string& alarm = v[t.alarm];
                                const string& event = v[t.event];
                                const string& status = v[t.status];
                                const string& alarm_condition = v[t.alarm_condition];
                                const string& event_condition = v[t.event_condition];

                                if (!alarm.empty()) {
                                        if (alarm_condition == "0") {       // 0报警
                                                if (!d) {
                                                        vector<string> vt;
                                                        mp[alarm] = vt;
                                                        tm[alarm] = vt;
                                                }
                                        } else {                            // 1报警
                                                if (d) {
                                                        vector<string> vt;
                                                        mp[alarm] = vt;
                                                        tm[alarm] = vt;
                                                }
                                        }
                                }

                                if (!event.empty()) {
                                        if (event_condition == "0") {       // 0事件
                                                if (!d) {
                                                        mp[event].push_back(
                                                                "," + to_string(d));
                                                        tm[event].push_back(
                                                                "," + to_string(d));
                                                }

                                        } else {                            // 1事件
                                                if (d) {
                                                        mp[event].push_back(
                                                                "," + to_string(d));
                                                        tm[event].push_back(
                                                                "," + to_string(d));
                                                }
                                        }
                                }

                                if (!status.empty()) {
                                        if (tm.find(status) == mp.end()) {
                                                vector<string> vec = {"," + to_string(d)};
                                                mp[status] = vec;
                                                tm[status] = vec;
                                        } else {
                                                mp[status].push_back("," + to_string(d));
                                                tm[status].push_back("," + to_string(d));
                                        }
                                }
                        }
                }
        }
        return 0;
}

int ModbusP::parseDI()
{
        u8 d = 0x00;
        u8 slave = 0x00;
        string sid = getSid();

        int cn = atoi(_cs->_current_num.c_str());
        struct MbTypes& t = _cs->_mbtypes._m[_cs->_mbdevices._m[sid].type];
        map<int, int>&di =
                _cs->_mbtypes.TYPENUM[_cs->_mbdevices._m[sid].type][_cs->_mbtypes.di_pos];
        const int& count = di[cn];

        int len = _data[2];
        u32 data = 0x00;
        u8* p = _data + 3;

        for (int i = 0; i < len; i++) {
                data += *p << (i * 8);
                p += 1;
        }

        for (int j = 0; j < count; j++) {
                d = (data >> j) & 0x01;

                for (auto& v : t.di) {
                        if (v[t.number] == to_string(cn + j)) { // upload specific di status
                                _cs->_mbdevices._m[sid].di_num += 1;
                                map<string, vector<string>>& mp =
                                        _cs->_mbdevices._m[sid].di;
                                map<string, vector<string>>& tm =
                                        _cs->_mbdevices._m[sid].template_msg;

                                const string& alarm = v[t.alarm];
                                const string& event = v[t.event];
                                const string& status = v[t.status];
                                const string& alarm_condition = v[t.alarm_condition];
                                const string& event_condition = v[t.event_condition];

                                if (!alarm.empty()) {
                                        if (alarm_condition == "0") {       // 0报警
                                                if (!d) {
                                                        vector<string> vt;
                                                        mp[alarm] = vt;
                                                        tm[alarm] = vt;
                                                }
                                        } else {                            // 1报警
                                                if (d) {
                                                        vector<string> vt;
                                                        mp[alarm] = vt;
                                                        tm[alarm] = vt;
                                                }
                                        }
                                }

                                if (!event.empty()) {
                                        if (event_condition == "0") {       // 0事件
                                                if (!d) {
                                                        mp[event].push_back(
                                                                "," + to_string(d));
                                                        tm[event].push_back(
                                                                "," + to_string(d));
                                                }

                                        } else {                            // 1事件
                                                if (d) {
                                                        mp[event].push_back(
                                                                "," + to_string(d));
                                                        tm[event].push_back(
                                                                "," + to_string(d));
                                                }
                                        }
                                }

                                if (!status.empty()) {
                                        if (tm.find(status) == mp.end()) {
                                                vector<string> vec = {"," + to_string(d)};
                                                mp[status] = vec;
                                                tm[status] = vec;
                                        } else {
                                                mp[status].push_back("," + to_string(d));
                                                tm[status].push_back("," + to_string(d));
                                        }
                                }
                        }
                }
        }

        return 0;
}

int ModbusP::parseHR()
{
	cout << "parseHR ()" << endl;
        int d = 0x00;
        float a = 0.0;
        u8 slave = 0x00;
        string sid = getSid();

        int cn = atoi(_cs->_current_num.c_str());
        struct MbTypes& t = _cs->_mbtypes._m[_cs->_mbdevices._m[sid].type];
        int len = _data[2];
        u8* p = _data + 3;
        int cnn = 0;

        for (int i = 0; i < (len / 2); i++) {
                cnn = cn + i;

                for (auto& v : t.hr) {
			cout << "v[t.number] :" << v[t.number] << "| to_string(cnn) :" <<to_string(cnn) << endl;
                        if (v[t.number] == to_string(cnn)) { // upload specific di status
                                d = 0x00;
                                //为了防止for循环多做一次
                                //i += atoi(v[t.noBits].c_str()) / 16;
                                int bytes = atoi(v[t.noBits].c_str()) / 8;
                                SSADebug("---parseHr : -bytes : " + to_string(bytes));
				int IER = atoi(v[t.noBits].c_str());
				//增加解析方式
				int analyticalType = atoi(v[t.analyticalType].c_str());
                                if (v[t._is_float_type] == "0") {
					switch (bytes) {
						case 2:
							if (analyticalType == 0) {
								d = (*(p)<<8) + *(p+1);
							}
							else {
								d = (*(p+1)<<8) + *(p);
							}			
						case 4:
							if(analyticalType == 2) {//4321
								d = (*(p)<<24)+(*(p+1)<<16)+(*(p+2)<<8)+(*(p+3));
							}else if(analyticalType == 3) {//1234
								d = (*(p+3)<<24)+(*(p+2)<<16)+(*(p+1)<<8)+(*(p));
							}else if(analyticalType == 4) {//3412
								d = (*(p+1)<<24)+(*(p)<<16)+(*(p+3)<<8)+(*(p+2));
							}else if(analyticalType == 5){//2143
								d = (*(p+2)<<24)+(*(p+3)<<16)+(*(p)<<8)+(*(p+1));
							}
						default:
							break;
					}
					p += bytes;
                                } else {
                                        //浮点型
                                        u8* buffer = new u8[bytes];
                                        //memcpy(buffer, p, bytes);
                                        //sprintf((char*)buffer, "%x%x%x%x", *(p + 2),
                                        //        *(p + 3), *p, *(p + 1));
					memcpy(buffer, p, 4);
                                        for (int i=0;i<bytes;i++){
						cout << hex << buffer[i] << endl;
					}
                                        a = HexToFloat(buffer);
					cout << "a :" << a << endl;
                                        delete[] buffer;
                                }

                                _cs->_mbdevices._m[sid].hr_num += 1;
				
                                map<string, vector<string>>& mp =
                                        _cs->_mbdevices._m[sid].hr;
                                map<string, vector<string>>& tm =
                                        _cs->_mbdevices._m[sid].template_msg;
                                map<string, vector<string>>& fm =
                                        _cs->_mbdevices._m[sid].forward_msg;
                                /*
                                 double d1 = 0.0;
                                 if (t.get(_is_float_type) == '0') {
                                 int d1 = d;
                                 data = calcFactor(d1);
                                 } else {
                                 float d1 = a;
                                 data = calcFactor(d1);
                                 }
                                 */
                                int d1 = d;
                                int sb = atoi(v[t.startBits].c_str());
                                int nb = atoi(v[t.noBits].c_str());
                                string is_signed = v[t.sign];

                                double weight = atof(v[t.weight].c_str());
                                double bias = atof(v[t.bias].c_str());
                                int point = atoi(v[t.point].c_str());

                                double data = 0.0;

                                if (v[t._is_float_type] == "0") {
                                        d1 = bat(d1, sb, nb, is_signed);
                                        data = (double)d1 * atof(v[t.factor].c_str());
                                } else {
                                        data = (double)a;
					cout << "data : "  << data << endl;
                                }

                                data = weight * data + bias;    // 計算 y = ax + b
                                string s_data = floatReserve(data, point);
                                SSADebug("====>parseHR(): send float data: " + s_data + "|point: " + to_string(point));

                                const string& alarm = v[t.alarm];
                                const string& event = v[t.event];
                                const string& status = v[t.status];
                                const string& measurement = v[t.measurement];

                                if (!alarm.empty()) {
                                        if (data > 0.000001) {
                                                vector<string> vt;
                                                mp[alarm] = vt;
                                                tm[alarm] = vt;
                                        }
                                }

                                if (!event.empty()) {
                                        mp[event].push_back("," + s_data);
                                        tm[event].push_back("," + s_data);
                                }

                                if (!measurement.empty()) {
                                        mp[measurement].push_back("," + s_data);
                                        tm[measurement].push_back("," + s_data);

                                        if (v[t.is_forward] == "1") {
                                                fm[measurement].push_back("," + s_data);
                                        }
                                }

                                if (!status.empty()) {
                                        if (tm.find(status) == mp.end()) {
                                                vector<string> vec = {"," + s_data};
                                                mp[status] = vec;
                                                tm[status] = vec;
                                        } else {
                                                mp[status].push_back("," + s_data);
                                                tm[status].push_back("," + s_data);
                                        }
                                }

                                break;
                        }
                }
        }

        return 0;
}

int ModbusP::parseIR()
{
        u16 d = 0x00;
        float a = 0.0;
        u8 slave = 0x00;
        string sid = getSid();

        int cn = atoi(_cs->_current_num.c_str());
        struct MbTypes& t = _cs->_mbtypes._m[_cs->_mbdevices._m[sid].type];
        int len = _data[2];
        u8* p = _data + 3;

        int cnn = 0;

        for (int i = 0; i < (len / 2); i++) {
                cnn = cn + i;

		

                for (auto& v : t.ir) {
			cout << "v[t.number] :" << v[t.number] << "| to_string(cnn) :" <<to_string(cnn) << endl;
                        if (v[t.number] == to_string(cnn)) { // upload specific di status
                                d = 0x00;
                                //为了防止for循环多做一次
                                //i += atoi(v[t.noBits].c_str()) / 16;
                                int bytes = atoi(v[t.noBits].c_str()) / 8;
                                SSADebug("---parseir : -bytes : " + to_string(bytes));
				int IER = atoi(v[t.noBits].c_str());
				//增加解析方式
				int analyticalType = atoi(v[t.analyticalType].c_str());
                                if (v[t._is_float_type] == "0") {
					switch (bytes) {
						case 2:
							if (analyticalType == 0) {
								d = (*(p)<<8) + *(p+1);
							}
							else {
								d = (*(p+1)<<8) + *(p);
							}			
						case 4:
							if(analyticalType == 2) {//4321
								d = (*(p)<<24)+(*(p+1)<<16)+(*(p+2)<<8)+(*(p+3));
							}else if(analyticalType == 3) {//1234
								d = (*(p+3)<<24)+(*(p+2)<<16)+(*(p+1)<<8)+(*(p));
							}else if(analyticalType == 4) {//3412
								d = (*(p+1)<<24)+(*(p)<<16)+(*(p+3)<<8)+(*(p+2));
							}else if(analyticalType == 5){//2143
								d = (*(p+2)<<24)+(*(p+3)<<16)+(*(p)<<8)+(*(p+1));
							}
						default:
							break;
					}
					p += bytes;
                                } else {
                                        //浮点型
                                        u8* buffer = new u8[bytes];
                                        //memcpy(buffer, p, bytes);
                                        //sprintf((char*)buffer, "%x%x%x%x", *(p + 2),
                                        //        *(p + 3), *p, *(p + 1));
					memcpy(buffer, p, 4);
                                        for (int i=0;i<bytes;i++){
						cout << hex << buffer[i] << endl;
					}
                                        a = HexToFloat(buffer);
					cout << "a :" << a << endl;
                                        delete[] buffer;
                                }

                                _cs->_mbdevices._m[sid].ir_num += 1;
				
                                map<string, vector<string>>& mp =
                                        _cs->_mbdevices._m[sid].ir;
                                map<string, vector<string>>& tm =
                                        _cs->_mbdevices._m[sid].template_msg;
                                map<string, vector<string>>& fm =
                                        _cs->_mbdevices._m[sid].forward_msg;
                                /*
                                 double d1 = 0.0;
                                 if (t.get(_is_float_type) == '0') {
                                 int d1 = d;
                                 data = calcFactor(d1);
                                 } else {
                                 float d1 = a;
                                 data = calcFactor(d1);
                                 }
                                 */
                                int d1 = d;
                                int sb = atoi(v[t.startBits].c_str());
                                int nb = atoi(v[t.noBits].c_str());
                                string is_signed = v[t.sign];

                                double weight = atof(v[t.weight].c_str());
                                double bias = atof(v[t.bias].c_str());
                                int point = atoi(v[t.point].c_str());

                                double data = 0.0;

                                if (v[t._is_float_type] == "0") {
                                        d1 = bat(d1, sb, nb, is_signed);
                                        data = (double)d1 * atof(v[t.factor].c_str());
                                } else {
                                        data = (double)a;
					cout << "data : "  << data << endl;
                                }

                                data = weight * data + bias;    // 計算 y = ax + b
                                string s_data = floatReserve(data, point);
                                SSADebug("====>parseIR(): send float data: " + s_data + "|point: " + to_string(point));

                                const string& alarm = v[t.alarm];
                                const string& event = v[t.event];
                                const string& status = v[t.status];
                                const string& measurement = v[t.measurement];

                                if (!alarm.empty()) {
                                        if (data > 0.000001) {
                                                vector<string> vt;
                                                mp[alarm] = vt;
                                                tm[alarm] = vt;
                                        }
                                }

                                if (!event.empty()) {
                                        mp[event].push_back("," + s_data);
                                        tm[event].push_back("," + s_data);
                                }

                                if (!measurement.empty()) {
                                        mp[measurement].push_back("," + s_data);
                                        tm[measurement].push_back("," + s_data);

                                        if (v[t.is_forward] == "1") {
                                                fm[measurement].push_back("," + s_data);
                                        }
                                }

                                if (!status.empty()) {
                                        if (tm.find(status) == mp.end()) {
                                                vector<string> vec = {"," + s_data};
                                                mp[status] = vec;
                                                tm[status] = vec;
                                        } else {
                                                mp[status].push_back("," + s_data);
                                                tm[status].push_back("," + s_data);
                                        }
                                }

                                break;
                        }
                }




        }

        return 0;
}

