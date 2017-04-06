﻿#ifndef GEO_NETWORK_CLIENT_TRANSACTIONMESSAGE_H
#define GEO_NETWORK_CLIENT_TRANSACTIONMESSAGE_H

#include "../../SenderMessage.h"

#include "../../../../common/Types.h"
#include "../../../../common/NodeUUID.h"
#include "../../../../common/memory/MemoryUtils.h"

#include "../../../../transactions/transactions/base/TransactionUUID.h"

#include <memory>
#include <utility>
#include <stdint.h>

using namespace std;

class TransactionMessage: public SenderMessage {

public:
    typedef shared_ptr<TransactionMessage> Shared;
    typedef shared_ptr<const TransactionMessage> ConstShared;

public:
    virtual const TransactionUUID &transactionUUID() const;

protected:
    TransactionMessage();

    TransactionMessage(
        const NodeUUID &senderUUID,
        const TransactionUUID &transactionUUID);

    TransactionMessage(
        BytesShared bufer);

    virtual const MessageType typeID() const = 0;

    virtual pair<BytesShared, size_t> serializeToBytes();

    virtual void deserializeFromBytes(
        BytesShared buffer);

    const size_t kOffsetToInheritedBytes();

private:
    const bool isTransactionMessage() const;

protected:
    TransactionUUID mTransactionUUID;
};
#endif //GEO_NETWORK_CLIENT_TRANSACTIONMESSAGE_H
