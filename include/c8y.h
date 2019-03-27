/*
 * c8y.h
 *
 *  Created on: Sep 7, 2018
 *      Author: wenhan
 */

#ifndef C8Y_H_
#define C8Y_H_

#include <string>

#include "sragent.h"
#include "srnethttp.h"

#include "client_session.h"

class ClientSession;

class C8yModel {
public:
        C8yModel(SrAgent& agent, SrNetHttp** http);
        ~C8yModel();

	//通过did查询sid
        std::string getDevInfo(const std::string& did);

        //void setInteval(ClientSession* cs);

        /* Get modbus childDevice's models */
        void getChildDevices(const std::string& sid, ClientSession* cs);

        /* Get devcie-database model */
	void getModel(const std::string& type, ClientSession* cs);

        /* Set modbus params on platform */
        void setModbusParms(const std::string& sid);

        /* Used to support the platform to add models in real time */
        void addDevices(SrRecord& r, ClientSession* cs);

        /* Get model params by type */
        //void getModel(const std::string& type, ClientSession* cs);

        /* Upload data for various models based on the template of the model */
        int sendMsg(const std::string& sid, struct MbDevices& mbd, ClientSession* cs);

        /* Get the training_rate and transmit_rate parameters in the device properties */
        std::vector<int> getProperties(const std::string& sid);

private:
        SrAgent& _agent;
        SrNetHttp** _nethttp;
};

#endif /* C8Y_H_ */
