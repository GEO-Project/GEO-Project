#include "MaxFlowCalculationSourceFstLevelTransaction.h"

MaxFlowCalculationSourceFstLevelTransaction::MaxFlowCalculationSourceFstLevelTransaction(
    const NodeUUID &nodeUUID,
    MaxFlowCalculationSourceFstLevelMessage::Shared message,
    TrustLinesManager *trustLinesManager,
    Logger &logger) :

    BaseTransaction(
        BaseTransaction::TransactionType::MaxFlowCalculationSourceFstLevelTransactionType,
        nodeUUID,
        logger),
    mMessage(message),
    mTrustLinesManager(trustLinesManager)
{}

MaxFlowCalculationSourceFstLevelMessage::Shared MaxFlowCalculationSourceFstLevelTransaction::message() const
{
    return mMessage;
}

TransactionResult::SharedConst MaxFlowCalculationSourceFstLevelTransaction::run()
{
#ifdef DEBUG_LOG_MAX_FLOW_CALCULATION
    info() << "run\t" << "Iam: " << mNodeUUID;
    info() << "run\t" << "sender: " << mMessage->senderUUID;
    info() << "run\t" << "OutgoingFlows: " << mTrustLinesManager->outgoingFlows().size();
    info() << "run\t" << "IncomingFlows: " << mTrustLinesManager->incomingFlows().size();
#endif
    vector<NodeUUID> outgoingFlowUuids = mTrustLinesManager->firstLevelNeighborsWithOutgoingFlow();
    for (auto const &nodeUUIDOutgoingFlow : outgoingFlowUuids) {
        if (nodeUUIDOutgoingFlow == mMessage->senderUUID) {
            continue;
        }
#ifdef DEBUG_LOG_MAX_FLOW_CALCULATION
        info() << "sendFirst\t" << nodeUUIDOutgoingFlow;
#endif
        sendMessage<MaxFlowCalculationSourceSndLevelMessage>(
            nodeUUIDOutgoingFlow,
            mNodeUUID,
            mMessage->senderUUID);
    }
    return resultDone();
}

const string MaxFlowCalculationSourceFstLevelTransaction::logHeader() const
{
    stringstream s;
    s << "[MaxFlowCalculationSourceFstLevelTA: " << currentTransactionUUID() << "]";
    return s.str();
}
