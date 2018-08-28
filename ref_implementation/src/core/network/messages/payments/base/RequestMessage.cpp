/**
 * This file is part of GEO Protocol.
 * It is subject to the license terms in the LICENSE.md file found in the top-level directory
 * of this distribution and at https://github.com/GEO-Protocol/GEO-network-client/blob/master/LICENSE.md
 *
 * No part of GEO Protocol, including this file, may be copied, modified, propagated, or distributed
 * except according to the terms contained in the LICENSE.md file.
 */

#include "RequestMessage.h"


RequestMessage::RequestMessage(
    const NodeUUID &senderUUID,
    const TransactionUUID &transactionUUID,
    const PathID &pathID,
    const TrustLineAmount &amount) :

    TransactionMessage(
        senderUUID,
        transactionUUID),
    mPathID(pathID),
    mAmount(amount)
{}

RequestMessage::RequestMessage(
    BytesShared buffer):

    TransactionMessage(buffer)
{
    auto parentMessageOffset = TransactionMessage::kOffsetToInheritedBytes();
    auto bytesBufferOffset = buffer.get() + parentMessageOffset;
    PathID *pathID = new (bytesBufferOffset) PathID;
    mPathID = *pathID;
    bytesBufferOffset += sizeof(PathID);
    auto amountEndOffset = bytesBufferOffset + kTrustLineBalanceBytesCount; // TODO: deserialize only non-zero
    vector<byte> amountBytes(
        bytesBufferOffset,
        amountEndOffset);

    mAmount = bytesToTrustLineAmount(amountBytes);
}

const TrustLineAmount &RequestMessage::amount() const
{
    return mAmount;
}

const PathID &RequestMessage::pathID() const
{
    return mPathID;
}

/*!
 *
 * Throws bad_alloc;
 */
pair<BytesShared, size_t> RequestMessage::serializeToBytes() const
    throw(bad_alloc)
{

    auto serializedAmount = trustLineAmountToBytes(mAmount); // TODO: serialize only non-zero
    auto parentBytesAndCount = TransactionMessage::serializeToBytes();
    size_t bytesCount =
        + parentBytesAndCount.second
        + sizeof(PathID)
        + kTrustLineAmountBytesCount;

    BytesShared buffer = tryMalloc(bytesCount);
    auto initialOffset = buffer.get();
    memcpy(
        initialOffset,
        parentBytesAndCount.first.get(),
        parentBytesAndCount.second);

    auto bytesBufferOffset = initialOffset + parentBytesAndCount.second;

    memcpy(
        bytesBufferOffset,
        &mPathID,
        sizeof(PathID));
    bytesBufferOffset += sizeof(PathID);

    memcpy(
        bytesBufferOffset,
        serializedAmount.data(),
        kTrustLineAmountBytesCount);

    return make_pair(
        buffer,
        bytesCount);
}

const size_t RequestMessage::kOffsetToInheritedBytes() const
    noexcept
{
    static const size_t offset =
        TransactionMessage::kOffsetToInheritedBytes()
        + sizeof(PathID)
        + kTrustLineAmountBytesCount;

    return offset;
}

