#ifndef GEO_NETWORK_CLIENT_RECEIVEMAXFLOWCALCULATIONTRANSACTION_H
#define GEO_NETWORK_CLIENT_RECEIVEMAXFLOWCALCULATIONTRANSACTION_H

#include "../base/BaseTransaction.h"
#include "../../../trust_lines/manager/TrustLinesManager.h"
#include "../../../network/messages/incoming/max_flow_calculation/ReceiveMaxFlowCalculationOnTargetMessage.h"
#include "../../../network/messages/outgoing/max_flow_calculation/SendResultMaxFlowCalculationMessage.h"
#include "../../../network/messages/outgoing/max_flow_calculation/SendMaxFlowCalculationTargetFstLevelMessage.h"
#include "../../scheduler/TransactionsScheduler.h"

class ReceiveMaxFlowCalculationOnTargetTransaction : public BaseTransaction {

public:
    typedef shared_ptr<ReceiveMaxFlowCalculationOnTargetTransaction> Shared;

public:
    ReceiveMaxFlowCalculationOnTargetTransaction(
            const NodeUUID &nodeUUID,
            ReceiveMaxFlowCalculationOnTargetMessage::Shared message,
            TrustLinesManager *manager,
            Logger *logger);

    ReceiveMaxFlowCalculationOnTargetMessage::Shared message() const;

    TransactionResult::SharedConst run();

private:

    void sendMessagesOnFirstLevel();

    void sendResultToInitiator();

private:
    ReceiveMaxFlowCalculationOnTargetMessage::Shared mMessage;
    TrustLinesManager *mTrustLinesManager;
    Logger *mLog;
};


#endif //GEO_NETWORK_CLIENT_RECEIVEMAXFLOWCALCULATIONTRANSACTION_H
