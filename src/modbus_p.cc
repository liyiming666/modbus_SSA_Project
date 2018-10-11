/*
 * modbus_p.cc
 *
 *  Created on: Sep 7, 2018
 *      Author: wenhan
 */



#include <string.h>
#include <string>

#include "modbus_p.h"
#include "ssa_logger.h"

using namespace std;

#define HEARTBEAT_SIZE      3

#define REGIST_SIZE         19
#define REGIST_HEADER_SIZE  4
#define HEADER_LEN          3
#define MB_TCP_HEADER_LEN   7
#define CRC_LEN             2

extern MBTYPES mbtypes;

ModbusP::ModbusP(u8** p, u8**pos, ClientSession* cs)
        : _d(p), _pos(pos), _data(*p), _cs(cs)
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
                        if (NULL != strstr((char*)_data+4, "HTLZ")) {
								SSAInfo("receive incorrect regist did: "
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
			cout << _cs->_did + "--->heart beat." << endl;
                        SSAInfo( _cs->getClientInfo() +"|"+ _cs->_did + "--->heart beat.");
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

	for (int i = 0; i < (*_pos - _data); i++) {
		SSADebug(getHexString(_data, 12));
		printf("%02x ", _data[i]);
	}
	printf("\n");
                return 1;
        } else {
                return 0;
        }
}

int ModbusP::checkMP(const u32& len)
{
        if (_data[1] == 0x05 || _data[1] == 0x06) {
                if (len < 5+3) return (5+3 - len);
                else return 0;
        }

        if (_data[2] != 0x00) { // rtu msg
                _tcp_flag = SSADefine::FALSE;
                u8 data_len = _data[2];
                if (len < ((u32)data_len + CRC_LEN + HEADER_LEN)) {
                        return (data_len + CRC_LEN - (len-HEADER_LEN));
                } else {
                        for (u32 i = 0; i < len; i++) { // print msg
                                printf("%02x ", _data[i]);
                        }
                        printf("\n");
			u16 str = crc16(_data,len-2);
			u16 crc = (_data[len-1] << 8) + _data[len-2];
			if (crc == str ) {
				cout << "数据校验成功" << endl;
				return 0;
			} else {
				cout << "数据校验失败" << endl;
				return -2;
			}
                }
        } else {        // tcp msg
                _tcp_flag = SSADefine::TRUE;
                if (len < MB_TCP_HEADER_LEN) { // read tcp header
                        return (MB_TCP_HEADER_LEN - len);
                } else { // read tcp body
                        u16 data_len = (_data[4]<<8) + _data[5];

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
        SSADebug("=========: "+ _cs->getClientInfo() + "|"  + _cs->_did + " receive: " + getHexString(_data, (*_pos-_data)));

        if (_tcp_flag) {       // tcp msg

        } else {                // rtu msg
                u8 fc = 0x00;
                if (_tcp_flag) fc = _data[7];
                else fc = _data[1];
//                u8& slave = _data[0];
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

int ModbusP::parseCO()
{
        u8 d = 0x00;
        u8 slave = 0x00;
        string sid;
        if (_tcp_flag) {
                //d = _data[9];
                slave = _data[6];
                sid = _cs->_tcp_slave_to_sid[to_string(slave)];
        } else {
                //d = _data[3];
                slave = _data[0];
                sid = _cs->_rtu_slave_to_sid[to_string(slave)];
        }

	int cn = atoi(_cs->_current_num.c_str());
        struct MbTypes& t = mbtypes._m[_cs->_mbdevices._m[sid].type];
	map<int, int>&co = mbtypes.TYPENUM[_cs->_mbdevices._m[sid].type][mbtypes.co_pos];
	const int& count = co[cn];

	int len = _data[2];

	u32 data = 0x00;
        u8* p = _data+3;

        for (int i = 0; i < len; i++) {
                data += *p << (i * 8);
		p += 1;
        }

	for (int j=0; j<count; j++){
		d = (data >> j) & 0x01;
        	for (auto& v: t.co) {
                	if (v[t.number] == to_string(cn + j)) { // upload specific coil status
                        	_cs->_mbdevices._m[sid].co_num += 1;
                        	map<string, vector<string>>& mp = _cs->_mbdevices._m[sid].co;
                        	map<string, vector<string>>& tm = _cs->_mbdevices._m[sid].template_msg;

                        	const string& alarm = v[t.alarm];
                       		const string& event = v[t.event];
                        	const string& status = v[t.status];

                        	if (!alarm.empty()) {
                        		if (d) {
                                	vector<string> vt;
                                	mp[alarm] = vt;
                                	tm[alarm] = vt;
                        		}
                        	}

                        	if (!event.empty()) {
                                	mp[event].push_back("," + to_string(d));
                                	tm[event].push_back("," + to_string(d));
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
        string sid;
        if (_tcp_flag) {
                //d = _data[9];
                slave = _data[6];
                sid = _cs->_tcp_slave_to_sid[to_string(slave)];
        } else {
                //d = _data[3];
                slave = _data[0];
                sid = _cs->_rtu_slave_to_sid[to_string(slave)];
        }

        int cn = atoi(_cs->_current_num.c_str());
        struct MbTypes& t = mbtypes._m[_cs->_mbdevices._m[sid].type];
	map<int, int>&di = mbtypes.TYPENUM[_cs->_mbdevices._m[sid].type][mbtypes.di_pos];
	const int& count = di[cn];

	int len = _data[2];
	u32 data = 0x00;
	u8* p = _data+3;

	for (int i =0; i < len ; i++) {
		data += *p << (i * 8);
		p += 1;
	}

	for (int j =0; j< count ; j++) {
		d = (data >> j) & 0x01;
        	for (auto& v: t.di) {
                	if (v[t.number] == to_string(cn + j)) { // upload specific di status
                        	_cs->_mbdevices._m[sid].di_num += 1;
                        	map<string, vector<string>>& mp = _cs->_mbdevices._m[sid].di;
                        	map<string, vector<string>>& tm = _cs->_mbdevices._m[sid].template_msg;

                        	const string& alarm = v[t.alarm];
                        	const string& event = v[t.event];
                        	const string& status = v[t.status];

                        	if (!alarm.empty()) {
                        		if (d) {
                                	vector<string> vt;
                                	mp[alarm] = vt;
                                	tm[alarm] = vt;
                        		}
                        	}

                        	if (!event.empty()) {
                                	mp[event].push_back("," + to_string(d));
                                	tm[event].push_back("," + to_string(d));
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
        int d = 0x00;
        u8 slave = 0x00;
        string sid;
        if (_tcp_flag) {
                //d = (_data[9]<<8) + _data[10];
                slave = _data[6];
                sid = _cs->_tcp_slave_to_sid[to_string(slave)];
        } else {
                //d = (_data[3]<<8) + _data[4];
                slave = _data[0];
                sid = _cs->_rtu_slave_to_sid[to_string(slave)];
        }

        int cn = atoi(_cs->_current_num.c_str());
	struct MbTypes& t = mbtypes._m[_cs->_mbdevices._m[sid].type];
	map<int, int>& hr= mbtypes.TYPENUM[_cs->_mbdevices._m[sid].type][mbtypes.hr_pos];
	const int& count = hr[cn];

	int len = _data[2];

	u8* p = _data+3;
	for (int i =0; i < (len/2); i++){
		d = (*p << 8 ) + *(p+1);
        for (auto& v: t.hr) {
                if (v[t.number] == to_string(cn + i)) { // upload specific di status
                        _cs->_mbdevices._m[sid].hr_num += 1;
                        map<string, vector<string>>& mp = _cs->_mbdevices._m[sid].hr;
                        map<string, vector<string>>& tm = _cs->_mbdevices._m[sid].template_msg;
                        int d1 = d;
                        int sb = atoi(v[t.startBits].c_str());
                        int nb = atoi(v[t.noBits].c_str());
                        string is_signed = v[t.sign];
                        d1 = bat(d1, sb, nb, is_signed);
                        double data = (double)d1 * atof(v[t.factor].c_str());
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
                                mp[event].push_back("," + to_string(data));
                                tm[event].push_back("," + to_string(data));
                        }

                        if (!measurement.empty()) {
                                mp[measurement].push_back("," + to_string(data));
                                tm[measurement].push_back("," + to_string(data));
                        }
                        if (!status.empty()) {
                                if (tm.find(status) == mp.end()) {
                                        vector<string> vec = {"," + to_string(data)};
                                        mp[status] = vec;
                                        tm[status] = vec;
                                } else {
                                        mp[status].push_back("," + to_string(data));
                                        tm[status].push_back("," + to_string(data));
                                }
                        }
                }
        }
	p += 2;
	}
        return 0;
}

int ModbusP::parseIR()
{
        u16 d = 0x00;
        u8 slave = 0x00;
        string sid;
        if (_tcp_flag) {
                //d = (_data[9]<<8) + _data[10];
                slave = _data[6];
                sid = _cs->_tcp_slave_to_sid[to_string(slave)];
        } else {
                //d = (_data[3]<<8) + _data[4];
                slave = _data[0];
                sid = _cs->_rtu_slave_to_sid[to_string(slave)];
        }

        int cn = atoi(_cs->_current_num.c_str());
        struct MbTypes& t = mbtypes._m[_cs->_mbdevices._m[sid].type];
	map<int, int>& ir= mbtypes.TYPENUM[_cs->_mbdevices._m[sid].type][mbtypes.ir_pos];
	const int& count = ir[cn];

	int len = _data[2];

	u8* p = _data+3;
	for (int i =0; i < (len/2); i++){
		d = (*p << 8 ) + *(p+1);
        for (auto& v: t.ir) {
                if (v[t.number] == to_string(cn + i)) { // upload specific di status
                        _cs->_mbdevices._m[sid].ir_num += 1;
                        map<string, vector<string>>& mp = _cs->_mbdevices._m[sid].ir;
                        map<string, vector<string>>& tm = _cs->_mbdevices._m[sid].template_msg;
                        u16 d1 = d;
                        int sb = atoi(v[t.startBits].c_str());
                        int nb = atoi(v[t.noBits].c_str());
                        string is_signed = v[t.sign];
                        d1 = bat(d1, sb, nb, is_signed);
                        double data = (double)d1 * atof(v[t.factor].c_str());
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
                                mp[event].push_back("," + to_string(data));
                                tm[event].push_back("," + to_string(data));
                        }
                        if (!measurement.empty()) {
                                mp[measurement].push_back("," + to_string(data));
                                tm[measurement].push_back("," + to_string(data));
                        }
                        if (!status.empty()) {
                                if (tm.find(status) == mp.end()) {
                                        vector<string> vec = {"," + to_string(data)};
                                        mp[status] = vec;
                                        tm[status] = vec;
                                } else {
                                        mp[status].push_back("," + to_string(data));
                                        tm[status].push_back("," + to_string(data));
                                }
                        }
                }
        }
	p += 2;
	}
        return 0;
}


