/*
 * modbus_model.h
 *
 *  Created on: Sep 7, 2018
 *      Author: wenhan
 */

#ifndef MODBUS_MODEL_H_
#define MODBUS_MODEL_H_

#include <string>

#include "sragent.h"

#include "map.h"

using namespace std;

struct MbDevices {
        map<string, vector<string>> template_msg;
        map<string, vector<string>> co; // 线圈
        map<string, vector<string>> di; // 离散输入量
        map<string, vector<string>> hr; // 保持寄存器
        map<string, vector<string>> ir; // 输入寄存器
        string addr; // ip地址
        string slave; // 设备的modbus地址
        string type; // 设备对应的设备数据库模型的系统id
        string ops_id;

        // The total number of coils currently processed
        size_t co_num = 0;

        // The total number of discrete inputs currently processed
        size_t di_num = 0;

        // The total number of holding registers currently processed
        size_t hr_num = 0;

        // The total number of input registers currently processed
        size_t ir_num = 0;
};

struct MbTypes {
        vector<vector<string>> co;//线圈
        vector<vector<string>> di;//离散输入量
        vector<vector<string>> hr;//保持寄存器
        vector<vector<string>> ir;//输入寄存器
        int number = 0;           // number position
        int alarm = 1;            // alarm template position
        int event = 2;            // event template position
        int status = 3;           // status template position

        int startBits = 4;      // start bits position
        int noBits = 5;         // number of bits position
        int factor = 6;       // factor position
        int measurement = 7;    // measurement template position
        int sign = 8;          // signed or unsigned position
        string is_server_time;
};


class MBTYPES: public SrMsgHandler{
public:
        MBTYPES(void);

        void addCoil(SrRecord &r); //821

        void addCoilAlarm(SrRecord &r); //822

        void addCoilEvent(SrRecord &r); //824

        void addCoilStatus(SrRecord &r); //830

        void saveConfigure(SrRecord &r); //817

        void addRegister(SrRecord &r); //825

        void addRegisterAlarm(SrRecord &r); //826

        void addRegisterMeasurement(SrRecord &r); //827

        void addRegisterEvent(SrRecord &r); //828

        void addRegisterStatus(SrRecord &r); //831

        void setCoil(SrRecord &r); //833

        void setRegister(SrRecord &r); //834

        void setmbtype(SrRecord &r); //839

        void setServerTime(SrRecord& r); // 829

        virtual void operator()(SrRecord &r, SrAgent &agent);

public:
        map <string, struct MbTypes> _m; //key:type value:通过接口调的模型
        map<string ,function<void(MBTYPES&, SrRecord&)>> m_mod;
	map<string, vector<map<int, int>>> TYPENUM;
	int co_pos = 0;
	int di_pos = 1;
	int hr_pos = 2;
	int ir_pos = 3;
private:
        struct MBTYPES* _mt;
        string _current_type;
};

class MBDEVICES:public SrMsgHandler{
public:
        MBDEVICES(SrAgent& agent);

        int addDevice(SrRecord &r); //816 847 832 848

	void clear(const string& sid);

        virtual void operator()(SrRecord &r ,SrAgent &agent);
public:
        map <string, struct MbDevices> _m;//key :sid value :结构体
private:
        map<string ,function<void(MBDEVICES&, SrRecord&)>> m_mod;
        SrAgent &_agent;
};

#endif /* MODBUS_MODEL_H_ */
