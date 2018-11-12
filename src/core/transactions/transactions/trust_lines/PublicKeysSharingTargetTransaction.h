#ifndef GEO_NETWORK_CLIENT_PUBLICKEYSSHARINGTARGETTRANSACTION_H
#define GEO_NETWORK_CLIENT_PUBLICKEYSSHARINGTARGETTRANSACTION_H

#include "../base/BaseTransaction.h"
#include "../../../trust_lines/manager/TrustLinesManager.h"
#include "../../../crypto/keychain.h"
#include "../../../crypto/lamportkeys.h"

#include "../../../subsystems_controller/TrustLinesInfluenceController.h"

#include "../../../network/messages/trust_lines/PublicKeysSharingInitMessage.h"
#include "../../../network/messages/trust_lines/PublicKeyMessage.h"
#include "../../../network/messages/trust_lines/PublicKeyHashConfirmation.h"

class PublicKeysSharingTargetTransaction : public BaseTransaction {

public:
    typedef shared_ptr<PublicKeysSharingTargetTransaction> Shared;

public:
    PublicKeysSharingTargetTransaction(
        const NodeUUID &nodeUUID,
        PublicKeysSharingInitMessage::Shared message,
        TrustLinesManager *manager,
        StorageHandler *storageHandler,
        Keystore *keystore,
        TrustLinesInfluenceController *trustLinesInfluenceController,
        Logger &logger);

    TransactionResult::SharedConst run();

protected:
    enum Stages {
        Initialization = 1,
        NextKeyProcessing = 2,
    };

protected: // log
    const string logHeader() const;

private:
    TransactionResult::SharedConst runPublicKeyReceiverInitStage();

    TransactionResult::SharedConst runReceiveNextKeyStage();

    TransactionResult::SharedConst runProcessKey(
        IOTransaction::Shared ioTransaction);

    TransactionResult::SharedConst sendKeyErrorConfirmation(
        ConfirmationMessage::OperationState errorState);

protected:
    // these constants should be the same as in PublicKeysSharingSourceTransaction
    static const uint32_t kWaitMillisecondsForResponse = 20000;
    static const uint16_t kMaxCountSendingAttempts = 3;

private:
    TrustLinesManager *mTrustLines;
    StorageHandler *mStorageHandler;
    Keystore *mKeysStore;

    NodeUUID mContractorUUID;
    KeyNumber mCurrentKeyNumber;
    lamport::PublicKey::Shared mCurrentPublicKey;
    KeysCount mContractorKeysCount;

    TrustLinesInfluenceController *mTrustLinesInfluenceController;
};


#endif //GEO_NETWORK_CLIENT_PUBLICKEYSSHARINGTARGETTRANSACTION_H
