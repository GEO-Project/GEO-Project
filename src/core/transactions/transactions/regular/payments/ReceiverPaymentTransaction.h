﻿#ifndef GEO_NETWORK_CLIENT_RECEIVERPAYMENTTRANSACTION_H
#define GEO_NETWORK_CLIENT_RECEIVERPAYMENTTRANSACTION_H


#include "base/BasePaymentTransaction.h"
#include "../../cycles/ThreeNodes/CyclesThreeNodesInitTransaction.h"


class ReceiverPaymentTransaction:
    public BasePaymentTransaction {

public:
    typedef shared_ptr<ReceiverPaymentTransaction> Shared;
    typedef shared_ptr<const ReceiverPaymentTransaction> ConstShared;

public:
    ReceiverPaymentTransaction(
        const NodeUUID &currentNodeUUID,
        ReceiverInitPaymentRequestMessage::ConstShared message,
        TrustLinesManager *trustLines,
        StorageHandler *storageHandler,
        MaxFlowCalculationCacheManager *maxFlowCalculationCacheManager,
        Logger *log);

    ReceiverPaymentTransaction(
        BytesShared buffer,
        const NodeUUID &nodeUUID,
        TrustLinesManager *trustLines,
        StorageHandler *storageHandler,
        MaxFlowCalculationCacheManager *maxFlowCalculationCacheManager,
        Logger *log);

    TransactionResult::SharedConst run()
        noexcept;

    pair<BytesShared, size_t> serializeToBytes();

protected:
    TransactionResult::SharedConst runInitialisationStage();
    TransactionResult::SharedConst runAmountReservationStage();
    TransactionResult::SharedConst runClarificationOfTransaction();
    TransactionResult::SharedConst runVotesCheckingStageWithCoordinatorClarification();

protected:
    // Receiver must must save payment operation into history.
    // Therefore this methods are overriden.
    TransactionResult::SharedConst approve();

protected:
    void savePaymentOperationIntoHistory();

    void launchThreeCyclesClosingTransactions();

    void deserializeFromBytes(
        BytesShared buffer);

    const string logHeader() const;

protected:
    const ReceiverInitPaymentRequestMessage::ConstShared mMessage;
    TrustLineAmount mTotalReserved;
};


#endif //GEO_NETWORK_CLIENT_RECEIVERPAYMENTTRANSACTION_H
