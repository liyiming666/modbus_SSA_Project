/*
 * ssa_logger.h
 *
 *  Created on: Sep 7, 2018
 *      Author: wenhan
 */

#ifndef SSA_LOGGER_H_
#define SSA_LOGGER_H_

#include <log4cplus/logger.h>
#include <log4cplus/configurator.h>
#include <log4cplus/loggingmacros.h>   // 日志输出宏
#include <log4cplus/consoleappender.h> // 将日志输出到控制台
#include <log4cplus/fileappender.h> // 将日志输出到文件
#include <log4cplus/layout.h>       // 输出格式控制
#include <log4cplus/loglevel.h>
#include <iomanip>
#include <unistd.h>

#include <memory>
#include <iostream>


enum LoggerLevel {
        TRACE = 0,
        DEBUG,
        INFO,
        WARN,
        ERROR,
        FATAL
};

class SSALogger : public log4cplus::Logger {
public:
        SSALogger(const std::string& name,
                  const std::string& pattern="%D{20%y-%m-%d %H:%M:%S} %p: %m%n"
                );
        virtual ~SSALogger();

public:
        void init();
        void setLogLevel(LoggerLevel lvl);
        void log(LoggerLevel lvl, const std::string& msg);
private:
        std::string _pattern;
        log4cplus::helpers::SharedObjectPtr<log4cplus::Appender> _append;
        std::auto_ptr<log4cplus::Layout> _layout;
        std::map<int, int> _lvl;
};

void setLogLevel(LoggerLevel lvl);

void SSATrace(const std::string& msg);

void SSADebug(const std::string& msg);

void SSAInfo(const std::string& msg);

void SSAWarn(const std::string& msg);

void SSAError(const std::string& msg);

void SSAFatal(const std::string& msg);

#endif /* SSA_LOGGER_H_ */
