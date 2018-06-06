#ifndef GEO_NETWORK_CLIENT_OPENTRUSTLINETRANSACTION_H
#define GEO_NETWORK_CLIENT_OPENTRUSTLINETRANSACTION_H

#include "../base/BaseTransaction.h"
#include "../../../interface/commands_interface/commands/trust_lines/SetOutgoingTrustLineCommand.h"
#include "../../../trust_lines/manager/TrustLinesManager.h"
#include "../../../subsystems_controller/SubsystemsController.h"
#include "../../../network/messages/trust_lines/SetIncomingTrustLineMessage.h"
#include "../../../network/messages/trust_lines/SetIncomingTrustLineFromGatewayMessage.h"
#include "../../../network/messages/trust_lines/TrustLineConfirmationMessage.h"
#include "PublicKeysSharingSourceTransaction.h"

class OpenTrustLineTransaction : public BaseTransaction {

public:
    typedef shared_ptr<OpenTrustLineTransaction> Shared;

public:
    OpenTrustLineTransaction(
        const NodeUUID &nodeUUID,
        SetOutgoingTrustLineCommand::Shared command,
        TrustLinesManager *manager,
        StorageHandler *storageHandler,
        SubsystemsController *subsystemsController,
        bool iAmGateway,
        Logger &logger)
    noexcept;

    TransactionResult::SharedConst run();

protected:
    enum Stages {
        Initialisation = 1,
        ResponseProcessing = 2,
    };

protected:
    TransactionResult::SharedConst resultOK();

    TransactionResult::SharedConst resultForbiddenRun();

    TransactionResult::SharedConst resultProtocolError();

protected: // trust lines history shortcuts
    void populateHistory(
        IOTransaction::Shared ioTransaction,
        TrustLineRecord::TrustLineOperationType operationType);

protected: // log
    const string logHeader() const
    noexcept;

private:
    TransactionResult::SharedConst runInitialisationStage();

    TransactionResult::SharedConst runResponseProcessingStage();

private:
    static const uint32_t kWaitMillisecondsForResponse = 60000;

protected:
    SetOutgoingTrustLineCommand::Shared mCommand;
    TrustLinesManager *mTrustLines;
    StorageHandler *mStorageHandler;
    SubsystemsController *mSubsystemsController;
    bool mIAmGateway;
};


#endif //GEO_NETWORK_CLIENT_OPENTRUSTLINETRANSACTION_H
