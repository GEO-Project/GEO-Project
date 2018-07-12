#include "CloseIncomingTrustLineTransaction.h"

CloseIncomingTrustLineTransaction::CloseIncomingTrustLineTransaction(
    const NodeUUID &nodeUUID,
    CloseIncomingTrustLineCommand::Shared command,
    TrustLinesManager *manager,
    StorageHandler *storageHandler,
    TopologyTrustLinesManager *topologyTrustLinesManager,
    TopologyCacheManager *topologyCacheManager,
    MaxFlowCacheManager *maxFlowCacheManager,
    SubsystemsController *subsystemsController,
    Keystore *keystore,
    Logger &logger)
    noexcept :

    BaseTrustLineTransaction(
        BaseTransaction::CloseIncomingTrustLineTransactionType,
        nodeUUID,
        command->equivalent(),
        manager,
        storageHandler,
        keystore,
        logger),
    mCommand(command),
    mTopologyTrustLinesManager(topologyTrustLinesManager),
    mTopologyCacheManager(topologyCacheManager),
    mMaxFlowCacheManager(maxFlowCacheManager),
    mSubsystemsController(subsystemsController)
{
    mContractorUUID = mCommand->contractorUUID();
    mPreviousState = TrustLine::Active;
    mAuditNumber = mTrustLines->auditNumber(mContractorUUID) + 1;
}

CloseIncomingTrustLineTransaction::CloseIncomingTrustLineTransaction(
    const NodeUUID &nodeUUID,
    SerializedEquivalent equivalent,
    const NodeUUID &contractorUUID,
    TrustLinesManager *manager,
    StorageHandler *storageHandler,
    TopologyTrustLinesManager *topologyTrustLinesManager,
    TopologyCacheManager *topologyCacheManager,
    MaxFlowCacheManager *maxFlowCacheManager,
    Keystore *keystore, Logger &logger) :
    BaseTrustLineTransaction(
        BaseTransaction::CloseIncomingTrustLineTransactionType,
        nodeUUID,
        equivalent,
        manager,
        storageHandler,
        keystore,
        logger),
    mTopologyTrustLinesManager(topologyTrustLinesManager),
    mTopologyCacheManager(topologyCacheManager),
    mMaxFlowCacheManager(maxFlowCacheManager)
{
    mContractorUUID = contractorUUID;
    mStep = Stages::AddToBlackList;
}

CloseIncomingTrustLineTransaction::CloseIncomingTrustLineTransaction(
    BytesShared buffer,
    const NodeUUID &nodeUUID,
    TrustLinesManager *manager,
    StorageHandler *storageHandler,
    Keystore *keystore,
    Logger &logger) :

    BaseTrustLineTransaction(
        buffer,
        nodeUUID,
        manager,
        storageHandler,
        keystore,
        logger)
{
    auto bytesBufferOffset = BaseTransaction::kOffsetToInheritedBytes();
    mStep = Stages::Recovery;

    memcpy(
        mContractorUUID.data,
        buffer.get() + bytesBufferOffset,
        NodeUUID::kBytesSize);
    bytesBufferOffset += NodeUUID::kBytesSize;

    auto *trustLineState = new (buffer.get()) TrustLine::SerializedTrustLineState;
    mPreviousState = (TrustLine::TrustLineState) *trustLineState;
    bytesBufferOffset += sizeof(TrustLine::SerializedTrustLineState);

    vector<byte> previousAmountBytes(
        buffer.get() + bytesBufferOffset,
        buffer.get() + bytesBufferOffset + kTrustLineAmountBytesCount);
    mPreviousIncomingAmount = bytesToTrustLineAmount(previousAmountBytes);

    if (mTrustLines->trustLineState(mContractorUUID) == TrustLine::AuditPending) {
        bytesBufferOffset += kTrustLineAmountBytesCount;

        memcpy(
            &mAuditNumber,
            buffer.get() + bytesBufferOffset,
            sizeof(AuditNumber));
        bytesBufferOffset += sizeof(AuditNumber);

        auto signature = make_shared<lamport::Signature>(
            buffer.get() + bytesBufferOffset);
        bytesBufferOffset += lamport::Signature::signatureSize();

        KeyNumber keyNumber;
        memcpy(
            &keyNumber,
            buffer.get() + bytesBufferOffset,
            sizeof(KeyNumber));

        mOwnSignatureAndKeyNumber = make_pair(
            signature,
            keyNumber);
    }
}

TransactionResult::SharedConst CloseIncomingTrustLineTransaction::run()
{
    switch (mStep) {
        case Stages::TrustLineInitialisation: {
            return runInitialisationStage();
        }
        case Stages::TrustLineResponseProcessing: {
            return runResponseProcessingStage();
        }
        case Stages::AuditInitialisation: {
            return runAuditInitializationStage();
        }
        case Stages::AuditResponseProcessing: {
            return runAuditResponseProcessingStage();
        }
        case Stages::Recovery: {
            return runRecoveryStage();
        }
        case Stages::AddToBlackList: {
            return runAddToBlackListStage();
        }
        default:
            throw ValueError(logHeader() + "::run: "
                "wrong value of mStep " + to_string(mStep));
    }
}

TransactionResult::SharedConst CloseIncomingTrustLineTransaction::runInitialisationStage()
{
    if (!mSubsystemsController->isRunTrustLineTransactions()) {
        debug() << "It is forbidden run trust line transactions";
        return resultForbiddenRun();
    }

    if (mContractorUUID == mNodeUUID) {
        warning() << "Attempt to launch transaction against itself was prevented.";
        return resultProtocolError();
    }

    try {
        if (mTrustLines->trustLineState(mContractorUUID) != TrustLine::Active) {
            warning() << "Invalid TL state " << mTrustLines->trustLineState(mContractorUUID);
            return resultProtocolError();
        }
    } catch (NotFoundError &e) {
        warning() << "Attempt to change not existing TL";
        return resultProtocolError();
    }

    mPreviousIncomingAmount = mTrustLines->incomingTrustAmount(mContractorUUID);
    mPreviousState = mTrustLines->trustLineState(mContractorUUID);

    // Trust line must be created (or updated) in the internal storage.
    // Also, history record must be written about this operation.
    // Both writes must be done atomically, so the IO transaction is used.
    auto ioTransaction = mStorageHandler->beginTransaction();
    try {
        // note: io transaction would commit automatically on destructor call.
        // there is no need to call commit manually.
        mTrustLines->closeIncoming(
            mContractorUUID);

        mTrustLines->setTrustLineState(
            mContractorUUID,
            TrustLine::Modify,
            ioTransaction);

        // remove this TL from Topology TrustLines Manager
        mTopologyTrustLinesManager->addTrustLine(
            make_shared<TopologyTrustLine>(
                mNodeUUID,
                mContractorUUID,
                make_shared<const TrustLineAmount>(0)));
        mTopologyCacheManager->resetInitiatorCache();
        mMaxFlowCacheManager->clearCashes();
        info() << "Incoming trust line from the node " << mContractorUUID
               << " successfully closed.";

        // Notifying remote node about trust line state changed.
        // Network communicator knows, that this message must be forced to be delivered,
        // so the TA itself might finish without any response from the remote node.
        sendMessage<CloseOutgoingTrustLineMessage>(
            mContractorUUID,
            mEquivalent,
            mNodeUUID,
            mTransactionUUID,
            mContractorUUID);

        auto bytesAndCount = serializeToBytes();
        info() << "Transaction serialized";
        ioTransaction->transactionHandler()->saveRecord(
            currentTransactionUUID(),
            bytesAndCount.first,
            bytesAndCount.second);
        info() << "Transaction saved";

        mStep = TrustLineResponseProcessing;
        return resultOK();

    } catch (IOError &e) {
        ioTransaction->rollback();
        // return closed TL
        mTrustLines->setIncoming(
            mContractorUUID,
            mPreviousIncomingAmount);
        mTrustLines->setTrustLineState(
            mContractorUUID,
            mPreviousState);
        warning() << "Attempt to close incoming TL from the node " << mContractorUUID << " failed. "
                  << "IO transaction can't be completed. "
                  << "Details are: " << e.what();

        // Rethrowing the exception,
        // because the TA can't finish properly and no result may be returned.
        throw e;
    }
}

TransactionResult::SharedConst CloseIncomingTrustLineTransaction::runResponseProcessingStage()
{
    if (mContext.empty()) {
        warning() << "Contractor don't send response. Transaction will be closed, and wait for message";
        return resultDone();
    }
    auto message = popNextMessage<TrustLineConfirmationMessage>();
    info() << "contractor " << message->senderUUID << " confirmed closing incoming TL.";
    if (message->senderUUID != mContractorUUID) {
        warning() << "Sender is not contractor of this transaction";
        return resultContinuePreviousState();
    }
    if (!mTrustLines->trustLineIsPresent(mContractorUUID)) {
        warning() << "Something wrong, because TL must be created";
        // todo : need correct reaction
        return resultDone();
    }
    auto ioTransaction = mStorageHandler->beginTransaction();
    try {
        if (message->state() != ConfirmationMessage::OK) {
            warning() << "Contractor didn't accept closing incoming TL. Response code: " << message->state();
            mTrustLines->setIncoming(
                mContractorUUID,
                mPreviousIncomingAmount);
            mTrustLines->setTrustLineState(
                mContractorUUID,
                mPreviousState,
                ioTransaction);
            // delete this transaction from storage
            ioTransaction->transactionHandler()->deleteRecord(
                currentTransactionUUID());
            processConfirmationMessage(message);
            return resultDone();
        }
        mTrustLines->setTrustLineState(
            mContractorUUID,
            TrustLine::AuditPending,
            ioTransaction);

        populateHistory(ioTransaction, TrustLineRecord::ClosingIncoming);

        // delete this transaction from storage
        ioTransaction->transactionHandler()->deleteRecord(
            currentTransactionUUID());
    } catch (IOError &e) {
        ioTransaction->rollback();
        // todo : need return intermediate state of TL
        error() << "Attempt to process confirmation from contractor " << message->senderUUID << " failed. "
                << "IO transaction can't be completed. Details are: " << e.what();
        return resultDone();
    }

    processConfirmationMessage(message);
    mStep = AuditInitialisation;
    return resultAwakeAsFastAsPossible();
}

TransactionResult::SharedConst CloseIncomingTrustLineTransaction::runRecoveryStage()
{
    info() << "Recovery";
    if (!mTrustLines->trustLineIsPresent(mContractorUUID)) {
        warning() << "Trust line is absent.";
        return resultDone();
    }
    if (mTrustLines->trustLineState(mContractorUUID) == TrustLine::Modify) {
        mTrustLines->closeIncoming(
            mContractorUUID);
        mStep = TrustLineResponseProcessing;
        return runResponseProcessingStage();
    } else if (mTrustLines->trustLineState(mContractorUUID) == TrustLine::AuditPending) {
        mTrustLines->closeIncoming(
            mContractorUUID);
        mStep = AuditResponseProcessing;
        return runAuditResponseProcessingStage();
    } else {
        warning() << "Invalid TL state for this TA state: "
                  << mTrustLines->trustLineState(mContractorUUID);
        return resultDone();
    }
}

TransactionResult::SharedConst CloseIncomingTrustLineTransaction::runAddToBlackListStage()
{
    info() << "Close incoming TL to " << mContractorUUID << " after adding to black list";
    try {
        mPreviousIncomingAmount = mTrustLines->incomingTrustAmount(mContractorUUID);
        mPreviousState = mTrustLines->trustLineState(mContractorUUID);
    } catch (NotFoundError &e) {
        warning() << "Attempt to change not existing TL";
        return resultDone();
    }

    // Trust line must be created (or updated) in the internal storage.
    // Also, history record must be written about this operation.
    // Both writes must be done atomically, so the IO transaction is used.
    auto ioTransaction = mStorageHandler->beginTransaction();
    try {
        // note: io transaction would commit automatically on destructor call.
        // there is no need to call commit manually.
        mTrustLines->closeIncoming(
            mContractorUUID);

        mTrustLines->setTrustLineState(
            mContractorUUID,
            TrustLine::Modify,
            ioTransaction);

        // remove this TL from Topology TrustLines Manager
        mTopologyTrustLinesManager->addTrustLine(
            make_shared<TopologyTrustLine>(
                mNodeUUID,
                mContractorUUID,
                make_shared<const TrustLineAmount>(0)));
        mTopologyCacheManager->resetInitiatorCache();
        mMaxFlowCacheManager->clearCashes();
        info() << "Incoming trust line from the node " << mContractorUUID
               << " successfully closed.";

        // Notifying remote node about trust line state changed.
        // Network communicator knows, that this message must be forced to be delivered,
        // so the TA itself might finish without any response from the remote node.
        sendMessage<CloseOutgoingTrustLineMessage>(
            mContractorUUID,
            mEquivalent,
            mNodeUUID,
            mTransactionUUID,
            mContractorUUID);

        auto bytesAndCount = serializeToBytes();
        info() << "Transaction serialized";
        ioTransaction->transactionHandler()->saveRecord(
            currentTransactionUUID(),
            bytesAndCount.first,
            bytesAndCount.second);
        info() << "Transaction saved";

        mStep = TrustLineResponseProcessing;
        return resultWaitForMessageTypes(
            {Message::TrustLines_Confirmation},
            kWaitMillisecondsForResponse);

    } catch (IOError &e) {
        ioTransaction->rollback();
        // return closed TL
        mTrustLines->setIncoming(
            mContractorUUID,
            mPreviousIncomingAmount);
        mTrustLines->setTrustLineState(
            mContractorUUID,
            mPreviousState);
        warning() << "Attempt to close incoming TL from the node " << mContractorUUID << " failed. "
                  << "IO transaction can't be completed. "
                  << "Details are: " << e.what();

        // Rethrowing the exception,
        // because the TA can't finish properly and no result may be returned.
        throw e;
    }
}

TransactionResult::SharedConst CloseIncomingTrustLineTransaction::resultOK()
{
    return transactionResultFromCommandAndWaitForMessageTypes(
        mCommand->responseOK(),
        {Message::TrustLines_Confirmation},
        kWaitMillisecondsForResponse);
}

TransactionResult::SharedConst CloseIncomingTrustLineTransaction::resultForbiddenRun()
{
    return transactionResultFromCommand(
        mCommand->responseForbiddenRunTransaction());
}

TransactionResult::SharedConst CloseIncomingTrustLineTransaction::resultProtocolError()
{
    return transactionResultFromCommand(
        mCommand->responseProtocolError());
}

pair<BytesShared, size_t> CloseIncomingTrustLineTransaction::serializeToBytes() const
{
    const auto parentBytesAndCount = BaseTransaction::serializeToBytes();
    size_t bytesCount = parentBytesAndCount.second
                        + NodeUUID::kBytesSize
                        + sizeof(TrustLine::SerializedTrustLineState)
                        + kTrustLineAmountBytesCount;
    if (mTrustLines->trustLineState(mContractorUUID) == TrustLine::AuditPending) {
        bytesCount += sizeof(AuditNumber)
                      + lamport::Signature::signatureSize()
                      + sizeof(KeyNumber);
    }

    BytesShared dataBytesShared = tryCalloc(bytesCount);
    size_t dataBytesOffset = 0;

    memcpy(
        dataBytesShared.get(),
        parentBytesAndCount.first.get(),
        parentBytesAndCount.second);
    dataBytesOffset += parentBytesAndCount.second;

    memcpy(
        dataBytesShared.get() + dataBytesOffset,
        mContractorUUID.data,
        NodeUUID::kBytesSize);
    dataBytesOffset += NodeUUID::kBytesSize;

    memcpy(
        dataBytesShared.get() + dataBytesOffset,
        &mPreviousState,
        sizeof(TrustLine::SerializedTrustLineState));
    dataBytesOffset += sizeof(TrustLine::SerializedTrustLineState);

    vector<byte> buffer = trustLineAmountToBytes(mPreviousIncomingAmount);
    memcpy(
        dataBytesShared.get() + dataBytesOffset,
        buffer.data(),
        buffer.size());

    if (mTrustLines->trustLineState(mContractorUUID) == TrustLine::AuditPending) {
        dataBytesOffset += kTrustLineAmountBytesCount;

        memcpy(
            dataBytesShared.get() + dataBytesOffset,
            &mAuditNumber,
            sizeof(AuditNumber));
        dataBytesOffset += sizeof(AuditNumber);

        memcpy(
            dataBytesShared.get() + dataBytesOffset,
            mOwnSignatureAndKeyNumber.first->data(),
            lamport::Signature::signatureSize());
        dataBytesOffset += lamport::Signature::signatureSize();

        memcpy(
            dataBytesShared.get() + dataBytesOffset,
            &mOwnSignatureAndKeyNumber.second,
            sizeof(KeyNumber));
    }

    return make_pair(
        dataBytesShared,
        bytesCount);
}

const string CloseIncomingTrustLineTransaction::logHeader() const
noexcept
{
    stringstream s;
    s << "[CloseIncomingTrustLineTA: " << currentTransactionUUID() << " " << mEquivalent << "]";
    return s.str();
}

void CloseIncomingTrustLineTransaction::populateHistory(
    IOTransaction::Shared ioTransaction,
    TrustLineRecord::TrustLineOperationType operationType)
{
#ifndef TESTS
    auto record = make_shared<TrustLineRecord>(
        mTransactionUUID,
        operationType,
        mContractorUUID);

    ioTransaction->historyStorage()->saveTrustLineRecord(
        record,
        mEquivalent);
#endif
}
