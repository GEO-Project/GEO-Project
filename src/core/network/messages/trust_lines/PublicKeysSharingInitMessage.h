#ifndef GEO_NETWORK_CLIENT_PUBLICKEYSSHARINGINITMESSAGE_H
#define GEO_NETWORK_CLIENT_PUBLICKEYSSHARINGINITMESSAGE_H

#include "PublicKeyMessage.h"

class PublicKeysSharingInitMessage : public PublicKeyMessage {

public:
    typedef shared_ptr<PublicKeysSharingInitMessage> Shared;

public:
    PublicKeysSharingInitMessage(
        const SerializedEquivalent equivalent,
        const TransactionUUID &transactionUUID,
        const KeysCount keysCount,
        const KeyNumber number,
        const lamport::PublicKey::Shared publicKey);

    PublicKeysSharingInitMessage(
        const SerializedEquivalent equivalent,
        ContractorID idOnSenderSide,
        const TransactionUUID &transactionUUID,
        const KeysCount keysCount,
        const KeyNumber number,
        const lamport::PublicKey::Shared publicKey);

    PublicKeysSharingInitMessage(
        BytesShared buffer);

    const KeysCount keysCount() const;

    const MessageType typeID() const;

    virtual pair<BytesShared, size_t> serializeToBytes() const;

private:
    KeysCount mKeysCount;
};


#endif //GEO_NETWORK_CLIENT_PUBLICKEYSSHARINGINITMESSAGE_H
