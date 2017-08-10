#include "CycleCloserInitiatorTransaction.h"

CycleCloserInitiatorTransaction::CycleCloserInitiatorTransaction(
    const NodeUUID &kCurrentNodeUUID,
    Path::ConstShared path,
    TrustLinesManager *trustLines,
    CyclesManager *cyclesManager,
    StorageHandler *storageHandler,
    MaxFlowCalculationCacheManager *maxFlowCalculationCacheManager,
    Logger &log,
    TestingController *testingController)
noexcept :

    BasePaymentTransaction(
        BaseTransaction::Payments_CycleCloserInitiatorTransaction,
        kCurrentNodeUUID,
        trustLines,
        storageHandler,
        maxFlowCalculationCacheManager,
        log,
        testingController),
    mCyclesManager(cyclesManager)
{
    mStep = Stages::Coordinator_Initialisation;
    mPathStats = make_unique<PathStats>(path);
}

CycleCloserInitiatorTransaction::CycleCloserInitiatorTransaction(
    BytesShared buffer,
    const NodeUUID &nodeUUID,
    TrustLinesManager *trustLines,
    CyclesManager *cyclesManager,
    StorageHandler *storageHandler,
    MaxFlowCalculationCacheManager *maxFlowCalculationCacheManager,
    Logger &log,
    TestingController *testingController)
throw (bad_alloc) :

    BasePaymentTransaction(
        buffer,
        nodeUUID,
        trustLines,
        storageHandler,
        maxFlowCalculationCacheManager,
        log,
        testingController),
    mCyclesManager(cyclesManager)
{}


TransactionResult::SharedConst CycleCloserInitiatorTransaction::run()
    noexcept
{
    try {
        switch (mStep) {
            case Stages::Coordinator_Initialisation:
                return runInitialisationStage();

            case Stages::Coordinator_AmountReservation:
                return runAmountReservationStage();

            case Stages::Coordinator_PreviousNeighborRequestProcessing:
                return runPreviousNeighborRequestProcessingStage();

            case Stages::Coordinator_FinalAmountsConfigurationConfirmation:
                return runFinalAmountsConfigurationConfirmationProcessingStage();

            case Stages::Common_VotesChecking:
                return runVotesConsistencyCheckingStage();

            case Stages::Cycles_WaitForIncomingAmountReleasing:
                return runPreviousNeighborRequestProcessingStageAgain();

            case Stages::Cycles_WaitForOutgoingAmountReleasing:
                return runAmountReservationStageAgain();

            case Stages::Common_RollbackByOtherTransaction:
                return runRollbackByOtherTransactionStage();

            case Stages::Common_Recovery:
                return runVotesRecoveryParentStage();

            default:
                throw RuntimeError(
                    "CycleCloserInitiatorTransaction::run(): "
                       "invalid transaction step.");
        }
    } catch(Exception &e) {
        error() << e.what();
        return recover("Something happens wrong in method run(). Transaction will be recovered");
    }
}

TransactionResult::SharedConst CycleCloserInitiatorTransaction::runInitialisationStage()
{
    debug() << "runInitialisationStage";
    // Firstly check if paths is valid cycle
    checkPath(
        mPathStats->path());
    debug() << "cycle is valid";
    mNextNode = mPathStats->path()->nodes[1];
    debug() << "first intermediate node: " << mNextNode;
    if (! mTrustLines->isNeighbor(mNextNode)){
        // Internal process error. Wrong path
        error() << "Invalid path occurred. Node (" << mNextNode << ") is not listed in first level contractors list.";
        throw RuntimeError(
            "CycleCloserInitiatorTransaction::runAmountReservationStage: "
                "invalid first level node occurred. ");
    }
    const auto kOutgoingAmounts = mTrustLines->availableOutgoingCycleAmounts(mNextNode);
    const auto kOutgoingAmountWithReservations = kOutgoingAmounts.first;
    const auto kOutgoingAmountWithoutReservations = kOutgoingAmounts.second;
    debug() << "OutgoingAmountWithReservations: " << *kOutgoingAmountWithReservations
            << " OutgoingAmountWithoutReservations: " << *kOutgoingAmountWithoutReservations;

    if (*kOutgoingAmountWithReservations == TrustLine::kZeroAmount()) {
        if (*kOutgoingAmountWithoutReservations == TrustLine::kZeroAmount()) {
            debug() << "Can't close cycle, because coordinator outgoing amount equal zero, "
                "and can't use reservations from other transactions";
            mCyclesManager->addClosedTrustLine(
                currentNodeUUID(),
                mNextNode);
            return resultDone();
        } else {
            mOutgoingAmount = TrustLineAmount(0);
        }
    } else {
        mOutgoingAmount = *kOutgoingAmountWithReservations;
    }

    debug() << "outgoing Possibilities: " << mOutgoingAmount;

    mPreviousNode = mPathStats->path()->nodes[mPathStats->path()->length() - 2];
    debug() << "last intermediate node: " << mPreviousNode;
    if (! mTrustLines->isNeighbor(mPreviousNode)){
        // Internal process error. Wrong path
        error() << "Invalid path occurred. Node (" << mPreviousNode << ") is not listed in first level contractors list.";
        throw RuntimeError(
            "CycleCloserInitiatorTransaction::runAmountReservationStage: "
                "invalid first level node occurred. ");
    }

    const auto kIncomingAmounts = mTrustLines->availableIncomingCycleAmounts(mPreviousNode);
    mIncomingAmount = *(kIncomingAmounts.first);

    if (mIncomingAmount == TrustLine::kZeroAmount()) {
        debug() << "Can't close cycle, because coordinator incoming amount equal zero";
        mCyclesManager->addClosedTrustLine(
            mPreviousNode,
            currentNodeUUID());
        return resultDone();
    }
    debug() << "Incoming Possibilities: " << mIncomingAmount;

    if (mIncomingAmount < mOutgoingAmount) {
        mOutgoingAmount = mIncomingAmount;
    }
    debug() << "Initial Outgoig Amount: " << mOutgoingAmount;
    mStep = Stages::Coordinator_AmountReservation;
    return runAmountReservationStage();
}

TransactionResult::SharedConst CycleCloserInitiatorTransaction::runAmountReservationStage ()
{
    debug() << "runAmountReservationStage";
    const auto kPathStats = mPathStats.get();
    if (kPathStats->isReadyToSendNextReservationRequest())
        return tryReserveNextIntermediateNodeAmount();

    else if (kPathStats->isWaitingForNeighborReservationResponse())
        return processNeighborAmountReservationResponse();

    else if (kPathStats->isWaitingForNeighborReservationPropagationResponse())
        return processNeighborFurtherReservationResponse();

    else if (kPathStats->isWaitingForReservationResponse())
        return processRemoteNodeResponse();

    throw RuntimeError(
        "CycleCloserInitiatorTransaction::runAmountReservationStage: "
            "unexpected behaviour occured.");
}

/**
 * @brief CycleCloserInitiatorTransaction::propagateVotesList
 * Collects all nodes from all paths into one votes list,
 * and propagates it to the next node in the votes list.
 */
TransactionResult::SharedConst CycleCloserInitiatorTransaction::propagateVotesListAndWaitForVoutingResult()
{
    debug() << "propagateVotesListAndWaitForVoutingResult";
    const auto kCurrentNodeUUID = currentNodeUUID();
    const auto kTransactionUUID = currentTransactionUUID();

    // TODO: additional check if payment is correct

    // Prevent simple transaction rolling back
    // todo: make this atomic
    mTransactionIsVoted = true;

    mParticipantsVotesMessage = make_shared<ParticipantsVotesMessage>(
        kCurrentNodeUUID,
        kTransactionUUID,
        kCurrentNodeUUID);

#ifdef DEBUG
    uint16_t totalParticipantsCount = 0;
#endif

    // If paths wasn't processed - exclude it (all it's nodes).
    // Unprocessed paths may occur, because paths are loaded into the transaction in batch,
    // some of them may be used, and some may be left unprocessed.
    auto kPathStats = mPathStats.get();

    for (const auto &nodeUUID : kPathStats->path()->nodes) {
        // By the protocol, coordinator node must be excluded from the message.
        // Only coordinator may emit ParticipantsApprovingMessage into the network.
        // It is supposed, that in case if it was emitted - than coordinator approved the transaction.
        //
        // TODO: [mvp] [cryptography] despite this, coordinator must sign the message,
        // so the other nodes would be possible to know that this message was emitted by the coordinator.
        if (nodeUUID == kCurrentNodeUUID)
            continue;

        mParticipantsVotesMessage->addParticipant(nodeUUID);

#ifdef DEBUG
        totalParticipantsCount++;
#endif
    }

#ifdef DEBUG
    debug() << "Total participants included: " << totalParticipantsCount;
    debug() << "Participants order is the next:";
    for (const auto kNodeUUIDAndVote : mParticipantsVotesMessage->votes()) {
        debug() << kNodeUUIDAndVote.first;
    }
#endif

    // Begin message propagation
    sendMessage(
        mParticipantsVotesMessage->firstParticipant(),
        mParticipantsVotesMessage);

    debug() << "Votes message constructed and sent to the (" << mParticipantsVotesMessage->firstParticipant() << ")";

    mStep = Stages::Common_VotesChecking;
    return resultWaitForMessageTypes(
        {Message::Payments_ParticipantsVotes},
        maxNetworkDelay(5));
}

TransactionResult::SharedConst CycleCloserInitiatorTransaction::tryReserveNextIntermediateNodeAmount ()
{
    debug() << "tryReserveNextIntermediateNodeAmount";
    /*
     * Nodes scheme:
     *  R - remote node;
     *  S - next node in path after remote one;
     */

    try {
        auto pathStats = mPathStats.get();
        const auto R_UUIDAndPos = pathStats->nextIntermediateNodeAndPos();
        const auto R_UUID = R_UUIDAndPos.first;
        const auto R_PathPosition = R_UUIDAndPos.second;

        const auto S_PathPosition = R_PathPosition + 1;
        const auto S_UUID = pathStats->path()->nodes[S_PathPosition];

        if (R_PathPosition == 1) {
            if (pathStats->isNeighborAmountReserved())
                return askNeighborToApproveFurtherNodeReservation();

            else
                return askNeighborToReserveAmount();

        } else {
            debug() << "Processing " << int(R_PathPosition) << " node in path: (" << R_UUID << ").";

            return askRemoteNodeToApproveReservation(
                R_UUID,
                R_PathPosition,
                S_UUID);
        }

    } catch(NotFoundError) {
        debug() << "No unprocessed paths are left.";
        debug() << "Requested amount can't be collected. Canceling.";
        return resultDone();
    }
}

TransactionResult::SharedConst CycleCloserInitiatorTransaction::askNeighborToReserveAmount()
{
    debug() << "askNeighborToReserveAmount";
    const auto kCurrentNode = currentNodeUUID();
    const auto kTransactionUUID = currentTransactionUUID();

    // Try reserve amount locally.
    auto path = mPathStats.get();
    path->shortageMaxFlow(mOutgoingAmount);
    path->setNodeState(
        1,
        PathStats::NeighbourReservationRequestSent);

    if (0 == mOutgoingAmount) {
        // try use reservations from other transations
        auto reservations = mTrustLines->reservationsToContractor(mNextNode);
        for (auto reservation : reservations) {
            debug() << "try use " << reservation->amount() << " from "
                    << reservation->transactionUUID() << " transaction";
            if (mCyclesManager->resolveReservationConflict(
                currentTransactionUUID(), reservation->transactionUUID())) {
                debug() << "win reservation";
                mConflictedTransaction = reservation->transactionUUID();
                mStep = Cycles_WaitForOutgoingAmountReleasing;
                mOutgoingAmount = reservation->amount();
                return resultAwaikAfterMilliseconds(
                    kWaitingForReleasingAmountMSec);
            }
            debug() << "don't win reservation";
        }
    }

    if (!reserveOutgoingAmount(mNextNode, mOutgoingAmount, 0)) {
        mCyclesManager->addClosedTrustLine(
            currentNodeUUID(),
            mNextNode);
        return resultDone();
    }

    debug() << "Send request reservation (" << path->maxFlow() << ") message to " << mNextNode;
    sendMessage<IntermediateNodeCycleReservationRequestMessage>(
        mNextNode,
        kCurrentNode,
        kTransactionUUID,
        path->maxFlow(),
        currentNodeUUID(),
        path->path()->length());

    return resultWaitForMessageTypes(
        {Message::Payments_IntermediateNodeCycleReservationResponse},
        maxNetworkDelay(1));
}

TransactionResult::SharedConst CycleCloserInitiatorTransaction::runAmountReservationStageAgain()
{
    debug() << "runAmountReservationStageAgain";

    if (mCyclesManager->isTransactionStillAlive(
        mConflictedTransaction)) {
        debug() << "wait again";
        return resultAwaikAfterMilliseconds(
            kWaitingForReleasingAmountMSec);
    }

    debug() << "Reservation was released, continue";
    if (!reserveOutgoingAmount(mNextNode, mOutgoingAmount, 0)) {
        debug() << "Can't reserve. Close transaction";
        return resultDone();
    }

    auto path = mPathStats.get();
    path->shortageMaxFlow(mOutgoingAmount);
    debug() << "Send request reservation (" << path->maxFlow() << ") message to " << mNextNode;
    sendMessage<IntermediateNodeCycleReservationRequestMessage>(
        mNextNode,
        currentNodeUUID(),
        currentTransactionUUID(),
        path->maxFlow(),
        currentNodeUUID(),
        path->path()->length());

    mStep = Coordinator_AmountReservation;
    return resultWaitForMessageTypes(
        {Message::Payments_IntermediateNodeCycleReservationResponse},
        maxNetworkDelay(1));
}

TransactionResult::SharedConst CycleCloserInitiatorTransaction::askNeighborToApproveFurtherNodeReservation()
{
    debug() << "askNeighborToApproveFurtherNodeReservation";
    const auto kCoordinator = currentNodeUUID();
    const auto kTransactionUUID = currentTransactionUUID();
    const auto kNeighborPathPosition = 1;
    auto path = mPathStats.get();
    const auto kNextAfterNeighborNode = path->path()->nodes[kNeighborPathPosition+1];

    // Note:
    // no check of "neighbor" node is needed here.
    // It was done on previous step.

    sendMessage<CoordinatorCycleReservationRequestMessage>(
        mNextNode,
        kCoordinator,
        kTransactionUUID,
        path->maxFlow(),
        kNextAfterNeighborNode);

    debug() << "Further amount reservation request sent to the node (" << mNextNode << ") [" << path->maxFlow() << "]";

    path->setNodeState(
        kNeighborPathPosition,
        PathStats::ReservationRequestSent);

    return resultWaitForMessageTypes(
        {Message::Payments_CoordinatorCycleReservationResponse},
        kMaxMessageTransferLagMSec);
}

TransactionResult::SharedConst CycleCloserInitiatorTransaction::processNeighborAmountReservationResponse()
{
    debug() << "processNeighborAmountReservationResponse";
    if (! contextIsValid(Message::Payments_IntermediateNodeCycleReservationResponse)) {
        debug() << "No neighbor node response received.";
        rollBack();
        mCyclesManager->addOfflineNode(mNextNode);
        return resultDone();
    }

    auto message = popNextMessage<IntermediateNodeCycleReservationResponseMessage>();
    // todo: check message sender

    if (message->state() != IntermediateNodeCycleReservationResponseMessage::Accepted) {
        error() << "Neighbor node doesn't approved reservation request";
        rollBack();
        mCyclesManager->addClosedTrustLine(
            currentNodeUUID(),
            mNextNode);
        return resultDone();
    }

    debug() << "(" << message->senderUUID << ") approved reservation request.";
    auto path = mPathStats.get();
    path->setNodeState(
        1, PathStats::NeighbourReservationApproved);
    path->shortageMaxFlow(message->amountReserved());
    return runAmountReservationStage();
}

TransactionResult::SharedConst CycleCloserInitiatorTransaction::processNeighborFurtherReservationResponse()
{
    debug() << "processNeighborFurtherReservationResponse";
    if (! contextIsValid(Message::Payments_CoordinatorCycleReservationResponse)) {
        debug() << "Neighbor node doesn't sent coordinator response.";
        rollBack();
        mCyclesManager->addOfflineNode(
            mNextNode);
        return resultDone();
    }

    auto message = popNextMessage<CoordinatorCycleReservationResponseMessage>();
    if (message->state() != CoordinatorCycleReservationResponseMessage::Accepted) {
        debug() << "Neighbor node doesn't accepted coordinator request.";
        rollBack();
        mCyclesManager->addClosedTrustLine(
            currentNodeUUID(),
            mNextNode);
        return resultDone();
    }

    auto path = mPathStats.get();
    path->setNodeState(
        1,
        PathStats::ReservationApproved);
    debug() << "Neighbor node accepted coordinator request. Reserved: " << message->amountReserved();

    path->shortageMaxFlow(message->amountReserved());
    debug() << "Path max flow is now " << path->maxFlow();

    // shortage reservation
    // TODO maby add if change path->maxFlow()
    for (auto const &itNodeAndReservations : mReservations) {
        auto nodeReservations = itNodeAndReservations.second;
        if (nodeReservations.size() != 1) {
            throw ValueError("CycleCloserInitiatorTransaction::processRemoteNodeResponse: "
                                 "unexpected behaviour: between two nodes should be only one reservation.");
        }
        shortageReservation(
            itNodeAndReservations.first,
            (*nodeReservations.begin()).second,
            path->maxFlow(),
            0);
    }

    return runAmountReservationStage();
}

TransactionResult::SharedConst CycleCloserInitiatorTransaction::askRemoteNodeToApproveReservation(
    const NodeUUID& remoteNode,
    const byte remoteNodePosition,
    const NodeUUID& nextNodeAfterRemote)
{
    debug() << "askRemoteNodeToApproveReservation";
    const auto kCoordinator = currentNodeUUID();
    const auto kTransactionUUID = currentTransactionUUID();
    auto path = mPathStats.get();

    sendMessage<CoordinatorCycleReservationRequestMessage>(
        remoteNode,
        kCoordinator,
        kTransactionUUID,
        path->maxFlow(),
        nextNodeAfterRemote);

    path->setNodeState(
        remoteNodePosition,
        PathStats::ReservationRequestSent);

    if (path->isLastIntermediateNodeProcessed()) {
        debug() << "Further amount reservation request sent to the last node (" << remoteNode << ") ["
               << path->maxFlow() << ", next node - (" << nextNodeAfterRemote << ")]";
        mStep = Coordinator_PreviousNeighborRequestProcessing;
        return resultWaitForMessageTypes(
            {Message::Payments_IntermediateNodeCycleReservationRequest,
            Message::Payments_CoordinatorCycleReservationResponse},
            maxNetworkDelay(2));
    }
    debug() << "Further amount reservation request sent to the node (" << remoteNode << ") ["
           << path->maxFlow() << ", next node - (" << nextNodeAfterRemote << ")]";

    // Response from te remote node will go throught other nodes in the path.
    // So them would be able to shortage it's reservations (if needed).
    // Total wait timeout must take note of this.
    return resultWaitForMessageTypes(
        {Message::Payments_CoordinatorCycleReservationResponse},
        maxNetworkDelay(4));
}

TransactionResult::SharedConst CycleCloserInitiatorTransaction::processRemoteNodeResponse()
{
    debug() << "processRemoteNodeResponse";
    if (! contextIsValid(Message::Payments_CoordinatorCycleReservationResponse)){
        error() << "Remoute node doesn't sent coordinator response. Can't pay.";
        dropReservationsOnPath(
            mPathStats.get(),
            0);
        mCyclesManager->addOfflineNode(
            mPathStats.get()->currentIntermediateNodeAndPos().first);
        return resultDone();
    }

    const auto message = popNextMessage<CoordinatorCycleReservationResponseMessage>();
    auto path = mPathStats.get();

    /*
     * Nodes scheme:
     * R - remote node;
     */

    const auto R_UUIDAndPos = path->currentIntermediateNodeAndPos();
    const auto R_PathPosition = R_UUIDAndPos.second;

    if (0 == message->amountReserved()) {
        path->setUnusable();
        path->setNodeState(
            R_PathPosition,
            PathStats::ReservationRejected);

        debug() << "Remote node rejected reservation. Can't pay";
        rollBack();
        mCyclesManager->addClosedTrustLine(
            path->path()->nodes.at(R_PathPosition),
            path->path()->nodes.at(R_PathPosition + 1));
        return resultDone();

    } else {
        const auto reservedAmount = message->amountReserved();

        path->shortageMaxFlow(reservedAmount);
        path->setNodeState(
            R_PathPosition,
            PathStats::ReservationApproved);

        // shortage reservation
        // TODO maby add if change path->maxFlow()
        for (auto const &itNodeAndReservations : mReservations) {
            auto nodeReservations = itNodeAndReservations.second;
            if (nodeReservations.size() != 1) {
                throw ValueError("CycleCloserInitiatorTransaction::processRemoteNodeResponse: "
                                     "unexpected behaviour: between two nodes should be only one reservation.");
            }
            shortageReservation(
                itNodeAndReservations.first,
                (*nodeReservations.begin()).second,
                path->maxFlow(),
                0);
        }

        debug() << "(" << message->senderUUID << ") reserved " << reservedAmount;
        debug() << "Path max flow is now " << path->maxFlow();

        if (path->isLastIntermediateNodeProcessed()) {

            const auto kTotalAmount = mPathStats.get()->maxFlow();
            debug() << "Current path reservation finished";
            debug() << "Total collected amount by cycle: " << kTotalAmount;
            debug() << "Total count of all participants without coordinator is " << path->path()->intermediateUUIDs().size();

            // send final path amount to all intermediate nodes on path
            sendFinalPathConfiguration(
                mPathStats.get(),
                path->maxFlow());

            for (const auto node : path->path()->intermediateUUIDs()) {
                mFinalAmountNodesConfirmation.insert(
                    make_pair(
                        node,
                        false));
            }

            mStep = Coordinator_FinalAmountsConfigurationConfirmation;
            return resultWaitForMessageTypes(
                {Message::Payments_FinalAmountsConfigurationResponse},
                maxNetworkDelay(3));
        }

        debug() << "Go to the next node in path";
        return tryReserveNextIntermediateNodeAmount();
    }
}

TransactionResult::SharedConst CycleCloserInitiatorTransaction::runPreviousNeighborRequestProcessingStage()
{
    debug() << "runPreviousNeighborRequestProcessingStage";
    if (! contextIsValid(Message::Payments_IntermediateNodeCycleReservationRequest, false))
        return reject("No amount reservation request was received. Rolled back.");

    const auto kMessage = popNextMessage<IntermediateNodeCycleReservationRequestMessage>();
    mPreviousNode = kMessage->senderUUID;
    debug() << "Coordiantor payment operation from node (" << mPreviousNode << ")";
    debug() << "Requested amount reservation: " << kMessage->amount();

    const auto kIncomingAmounts = mTrustLines->availableIncomingCycleAmounts(mPreviousNode);
    const auto kIncomingAmountWithReservations = kIncomingAmounts.first;
    const auto kIncomingAmountWithoutReservations = kIncomingAmounts.second;
    debug() << "IncomingAmountWithReservations: " << *kIncomingAmountWithReservations
            << " IncomingAmountWithoutReservations: " << *kIncomingAmountWithoutReservations;
    if (*kIncomingAmountWithReservations == TrustLine::kZeroAmount()) {
        if (*kIncomingAmountWithoutReservations == TrustLine::kZeroAmount()) {
            sendMessage<IntermediateNodeCycleReservationResponseMessage>(
                mPreviousNode,
                currentNodeUUID(),
                currentTransactionUUID(),
                ResponseCycleMessage::Rejected);
            debug() << "can't reserve requested amount, because coordinator incoming amount "
                "event without reservations equal zero, transaction closed";
            return resultDone();
        } else {
            mIncomingAmount = TrustLineAmount(0);
        }
    } else {
        mIncomingAmount = min(
            kMessage->amount(),
            *kIncomingAmountWithReservations);
    }

    if (0 == mIncomingAmount) {
        auto reservations = mTrustLines->reservationsFromContractor(mPreviousNode);
        for (auto reservation : reservations) {
            if (mCyclesManager->resolveReservationConflict(
                currentTransactionUUID(), reservation->transactionUUID())) {
                mConflictedTransaction = reservation->transactionUUID();
                mStep = Cycles_WaitForIncomingAmountReleasing;
                mIncomingAmount = min(
                    reservation->amount(),
                    kMessage->amount());
                return resultAwaikAfterMilliseconds(
                    kWaitingForReleasingAmountMSec);
            }
        }
    }

    if (0 == mIncomingAmount || ! reserveIncomingAmount(mPreviousNode, mIncomingAmount, 0)) {
        sendMessage<IntermediateNodeCycleReservationResponseMessage>(
            mPreviousNode,
            currentNodeUUID(),
            currentTransactionUUID(),
            ResponseCycleMessage::Rejected);
        return reject("No incoming amount reservation is possible. Rolled back.");
    }

    debug() << "send accepted message with reserve (" << mIncomingAmount << ") to node " << mPreviousNode;
    sendMessage<IntermediateNodeCycleReservationResponseMessage>(
        mPreviousNode,
        currentNodeUUID(),
        currentTransactionUUID(),
        ResponseCycleMessage::Accepted,
        mIncomingAmount);

    mStep = Coordinator_AmountReservation;
    return resultWaitForMessageTypes(
        {Message::Payments_CoordinatorCycleReservationResponse},
        maxNetworkDelay(2));
}

TransactionResult::SharedConst CycleCloserInitiatorTransaction::runPreviousNeighborRequestProcessingStageAgain()
{
    debug() << "runPreviousNeighborRequestProcessingStageAgain";

    if (mCyclesManager->isTransactionStillAlive(
        mConflictedTransaction)) {
        debug() << "wait again";
        return resultAwaikAfterMilliseconds(
            kWaitingForReleasingAmountMSec);
    }

    if (! reserveIncomingAmount(mPreviousNode, mIncomingAmount, 0)) {
        sendMessage<IntermediateNodeCycleReservationResponseMessage>(
            mPreviousNode,
            currentNodeUUID(),
            currentTransactionUUID(),
            ResponseCycleMessage::Rejected);
        return reject("No incoming amount reservation is possible. Rolled back.");
    }

    debug() << "send accepted message with reserve (" << mIncomingAmount << ") to node " << mPreviousNode;
    sendMessage<IntermediateNodeCycleReservationResponseMessage>(
        mPreviousNode,
        currentNodeUUID(),
        currentTransactionUUID(),
        ResponseCycleMessage::Accepted,
        mIncomingAmount);

    mStep = Coordinator_AmountReservation;
    return resultWaitForMessageTypes(
        {Message::Payments_CoordinatorCycleReservationResponse},
        maxNetworkDelay(2));
}

TransactionResult::SharedConst CycleCloserInitiatorTransaction::runFinalAmountsConfigurationConfirmationProcessingStage()
{
    debug() << "runFinalAmountsConfigurationConfirmationProcessingStage";
    if (contextIsValid(Message::MessageType::Payments_FinalAmountsConfigurationResponse, false)) {
        auto kMessage = popNextMessage<FinalAmountsConfigurationResponseMessage>();
        if (mFinalAmountNodesConfirmation.find(kMessage->senderUUID) == mFinalAmountNodesConfirmation.end()) {
            // todo : need actual action on this case
            error() << "Sender is not participant of this transaction";
            return resultWaitForMessageTypes(
                {Message::Payments_FinalAmountsConfigurationResponse},
                maxNetworkDelay(2));
        }
        if (kMessage->state() == FinalAmountsConfigurationResponseMessage::Rejected) {
            debug() << "Node " << kMessage->senderUUID << " rejected final amounts";
            return reject("Haven't reach consensus on reservation. Transaction rejected.");
        }
        debug() << "Node " << kMessage->senderUUID << " confirmed final amounts";
        mFinalAmountNodesConfirmation[kMessage->senderUUID] = true;
        for (const auto nodeUUIDAndConfirmation : mFinalAmountNodesConfirmation) {
            if (!nodeUUIDAndConfirmation.second) {
                debug() << "Some nodes are still not confirmed final amounts. Waiting.";
                return resultWaitForMessageTypes(
                    {Message::Payments_FinalAmountsConfigurationResponse},
                    maxNetworkDelay(1));
            }
        }
        debug() << "All nodes confirmed final configuration. Begin processing participants votes.";
        return propagateVotesListAndWaitForVoutingResult();
    }
    return reject("Some nodes didn't confirm final amount configuration. Transaction rejected.");
}

TransactionResult::SharedConst CycleCloserInitiatorTransaction::runVotesConsistencyCheckingStage()
{
    debug() << "runVotesConsistencyCheckingStage";
    if (! contextIsValid(Message::Payments_ParticipantsVotes)) {
        return reject("Coordinator didn't receive message with votes");
    }

    const auto kMessage = popNextMessage<ParticipantsVotesMessage>();
    debug () << "Participants votes message received.";

    mParticipantsVotesMessage = kMessage;
    if (mParticipantsVotesMessage->containsRejectVote()) {
        return reject("Some participant node has been rejected the transaction. Rolling back.");
    }

    if (mParticipantsVotesMessage->achievedConsensus()){
        debug() << "Coordinator received achieved consensus message.";
        if (!checkReservationsDirections()) {
            return reject("Reservations on node are invalid");
        }
        mParticipantsVotesMessage->addParticipant(currentNodeUUID());
        mParticipantsVotesMessage->approve(currentNodeUUID());
        propagateVotesMessageToAllParticipants(mParticipantsVotesMessage);
        return approve();

    }

    return reject("Coordinator received message with some uncertain votes. Rolling back");
}

void CycleCloserInitiatorTransaction::checkPath(
    const Path::ConstShared path)
{
    if (path->length() < 3 || path->length() > 7) {
        throw ValueError("CycleCloserInitiatorTransaction::checkPath: "
                             "invalid paths length");
    }
    if (path->sourceUUID() != path->destinationUUID()) {
        throw ValueError("CycleCloserInitiatorTransaction::checkPath: "
                             "path isn't cycle");
    }
    for (const auto node : path->intermediateUUIDs()) {
        if (node == path->sourceUUID() || node == path->destinationUUID()) {
            throw ValueError("CycleCloserInitiatorTransaction::checkPath: "
                                 "paths contains repeated nodes");
        }
    }
    auto itGlobal = path->nodes.begin() + 1;
    while (itGlobal != path->nodes.end() - 2) {
        auto itLocal = itGlobal + 1;
        while (itLocal != path->nodes.end() - 1) {
            if (*itGlobal == *itLocal) {
                throw ValueError("CycleCloserInitiatorTransaction::checkPath: "
                                     "paths contains repeated nodes");
            }
            itLocal++;
        }
        itGlobal++;
    }
}

void CycleCloserInitiatorTransaction::sendFinalPathConfiguration(
    PathStats* pathStats,
    const TrustLineAmount &finalPathAmount)
{
    debug() << "sendFinalPathConfiguration";
    for (const auto &intermediateNode : pathStats->path()->intermediateUUIDs()) {
        debug() << "send message with final path amount info for node " << intermediateNode;
        sendMessage<FinalPathCycleConfigurationMessage>(
            intermediateNode,
            currentNodeUUID(),
            currentTransactionUUID(),
            finalPathAmount);
    }
}

void CycleCloserInitiatorTransaction::savePaymentOperationIntoHistory()
{
    debug() << "savePaymentOperationIntoHistory";
    auto path = mPathStats.get();
    auto ioTransaction = mStorageHandler->beginTransaction();
    ioTransaction->historyStorage()->savePaymentRecord(
        make_shared<PaymentRecord>(
            currentTransactionUUID(),
            PaymentRecord::PaymentOperationType::CycleCloserType,
            path->maxFlow()));
}

bool CycleCloserInitiatorTransaction::checkReservationsDirections() const
{
    debug() << "checkReservationsDirections";
    if (mReservations.size() != 2) {
        error() << "Wrong nodes reservations size: " << mReservations.size();
        return false;
    }

    auto firstNodeReservation = mReservations.begin()->second;
    auto secondNodeReservation = mReservations.end() ->second;
    if (firstNodeReservation.size() != 1 || secondNodeReservation.size() != 1) {
        error() << "Wrong reservations size";
        return false;
    }
    const auto firstReservation = firstNodeReservation.at(0);
    const auto secondReservation = secondNodeReservation.at(0);
    if (firstReservation.first != secondReservation.first) {
        error() << "Reservations on different ways";
        return false;
    }
    if (firstReservation.second->amount() != secondReservation.second->amount()) {
        error() << "Different reservations amount";
        return false;
    }
    if (firstReservation.second->direction() == secondReservation.second->direction()) {
        error() << "Wrong directions";
        return false;
    }
    return true;
}

const NodeUUID& CycleCloserInitiatorTransaction::coordinatorUUID() const
{
    return currentNodeUUID();
}

const uint8_t CycleCloserInitiatorTransaction::cycleLength() const
{
    return (uint8_t)mPathStats->path()->length();
}

const string CycleCloserInitiatorTransaction::logHeader() const
{
    stringstream s;
    s << "[CycleCloserInitiatorTA: " << currentTransactionUUID() << "] ";
    return s.str();
}