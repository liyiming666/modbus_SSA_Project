/*
 * main.cc
 *
 *  Created on: Sep 7, 2018
 *      Author: wenhan
 */

#include <stdio.h>
#include <iostream>
#include <chrono>
#include <future>
#include <unistd.h>
#include <sys/types.h>
#include "srutils.h"
#include "integrate.h"
#include "srreporter.h"
#include "srdevicepush.h"
#include <srnethttp.h>
#include "util.h"
#include "cpuinfo.h"
#include "client_session.h"
#include "io_service_thread.h"
#include "ssa_logger.h"
#include "agent_msg_handler.h"

#include <boost/thread/thread_pool.hpp>
//ing namespace boost::threadpool;

//#define USE_MQTT

#ifdef CATCH_SIGNAL

void sig_handler(int sig)
{
        if (sig == SIGINT) {
                exit(0);
        }
}

#endif

int main(void)
{
#ifdef CATCH_SIGNAL
        signal(SIGINT, sig_handler);
#endif

        string device_id, server, log_dir, port,interval, tms;
        string credential_path = "./conf/credential";
        string template_dir = "./conf/srtemplate.txt";
        readConfig("./conf/agentdtu.conf", "device_id", device_id);
        readConfig("./conf/agentdtu.conf", "server", server);
        readConfig("./conf/agentdtu.conf", "timer", tms);
        readConfig("./conf/agentdtu.conf", "log_dir", log_dir);
        readConfig("./conf/agentdtu.conf", "port", port);
        readConfig("./conf/agentdtu.conf", "interval", interval);

        //curl_global_init(CURL_GLOBAL_ALL);

        string srversion, srtemplate;
        if (0 != readSrTemplate(template_dir, srversion, srtemplate)) {
                SSAError("read template failed");
                return 0;
        }

        Integrate igt;
        SrAgent agent(server, device_id, &igt);
        setLogLevel(DEBUG);
        srLogSetLevel(SRLOG_DEBUG);

        if (0 != agent.bootstrap(credential_path)) {
                SSAError("create agent credential_path failed");
                return 0;
        }

        if (0 != agent.integrate(srversion, srtemplate)) {
                SSAError("parent device integrate failed");
                return 0;
        }

        CpuInfo ci;
        SrTimer cit(120*1000, &ci);
        agent.addTimer(cit);
        cit.start();

        SrReporter reporter(server, agent.XID(),
                            agent.auth(), agent.egress, agent.ingress);
        SrDevicePush push(server, agent.XID(),
                         agent.auth(), agent.ID(), agent.ingress);

        if (reporter.start() || push.start()) {
                return 0;
        }

        SrNetHttp http(agent.server() + "/s", srversion, agent.auth());
        AgentMsgHandler amh(agent, &http);

        IOServiceThread thrd(agent, atoi(port.c_str()), &http,
                             srversion, atoi(tms.c_str()));
        thrd.Start();

        agent.loop();

        return 0;
}


