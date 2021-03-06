/*
 * util.h
 *
 *  Created on: Sep 7, 2018
 *      Author: wenhan
 */

#ifndef UTIL_H_
#define UTIL_H_

#include "sragent.h"
#include <string>
#include <string.h>
#include <iomanip>
#include <math.h>

#include "define.h"
using namespace std;

#pragma pack(push, 1)

struct Header {
        uint32_t id;
        uint16_t version;
        uint16_t addr;
        uint8_t  type;
        uint8_t  model;
        uint8_t  flag;
};

struct Data {
        uint8_t start_flag;
        uint8_t type;
        uint8_t custom;
        uint16_t multiple;
        uint8_t len;
};

#pragma pack(pop) // 恢复先前的pack设置

/* 读取配置文件 */
void readConfig(const char* path, const string& key, string& out_ptr);

/* 向平台获取deviceID对应的系统ID */
//string getSid(const SrAgent& agent, const string& srv, const string& did);

uint16_t crc16(uint8_t *str, uint16_t len);

/* 获取SSA本地时间 */
string getServerTime();

string getHexString(u8* s, const u32& len);

int bat(int d, int& sb, int& nb, string& is_signed);

u16 unbat(u16 old, int& sb, int& nb, int& value);
	


unsigned int getbitu(const unsigned char *buff, int pos, int len);

float HexToFloat(const unsigned char *buf);

double HexToDouble(const unsigned char* buf);

double calcFactor(float a);

/* 获取毫秒级时间 */
long getMilliseconds();

/* 计算浮点型需要保留的小数位，返回一个string型的数据 */
string floatReserve(const double& f, const int& point);

#endif /* UTIL_H_ */
