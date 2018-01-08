#include "FinalAmountsConfigurationResponseMessage.h"

FinalAmountsConfigurationResponseMessage::FinalAmountsConfigurationResponseMessage(
    const NodeUUID& senderUUID,
    const TransactionUUID& transactionUUID,
    const OperationState state) :

    TransactionMessage(
        senderUUID,
        transactionUUID),
    mState(state)
{}

FinalAmountsConfigurationResponseMessage::FinalAmountsConfigurationResponseMessage(
    BytesShared buffer):

    TransactionMessage(buffer)
{
    size_t bytesBufferOffset = TransactionMessage::kOffsetToInheritedBytes();
    //----------------------------------------------------
    SerializedOperationState *state = new (buffer.get() + bytesBufferOffset) SerializedOperationState;
    mState = (OperationState) (*state);
}

const FinalAmountsConfigurationResponseMessage::OperationState FinalAmountsConfigurationResponseMessage::state() const
{
    return mState;
}

/**
 *
 * @throws bad_alloc;
 */
pair<BytesShared, size_t> FinalAmountsConfigurationResponseMessage::serializeToBytes() const
    throw (bad_alloc)
{
    auto parentBytesAndCount = TransactionMessage::serializeToBytes();

    size_t bytesCount =
        parentBytesAndCount.second
        + sizeof(SerializedOperationState);

    BytesShared dataBytesShared = tryMalloc(bytesCount);
    size_t dataBytesOffset = 0;
    //----------------------------------------------------
    memcpy(
        dataBytesShared.get(),
        parentBytesAndCount.first.get(),
        parentBytesAndCount.second);
    dataBytesOffset += parentBytesAndCount.second;
    //----------------------------------------------------
    SerializedOperationState state(mState);
    memcpy(
        dataBytesShared.get() + dataBytesOffset,
        &state,
        sizeof(SerializedOperationState));
    //----------------------------------------------------
    return make_pair(
        dataBytesShared,
        bytesCount);
}

const Message::MessageType FinalAmountsConfigurationResponseMessage::typeID() const
{
    return Message::Payments_FinalAmountsConfigurationResponse;
}