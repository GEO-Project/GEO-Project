#include "MaxFlowCalculationMessage.h"

MaxFlowCalculationMessage::MaxFlowCalculationMessage() {}

MaxFlowCalculationMessage::MaxFlowCalculationMessage(
    const NodeUUID &senderUUID,
    const NodeUUID &targetUUID,
    const TransactionUUID &transactionUUID) :

    TransactionMessage(
    senderUUID,
    transactionUUID),

    mTargetUUID(targetUUID){}

const NodeUUID &MaxFlowCalculationMessage::targetUUID() const {

    return mTargetUUID;
}

pair<BytesShared, size_t> MaxFlowCalculationMessage::serializeToBytes() {

    auto parentBytesAndCount = TransactionMessage::serializeToBytes();

    size_t bytesCount = parentBytesAndCount.second +
                        NodeUUID::kBytesSize;

    BytesShared dataBytesShared = tryCalloc(bytesCount);
    size_t dataBytesOffset = 0;
    //----------------------------------------------------
    memcpy(
        dataBytesShared.get(),
        parentBytesAndCount.first.get(),
        parentBytesAndCount.second
    );
    dataBytesOffset += parentBytesAndCount.second;
    //----------------------------------------------------
    memcpy(
        dataBytesShared.get() + dataBytesOffset,
        mTargetUUID.data,
        NodeUUID::kBytesSize
    );
    //----------------------------------------------------
    return make_pair(
        dataBytesShared,
        bytesCount
    );
}

void MaxFlowCalculationMessage::deserializeFromBytes(
    BytesShared buffer) {

    TransactionMessage::deserializeFromBytes(buffer);
    size_t bytesBufferOffset = TransactionMessage::kOffsetToInheritedBytes();
    //----------------------------------------------------
    memcpy(
        mTargetUUID.data,
        buffer.get() + bytesBufferOffset,
        NodeUUID::kBytesSize
    );
}

const size_t MaxFlowCalculationMessage::kOffsetToInheritedBytes() {

    static const size_t offset = TransactionMessage::kOffsetToInheritedBytes()
                                 + NodeUUID::kBytesSize;
    return offset;
}
