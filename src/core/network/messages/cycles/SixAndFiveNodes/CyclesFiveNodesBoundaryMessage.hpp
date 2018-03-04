#ifndef GEO_NETWORK_CLIENT_CYCLEFIVENODESBOUNDARYMESSAGE_H
#define GEO_NETWORK_CLIENT_CYCLEFIVENODESBOUNDARYMESSAGE_H

#include "base/CyclesBaseFiveOrSixNodesBoundaryMessage.h"

class CyclesFiveNodesBoundaryMessage:
    public CyclesBaseFiveOrSixNodesBoundaryMessage {
public:
    CyclesFiveNodesBoundaryMessage(
        const SerializedEquivalent equivalent,
        vector<NodeUUID> &path,
        vector<NodeUUID> &boundaryNodes) :
        CyclesBaseFiveOrSixNodesBoundaryMessage(
            equivalent,
            path,
            boundaryNodes){};

    CyclesFiveNodesBoundaryMessage(
        BytesShared buffer) :
        CyclesBaseFiveOrSixNodesBoundaryMessage(buffer){};

    const MessageType typeID() const {
        return Message::MessageType::Cycles_FiveNodesBoundary;
    };
};
#endif //GEO_NETWORK_CLIENT_CYCLEFIVENODESBOUNDARYMESSAGE_H