#ifndef GEO_NETWORK_CLIENT_BASETRUSTLINETRANSACTION_H
#define GEO_NETWORK_CLIENT_BASETRUSTLINETRANSACTION_H

#include "../../base/BaseTransaction.h"
#include "../../../../trust_lines/manager/TrustLinesManager.h"
#include "../../../../crypto/keychain.h"
#include "../../../../crypto/lamportkeys.h"

#include "../../../../subsystems_controller/TrustLinesInfluenceController.h"

#include "../../../../network/messages/trust_lines/TrustLineConfirmationMessage.h"
#include "../../../../network/messages/trust_lines/AuditMessage.h"
#include "../../../../network/messages/trust_lines/AuditResponseMessage.h"
#include "../../../../network/messages/trust_lines/PublicKeysSharingInitMessage.h"
#include "../../../../network/messages/trust_lines/PublicKeyMessage.h"
#include "../../../../network/messages/trust_lines/PublicKeyHashConfirmation.h"

#include "../ConflictResolverInitiatorTransaction.h"

namespace signals = boost::signals2;

class BaseTrustLineTransaction : public BaseTransaction {

public:
    typedef shared_ptr<BaseTrustLineTransaction> Shared;

public:
    typedef signals::signal<void(const NodeUUID&, const SerializedEquivalent)> PublicKeysSharingSignal;

    BaseTrustLineTransaction(
        const TransactionType type,
        const NodeUUID &currentNodeUUID,
        const SerializedEquivalent equivalent,
        const NodeUUID &contractorUUID,
        TrustLinesManager *trustLines,
        StorageHandler *storageHandler,
        Keystore *keystore,
        TrustLinesInfluenceController *trustLinesInfluenceController,
        Logger &log);

    BaseTrustLineTransaction(
        const TransactionType type,
        const TransactionUUID &transactionUUID,
        const NodeUUID &currentNodeUUID,
        const SerializedEquivalent equivalent,
        const NodeUUID &contractorUUID,
        TrustLinesManager *trustLines,
        StorageHandler *storageHandler,
        Keystore *keystore,
        TrustLinesInfluenceController *trustLinesInfluenceController,
        Logger &log);

    BaseTrustLineTransaction(
        BytesShared buffer,
        const NodeUUID &nodeUUID,
        TrustLinesManager *trustLines,
        StorageHandler *storageHandler,
        Keystore *keystore,
        TrustLinesInfluenceController *trustLinesInfluenceController,
        Logger &log);

protected:
    enum Stages {
        TrustLineInitialization = 1,
        TrustLineResponseProcessing = 2,
        AuditInitialization = 3,
        AuditResponseProcessing = 4,
        AuditTarget = 5,
        KeysSharingInitialization = 6,
        NextKeyProcessing = 7,
        KeysSharingTargetInitialization = 8,
        KeysSharingTargetNextKey = 9,
        Recovery = 10,
        AddToBlackList = 11,
    };

protected:
    TransactionResult::SharedConst runPublicKeysSharingInitializationStage();

    TransactionResult::SharedConst runPublicKeysSendNextKeyStage();

    TransactionResult::SharedConst runPublicKeyReceiverInitStage();

    TransactionResult::SharedConst runPublicKeyReceiverStage();

    TransactionResult::SharedConst runAuditInitializationStage();

    TransactionResult::SharedConst runAuditResponseProcessingStage();

    TransactionResult::SharedConst runAuditTargetStage();

    void updateTrustLineStateAfterInitialAudit(
        IOTransaction::Shared ioTransaction);

    void updateTrustLineStateAfterNextAudit(
        IOTransaction::Shared ioTransaction,
        TrustLineKeychain *keyChain);

    TransactionResult::SharedConst sendTrustLineErrorConfirmation(
        ConfirmationMessage::OperationState errorState);

    TransactionResult::SharedConst sendKeyErrorConfirmation(
        ConfirmationMessage::OperationState errorState);

    TransactionResult::SharedConst sendAuditErrorConfirmation(
        ConfirmationMessage::OperationState errorState);

protected:
    pair<BytesShared, size_t> getOwnSerializedAuditData();

    pair<BytesShared, size_t> getContractorSerializedAuditData();

public:
    // signal for launching transaction of public keys sharing
    mutable PublicKeysSharingSignal mPublicKeysSharingSignal;

protected:
    static const uint32_t kWaitMillisecondsForResponse = 60000;

protected:
    TrustLinesManager *mTrustLines;
    StorageHandler *mStorageHandler;
    Keystore *mKeysStore;

    NodeUUID mContractorUUID;
    AuditNumber mAuditNumber;
    pair<lamport::Signature::Shared, KeyNumber> mOwnSignatureAndKeyNumber;

    AuditMessage::Shared mAuditMessage;

    KeysCount mContractorKeysCount;
    KeyNumber mCurrentKeyNumber;
    lamport::PublicKey::Shared mCurrentPublicKey;

    TrustLinesInfluenceController *mTrustLinesInfluenceController;

    // this field is used for logic with receiving Public keys (first and next)
    // when logic with ioTransaction as parameter in run.. methods (for processing Exceptions) will be applied,
    // this field can be removed
    bool mShouldPopPublicKeyMessage;
};


#endif //GEO_NETWORK_CLIENT_BASETRUSTLINETRANSACTION_H
