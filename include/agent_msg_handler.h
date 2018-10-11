/*
 * agent_msg_handler.h
 *
 *  Created on: Sep 7, 2018
 *      Author: wenhan
 */

#ifndef AGENT_MSG_HANDLER_H_
#define AGENT_MSG_HANDLER_H_

#include "sragent.h"
#include "srnethttp.h"
#include "util.h"

class AgentMsgHandler : public SrMsgHandler {
public:
        AgentMsgHandler(SrAgent& agent, SrNetHttp* http);

        /* 析构函数 */
        ~AgentMsgHandler();

        void init();

public:
        virtual void operator()(SrRecord &r, SrAgent &agent);

private:
        SrAgent& _agent;
        SrNetHttp* _http;
};

#endif /* AGENT_MSG_HANDLER_H_ */
