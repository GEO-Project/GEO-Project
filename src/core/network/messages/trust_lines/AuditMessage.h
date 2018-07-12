#ifndef GEO_NETWORK_CLIENT_AUDITMESSAGE_H
#define GEO_NETWORK_CLIENT_AUDITMESSAGE_H

#include "../base/transaction/ConfirmationMessage.h"
#include "../../../crypto/lamportscheme.h"

using namespace crypto;

class AuditMessage : public ConfirmationMessage {

public:
    typedef shared_ptr<AuditMessage> Shared;

public:
    AuditMessage(
        const SerializedEquivalent equivalent,
        const NodeUUID &senderUUID,
        const TransactionUUID &transactionUUID,
        const KeyNumber keyNumber,
        const lamport::Signature::Shared signature);

    AuditMessage(
        const SerializedEquivalent equivalent,
        const NodeUUID &senderUUID,
        const TransactionUUID &transactionUUID,
        OperationState state);

    AuditMessage(
        BytesShared buffer);

    const lamport::Signature::Shared signature() const;

    const KeyNumber keyNumber() const;

    const MessageType typeID() const;

    const bool isAddToConfirmationRequiredMessagesHandler() const;

    pair<BytesShared, size_t> serializeToBytes() const
        throw (bad_alloc);

private:
    uint32_t mKeyNumber;
    lamport::Signature::Shared mSignature;
};


#endif //GEO_NETWORK_CLIENT_AUDITMESSAGE_H
