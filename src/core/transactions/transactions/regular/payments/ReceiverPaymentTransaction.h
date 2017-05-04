﻿#ifndef GEO_NETWORK_CLIENT_RECEIVERPAYMENTTRANSACTION_H
#define GEO_NETWORK_CLIENT_RECEIVERPAYMENTTRANSACTION_H


#include "base/BasePaymentTransaction.h"


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
        Logger *log);

    ReceiverPaymentTransaction(
        BytesShared buffer,
        const NodeUUID &nodeUUID,
        TrustLinesManager *trustLines,
        StorageHandler *storageHandler,
        Logger *log);

    TransactionResult::SharedConst run();

    pair<BytesShared, size_t> serializeToBytes();

protected:
    TransactionResult::SharedConst runInitialisationStage();
    TransactionResult::SharedConst runAmountReservationStage();

protected:
    void deserializeFromBytes(
        BytesShared buffer);

    const string logHeader() const;

protected:
    const ReceiverInitPaymentRequestMessage::ConstShared mMessage;
    TrustLineAmount mTotalReserved;
};


#endif //GEO_NETWORK_CLIENT_RECEIVERPAYMENTTRANSACTION_H
