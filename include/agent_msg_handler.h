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
#include "client_session.h"

class AgentMsgHandler : public SrMsgHandler {
public:
        AgentMsgHandler(SrAgent& agent, SrNetHttp* http);

        /* 析构函数 */
        ~AgentMsgHandler();

        void init();

public:
        virtual void operator()(SrRecord &r, SrAgent &agent);

        void setValue(string sid, SrRecord& r, pair<string, struct MbDevices> mbd);

	SrNetHttp* getHttp(){
		return _http;
	}
private:
        SrAgent& _agent;
        SrNetHttp* _http;
};

#endif /* AGENT_MSG_HANDLER_H_ */
