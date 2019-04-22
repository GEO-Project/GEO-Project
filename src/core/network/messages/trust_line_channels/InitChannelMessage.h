#ifndef GEO_NETWORK_CLIENT_INITCHANNELMESSAGE_H
#define GEO_NETWORK_CLIENT_INITCHANNELMESSAGE_H

#include "../base/transaction/TransactionMessage.h"
#include "../../../contractors/Contractor.h"

class InitChannelMessage : public TransactionMessage {

public:
    typedef shared_ptr<InitChannelMessage> Shared;

public:
    InitChannelMessage(
        vector<BaseAddress::Shared> senderAddresses,
        const TransactionUUID &transactionUUID,
        Contractor::Shared contractor)
        noexcept;

    InitChannelMessage(
        BytesShared buffer)
        noexcept;

    const MessageType typeID() const
    noexcept override;

    const ContractorID contractorID() const
    noexcept;

    const MsgEncryptor::PublicKeyShared publicKey() const
    noexcept;

    const bool isAddToConfirmationRequiredMessagesHandler() const override;

    pair<BytesShared, size_t> serializeToBytes() const override;

protected:
    ContractorID mContractorID;
    MsgEncryptor::PublicKeyShared mPublicKey;
};


#endif //GEO_NETWORK_CLIENT_INITCHANNELMESSAGE_H
