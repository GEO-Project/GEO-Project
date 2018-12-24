﻿#include "TransactionMessage.h"


TransactionMessage::TransactionMessage(
    const SerializedEquivalent equivalent,
    const TransactionUUID &transactionUUID)
    noexcept :

    SenderMessage(
        equivalent,
        0),
    mTransactionUUID(transactionUUID)
{}

TransactionMessage::TransactionMessage(
    const SerializedEquivalent equivalent,
    ContractorID idOnReceiverSide,
    vector<BaseAddress::Shared> &senderAddresses,
    const TransactionUUID &transactionUUID)
    noexcept:

    SenderMessage(
        equivalent,
        idOnReceiverSide,
        senderAddresses),
    mTransactionUUID(transactionUUID)
{}

TransactionMessage::TransactionMessage(
    const SerializedEquivalent equivalent,
    vector<BaseAddress::Shared> &senderAddresses,
    const TransactionUUID &transactionUUID)
    noexcept:

    SenderMessage(
        equivalent,
        senderAddresses),
    mTransactionUUID(transactionUUID)
{}

TransactionMessage::TransactionMessage(
    const SerializedEquivalent equivalent,
    ContractorID idOnReceiverSide,
    const TransactionUUID &transactionUUID)
    noexcept:

    SenderMessage(
        equivalent,
        idOnReceiverSide),
    mTransactionUUID(transactionUUID)
{}

TransactionMessage::TransactionMessage(
    BytesShared buffer)
    noexcept :

    SenderMessage(buffer),
    mTransactionUUID([&buffer](const size_t parentOffset) -> const TransactionUUID {
        TransactionUUID tu;

        memcpy(
            tu.data,
            buffer.get() + parentOffset,
            TransactionUUID::kBytesSize);

        return tu;
    }(SenderMessage::kOffsetToInheritedBytes()))
{}

const bool TransactionMessage::isTransactionMessage() const
    noexcept
{
    return true;
}

const TransactionUUID &TransactionMessage::transactionUUID() const
    noexcept
{
    return mTransactionUUID;
}

/*
 * ToDo: rewrite me with bytes deserializer
 */
pair<BytesShared, size_t> TransactionMessage::serializeToBytes() const
{
    auto parentBytesAndCount = SenderMessage::serializeToBytes();

    size_t bytesCount = parentBytesAndCount.second
                        + TransactionUUID::kBytesSize;

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
        mTransactionUUID.data,
        TransactionUUID::kBytesSize);
    //----------------------------------------------------
    return make_pair(
        dataBytesShared,
        bytesCount);
}

const size_t TransactionMessage::kOffsetToInheritedBytes() const
    noexcept
{
    const auto kOffset =
          SenderMessage::kOffsetToInheritedBytes()
        + TransactionUUID::kBytesSize;

    return kOffset;
}
