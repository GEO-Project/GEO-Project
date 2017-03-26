﻿#include "ReceiverPaymentTransaction.h"


ReceiverPaymentTransaction::ReceiverPaymentTransaction(
    const NodeUUID &currentNodeUUID,
    ReceiverInitPaymentRequestMessage::ConstShared message,
    TrustLinesManager *trustLines,
    Logger *log) :

    BasePaymentTransaction(
        BaseTransaction::ReceiverPaymentTransaction,
        message->transactionUUID(),
        currentNodeUUID,
        trustLines,
        log),
    mMessage(message),
    mTotalReserved(0)
{}

ReceiverPaymentTransaction::ReceiverPaymentTransaction(
    BytesShared buffer,
    TrustLinesManager *trustLines,
    Logger *log) :

    BasePaymentTransaction(
        BaseTransaction::ReceiverPaymentTransaction,
        buffer,
        trustLines,
        log)
{
    deserializeFromBytes(buffer);
}

/**
 * @throws RuntimeError in case if current stage is invalid.
 * @throws Exception from inner logic
 */
TransactionResult::SharedConst ReceiverPaymentTransaction::run()
{
    switch (mStep) {
    case Stages::CoordinatorRequestApproving:
        return runInitialisationStage();

    case Stages::AmountReservationsProcessing:
        return runAmountReservationStage();

    case Stages::VotesChecking:
        return runVotesCheckingStage();

    default:
        throw RuntimeError(
            "ReceiverPaymentTransaction::run(): "
            "invalid stage number occurred");
    }
}

pair<BytesShared, size_t> ReceiverPaymentTransaction::serializeToBytes()
{
    throw ValueError("Not implemented");
}

void ReceiverPaymentTransaction::deserializeFromBytes(BytesShared buffer)
{
    throw ValueError("Not implemented");
}

const string ReceiverPaymentTransaction::logHeader() const
{
    stringstream s;
    s << "[ReceiverPaymentTA: " << UUID().stringUUID() << "] ";
    return s.str();
}

TransactionResult::SharedConst ReceiverPaymentTransaction::runInitialisationStage()
{
    const auto kCoordinator = mMessage->senderUUID();

    info() << "Operation for " << mMessage->amount() << " initialised by the (" << kCoordinator << ")";

    // TODO: (optimisation)
    // Check if total incoming possibilities of the node are <= of the payment amount.
    // If not - there is no reason to process the operation at all.
    // (reject operation)

    sendMessage<ReceiverInitPaymentResponseMessage>(
        kCoordinator,
        nodeUUID(),
        UUID(),
        ReceiverInitPaymentResponseMessage::Accepted);


    // Begin waiting for amount reservation requests.
    // There is non-zero probability, that first couple of paths will fail.
    // So receiver will wait for time, that is approximatyle neede for several nodes for processing.
    //
    // TODO: enhancement: send aproximate paths count to receiver, so it will be able to wait correct timeout.
    mStep = Stages::AmountReservationsProcessing;
    return resultWaitForMessageTypes(
        {Message::Payments_IntermediateNodeReservationRequest},
        maxTimeout(kMaxPathLength*3));
}

TransactionResult::SharedConst ReceiverPaymentTransaction::runAmountReservationStage()
{
    if (! contextIsValid(Message::Payments_IntermediateNodeReservationRequest))
        return reject("No amount reservation request was received. Rolled back.");


    const auto kMessage = popNextMessage<IntermediateNodeReservationRequestMessage>();
    const auto kNeighbor = kMessage->senderUUID();

    if (! mTrustLines->isNeighbor(kNeighbor)) {
        // Message was sent from node, that is not listed in neighbors list.
        //
        // TODO: enhance this check
        // Neighbor public key must be used here.

        // Ignore received message.
        // Begin waiting for another one.
        //
        // TODO: enhancement: send aproximate paths count to receiver, so it will be able to wait correct timeout.
        return resultWaitForMessageTypes(
            {Message::Payments_IntermediateNodeReservationRequest},
            maxTimeout(kMaxPathLength*3));
    }


    info() << "Amount reservation for " << kMessage->amount() << " request received from " << kNeighbor;

    // Note: copy of shared pointer is required.
    const auto kAvailableAmount = mTrustLines->availableIncomingAmount(kNeighbor);
    if (kMessage->amount() > *kAvailableAmount || ! reserveIncomingAmount(kNeighbor, kMessage->amount())) {
        // Receiver must not confirm reservation in case if requested amount is less than available.
        // Intermediate nodes may decrease requested reservation amount, but receiver must not do this.
        // It must stay synchronised with previous node.
        // So, in case if requested amount is less than available -
        // previous node must report about it to the coordinator.
        // In this case, receiver should even not receive reservation request at all.
        //
        // Also, this kind of problem may appear when two nodes are involved
        // in several COUNTER transactions.
        // In this case, balances may be reserved on the nodes,
        // but neighbor node may reject reservation,
        // because it already reserved amount or other transactions,
        // that wasn't approved by the current node yet.
        //
        // In this case, reservation request must be rejected.

        sendMessage<IntermediateNodeReservationResponseMessage>(
            kNeighbor,
            nodeUUID(),
            UUID(),
            ResponseMessage::Rejected);

        // Begin accepting other reservation messages
        return resultWaitForMessageTypes(
            {Message::Payments_IntermediateNodeReservationRequest},
            maxTimeout(kMaxPathLength*3));

    }

    const auto kTotalTransactionAmount = mMessage->amount();
    mTotalReserved += kMessage->amount();
    if (mTotalReserved > kTotalTransactionAmount){
        sendMessage<IntermediateNodeReservationResponseMessage>(
            kNeighbor,
            nodeUUID(),
            UUID(),
            ResponseMessage::Rejected);

        return reject(
            "Reserved amount is greater than requested. "
            "It indicates protocol or realisation error. "
            "Rolled back.");
    }


    info() << "Reserved locally: " << kMessage->amount();
    sendMessage<IntermediateNodeReservationResponseMessage>(
        kNeighbor,
        nodeUUID(),
        UUID(),
        ResponseMessage::Accepted);


    if (mTotalReserved == mMessage->amount()) {
        // Reserved amount is enough to move to votes processing stage.

        // TODO: receiver must know, how many paths are involved into the transaction.
        // This info helps to calculate max timeout,
        // that would be used for waiting for votes list message.

        mStep = Stages::VotesChecking;
        return resultWaitForMessageTypes(
            {Message::Payments_ParticipantsVotes},
            maxTimeout(kMaxPathLength*3));

    } else {
        // Waiting for another reservation request
        return resultWaitForMessageTypes(
            {Message::Payments_IntermediateNodeReservationRequest},
            maxTimeout(kMaxPathLength*3));
    }
}