/*
 * define.h
 *
 *  Created on: Sep 7, 2018
 *      Author: wenhan
 */

#ifndef DEFINE_H_
#define DEFINE_H_

#include <stdint.h>

/* Fields are used for duplication of the modbus library, using namespace */
namespace SSADefine{


        enum TF {
                FALSE,
                TRUE
        };

}

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

typedef bool Bool;
#define BUFSIZE 1024

#define CONNECTED           0
#define DISCONNECTED        1

#endif /* DEFINE_H_ */
