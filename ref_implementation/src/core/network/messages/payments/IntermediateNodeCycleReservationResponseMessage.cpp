/**
 * This file is part of GEO Protocol.
 * It is subject to the license terms in the LICENSE.md file found in the top-level directory
 * of this distribution and at https://github.com/GEO-Protocol/GEO-network-client/blob/master/LICENSE.md
 *
 * No part of GEO Protocol, including this file, may be copied, modified, propagated, or distributed
 * except according to the terms contained in the LICENSE.md file.
 */

#include "IntermediateNodeCycleReservationResponseMessage.h"

IntermediateNodeCycleReservationResponseMessage::IntermediateNodeCycleReservationResponseMessage(
    const NodeUUID& senderUUID,
    const TransactionUUID& transactionUUID,
    const ResponseCycleMessage::OperationState state,
    const TrustLineAmount& reservedAmount):

    ResponseCycleMessage(
        senderUUID,
        transactionUUID,
        state),
    mAmountReserved(reservedAmount)
{}

IntermediateNodeCycleReservationResponseMessage::IntermediateNodeCycleReservationResponseMessage(
    BytesShared buffer) :

    ResponseCycleMessage(
        buffer)
{
    auto parentMessageOffset = ResponseCycleMessage::kOffsetToInheritedBytes();
    auto amountOffset = buffer.get() + parentMessageOffset;
    auto amountEndOffset = amountOffset + kTrustLineAmountBytesCount; // TODO: deserialize only non-zero
    vector<byte> amountBytes(
        amountOffset,
        amountEndOffset);

    mAmountReserved = bytesToTrustLineAmount(amountBytes);
}

const TrustLineAmount& IntermediateNodeCycleReservationResponseMessage::amountReserved() const
{
    return mAmountReserved;
}

pair<BytesShared, size_t> IntermediateNodeCycleReservationResponseMessage::serializeToBytes() const
    throw(bad_alloc)
{
    auto parentBytesAndCount = ResponseCycleMessage::serializeToBytes();
    auto serializedAmount = trustLineAmountToBytes(mAmountReserved);

    size_t bytesCount =
        parentBytesAndCount.second
        + serializedAmount.size();

    BytesShared dataBytesShared = tryMalloc(bytesCount);
    size_t dataBytesOffset = 0;
    //----------------------------------------------------
    memcpy(
        dataBytesShared.get(),
        parentBytesAndCount.first.get(),
        parentBytesAndCount.second);
    dataBytesOffset += parentBytesAndCount.second;
    //----------------------------------------------------
    memcpy(
        dataBytesShared.get() + dataBytesOffset,
        serializedAmount.data(),
        serializedAmount.size());
    //----------------------------------------------------
    return make_pair(
        dataBytesShared,
        bytesCount);
}

const Message::MessageType IntermediateNodeCycleReservationResponseMessage::typeID() const
{
    return Message::Payments_IntermediateNodeCycleReservationResponse;
}
