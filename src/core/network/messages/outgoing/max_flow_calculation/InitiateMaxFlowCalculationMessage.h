//
// Created by mc on 14.02.17.
//

#ifndef GEO_NETWORK_CLIENT_INITIATEMAXFLOWCALCULATIONMESSAGE_H
#define GEO_NETWORK_CLIENT_INITIATEMAXFLOWCALCULATIONMESSAGE_H

#include "../../base/max_flow_calculation/MaxFlowCalculationMessage.h"
#include "../../result/MessageResult.h"

class InitiateMaxFlowCalculationMessage : public MaxFlowCalculationMessage {

public:
    typedef shared_ptr<InitiateMaxFlowCalculationMessage> Shared;

public:
    InitiateMaxFlowCalculationMessage(
            NodeUUID &targetUUID);

    const MessageType typeID() const;

};


#endif //GEO_NETWORK_CLIENT_INITIATEMAXFLOWCALCULATIONMESSAGE_H
