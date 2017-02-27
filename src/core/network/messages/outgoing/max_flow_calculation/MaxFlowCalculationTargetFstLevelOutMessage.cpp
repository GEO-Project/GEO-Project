#include "MaxFlowCalculationTargetFstLevelOutMessage.h"

MaxFlowCalculationTargetFstLevelOutMessage::MaxFlowCalculationTargetFstLevelOutMessage(
    const NodeUUID& senderUUID,
    const NodeUUID& targetUUID) :

    MaxFlowCalculationMessage(senderUUID, targetUUID) {};

const Message::MessageType MaxFlowCalculationTargetFstLevelOutMessage::typeID() const {

    return Message::MessageTypeID::MaxFlowCalculationTargetFstLevelOutMessageType;
}
