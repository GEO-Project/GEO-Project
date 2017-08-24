﻿#ifndef BASEPAYMENTTRANSACTION_H
#define BASEPAYMENTTRANSACTION_H


#include "../../../base/BaseTransaction.h"

#include "../../../../../common/Types.h"
#include "../../../../../paths/lib/Path.h"
#include "../../../../../logger/Logger.h"

#include "../../../../../trust_lines/manager/TrustLinesManager.h"
#include "../../../../../io/storage/StorageHandler.h"
#include "../../../../../max_flow_calculation/cashe/MaxFlowCalculationCacheManager.h"

#include "../../../../../network/messages/payments/ReceiverInitPaymentRequestMessage.h"
#include "../../../../../network/messages/payments/ReceiverInitPaymentResponseMessage.h"
#include "../../../../../network/messages/payments/CoordinatorReservationRequestMessage.h"
#include "../../../../../network/messages/payments/CoordinatorReservationResponseMessage.h"
#include "../../../../../network/messages/payments/IntermediateNodeReservationRequestMessage.h"
#include "../../../../../network/messages/payments/IntermediateNodeReservationResponseMessage.h"
#include "../../../../../network/messages/payments/CoordinatorCycleReservationRequestMessage.h"
#include "../../../../../network/messages/payments/CoordinatorCycleReservationResponseMessage.h"
#include "../../../../../network/messages/payments/IntermediateNodeCycleReservationRequestMessage.h"
#include "../../../../../network/messages/payments/IntermediateNodeCycleReservationResponseMessage.h"
#include "../../../../../network/messages/payments/ParticipantsVotesMessage.h"
#include "../../../../../network/messages/payments/VotesStatusRequestMessage.hpp"
#include "../../../../../network/messages/payments/FinalPathConfigurationMessage.h"
#include "../../../../../network/messages/payments/FinalPathCycleConfigurationMessage.h"
#include "../../../../../network/messages/payments/TTLProlongationRequestMessage.h"
#include "../../../../../network/messages/payments/TTLProlongationResponseMessage.h"
#include "../../../../../network/messages/payments/FinalAmountsConfigurationMessage.h"
#include "../../../../../network/messages/payments/FinalAmountsConfigurationResponseMessage.h"

#include "PathStats.h"

#include "../../../../../subsystems_controller/SubsystemsController.h"

#include <unordered_set>

namespace signals = boost::signals2;

// TODO: Add restoring of the reservations after transaction deserialization.
class BasePaymentTransaction:
    public BaseTransaction {

public:
    typedef shared_ptr<BasePaymentTransaction> Shared;

public:
    typedef signals::signal<void(vector<NodeUUID> &contractorUUID)> BuildCycleThreeNodesSignal;
    typedef signals::signal<void(vector<pair<NodeUUID, NodeUUID>> &debtorsAndCreditors)> BuildCycleFourNodesSignal;

public:
    BasePaymentTransaction(
        const TransactionType type,
        const NodeUUID &currentNodeUUID,
        TrustLinesManager *trustLines,
        StorageHandler *storageHandler,
        MaxFlowCalculationCacheManager *maxFlowCalculationCacheManager,
        Logger &log,
        SubsystemsController *subsystemsController);

    BasePaymentTransaction(
        const TransactionType type,
        const TransactionUUID &transactionUUID,
        const NodeUUID &currentNodeUUID,
        TrustLinesManager *trustLines,
        StorageHandler *storageHandler,
        MaxFlowCalculationCacheManager *maxFlowCalculationCacheManager,
        Logger &log,
        SubsystemsController *subsystemsController);

    BasePaymentTransaction(
        BytesShared buffer,
        const NodeUUID &nodeUUID,
        TrustLinesManager *trustLines,
        StorageHandler *storageHandler,
        MaxFlowCalculationCacheManager *maxFlowCalculationCacheManager,
        Logger &log,
        SubsystemsController *subsystemsController);

    virtual pair<BytesShared, size_t> serializeToBytes() const;

public:
    virtual const NodeUUID& coordinatorUUID() const;

    virtual const uint8_t cycleLength() const;

    bool isCommonVotesCheckingstage() const;

    void setRollbackByOtherTransactionStage();

protected:
    enum Stages {
        Coordinator_Initialisation = 1,
        Coordinator_ReceiverResourceProcessing,
        Coordinator_ReceiverResponseProcessing,
        Coordinator_AmountReservation,
        Coordinator_ShortPathAmountReservationResponseProcessing,
        Coordinator_PreviousNeighborRequestProcessing,
        Coordinator_FinalAmountsConfigurationConfirmation,

        Receiver_CoordinatorRequestApproving,
        Receiver_AmountReservationsProcessing,

        IntermediateNode_PreviousNeighborRequestProcessing,
        IntermediateNode_CoordinatorRequestProcessing,
        IntermediateNode_NextNeighborResponseProcessing,
        IntermediateNode_ReservationProlongation,

        Cycles_WaitForOutgoingAmountReleasing,
        Cycles_WaitForIncomingAmountReleasing,

        Common_VotesChecking,
        Common_FinalPathConfigurationChecking,
        Common_Recovery,
        Common_ClarificationTransactionBeforeVoting,
        Common_ClarificationTransactionDuringVoting,

        Common_RollbackByOtherTransaction
    };

    enum VotesRecoveryStages {
        Common_PrepareNodesListToCheckVotes,
        Common_CheckCoordinatorVotesStage,
        Common_CheckIntermediateNodeVotesStage
    };

protected:
    // TODO: move it into separate *.h file.
    typedef uint64_t PathUUID;

    // Stages handlers
    virtual TransactionResult::SharedConst runVotesCheckingStage();
    virtual TransactionResult::SharedConst runVotesConsistencyCheckingStage();

    virtual TransactionResult::SharedConst approve();
    virtual TransactionResult::SharedConst recover(
        const char *message = nullptr);
    virtual TransactionResult::SharedConst reject(
        const char *message = nullptr);
    virtual TransactionResult::SharedConst cancel(
        const char *message = nullptr);

    TransactionResult::SharedConst exitWithResult(
        const TransactionResult::SharedConst result,
        const char *message=nullptr);

    TransactionResult::SharedConst runVotesRecoveryParentStage();
    TransactionResult::SharedConst sendVotesRequestMessageAndWaitForResponse(
        const NodeUUID &contractorUUID);
    TransactionResult::SharedConst runPrepareListNodesToCheckNodes();
    TransactionResult::SharedConst runCheckCoordinatorVotesStage();
    TransactionResult::SharedConst runCheckIntermediateNodeVotesStage();
    TransactionResult::SharedConst runRollbackByOtherTransactionStage();

protected:
    const bool reserveOutgoingAmount(
        const NodeUUID &neighborNode,
        const TrustLineAmount& amount,
        const PathUUID &pathUUID);

    const bool reserveIncomingAmount(
        const NodeUUID &neighborNode,
        const TrustLineAmount& amount,
        const PathUUID &pathUUID);

    const bool shortageReservation(
        const NodeUUID kContractor,
        const AmountReservation::ConstShared kReservation,
        const TrustLineAmount &kNewAmount,
        const PathUUID &pathUUID);

    void saveVotes(
        IOTransaction::Shared ioTransaction);

    void commit(
        IOTransaction::Shared ioTransaction);

    void rollBack();

    void rollBack(
        const PathUUID &pathUUID);

    uint32_t maxNetworkDelay (
        const uint16_t totalHopsCount) const;

    uint32_t maxCoordinatorResponseTimeout() const;

    const bool contextIsValid(
        Message::MessageType messageType,
        bool showErrorMessage = true) const;

    const bool positiveVoteIsPresent (
        const ParticipantsVotesMessage::ConstShared kMessage) const;

    void propagateVotesMessageToAllParticipants (
        const ParticipantsVotesMessage::Shared kMessage) const;

    void dropNodeReservationsOnPath(
        PathUUID pathUUID);

    void sendFinalPathConfiguration(
        PathStats* pathStats,
        PathUUID pathUUID,
        const TrustLineAmount &finalPathAmount);

    // Updates all reservations according to finalAmounts
    // if some reservations will be found, pathUUIDs of which are absent in finalAmounts, returns false,
    // otherwise returns true
    bool updateReservations(
        const vector<pair<PathUUID, ConstSharedTrustLineAmount>> &finalAmounts);

    // Returns reservation pathID, which was updated, if reservation was dropped, returns 0
    PathUUID updateReservation(
        const NodeUUID &contractorUUID,
        pair<PathUUID, AmountReservation::ConstShared> &reservation,
        const vector<pair<PathUUID, ConstSharedTrustLineAmount>> &finalAmounts);

    size_t reservationsSizeInBytes() const;

    TransactionResult::SharedConst processNextNodeToCheckVotes();

    const TrustLineAmount totalReservedAmount(
        AmountReservation::ReservationDirection reservationDirection) const;

    virtual void savePaymentOperationIntoHistory(
        IOTransaction::Shared ioTransaction) = 0;

    virtual bool checkReservationsDirections() const = 0;

protected:
    // Specifies how long node must wait for the response from the remote node.
    // This timeout must take into account also that remote node may process other transaction,
    // and may be too busy to response.
    // (it is not only network transfer timeout).
    static const uint16_t kMaxMessageTransferLagMSec = 1500; // milliseconds

    // Specifies how long node must wait for the resources from other transaction
    static const uint16_t kMaxResourceTransferLagMSec = 2000; //

    static const auto kMaxPathLength = 7;

    static const uint32_t kWaitMillisecondsToTryRecoverAgain = 30000;

public:
    mutable BuildCycleThreeNodesSignal mBuildCycleThreeNodesSignal;

    mutable BuildCycleFourNodesSignal mBuildCycleFourNodesSignal;

protected:
    TrustLinesManager *mTrustLines;
    StorageHandler *mStorageHandler;
    MaxFlowCalculationCacheManager *mMaxFlowCalculationCacheManager;

    // If true - votes check stage has been processed and transaction has been approved.
    // In this case transaction can't be simply rolled back.
    // It may only be canceled through recover stage.
    //
    // If false - transaction wasn't approved yet.
    bool mTransactionIsVoted;

    // Votes message cant be signed right after it is received.
    // Additional message must be received from the coordinator,
    // so the votes message must be saved for further processing.
    ParticipantsVotesMessage::Shared mParticipantsVotesMessage;

    map<NodeUUID, vector<pair<PathUUID, AmountReservation::ConstShared>>> mReservations;

    // Votes recovery
    vector<NodeUUID> mNodesToCheckVotes;
    NodeUUID mCurrentNodeToCheckVotes;

    // this amount used for saving in payment history
    TrustLineAmount mCommitedAmount;

protected:
    SubsystemsController *mSubsystemsController;
};

#endif // BASEPAYMENTTRANSACTION_H
