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
#include "client_session.h"
#include "io_service_thread.h"
#include "ssa_logger.h"
#include "agent_msg_handler.h"

//#define USE_MQTT

int main(void)
{
        string device_id, server, log_dir, port, tms;
        string credential_path = "./conf/credential";
        string template_dir = "./conf/srtemplate.txt";
        readConfig("./conf/agentdtu.conf", "device_id", device_id);
        readConfig("./conf/agentdtu.conf", "server", server);
        readConfig("./conf/agentdtu.conf", "hbTimerOut", tms);
        readConfig("./conf/agentdtu.conf", "log_dir", log_dir);
        readConfig("./conf/agentdtu.conf", "port", port);
        //readConfig("./conf/agentdtu.conf", "interval", interval);

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

        SrDevicePush push(server, agent.XID(), agent.auth(), agent.ID(), agent.ingress);

        if (push.start()) {
                return 0;
        }

        SrNetHttp http(agent.server() + "/s", srversion, agent.auth());
        AgentMsgHandler amh(agent, &http);
        IOServiceThread thrd(agent, atoi(port.c_str()), &http, srversion,
                atoi(tms.c_str()));
        thrd.Start();

        agent.loop();

        return 0;
}

