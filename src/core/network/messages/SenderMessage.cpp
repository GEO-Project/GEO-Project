#include "SenderMessage.h"


SenderMessage::SenderMessage(
    const NodeUUID &senderUUID)
    noexcept :

    senderUUID(senderUUID)
{}

SenderMessage::SenderMessage(
    BytesShared buffer)
    noexcept
{
    memcpy(
        const_cast<NodeUUID*>(&senderUUID),
        buffer.get() + Message::kOffsetToInheritedBytes(),
        NodeUUID::kBytesSize);
}

/**
 * @throws bad_alloc;
 */
pair<BytesShared, size_t> SenderMessage::serializeToBytes() const
    noexcept(false)
{
    BytesSerializer serializer;

    serializer.enqueue(Message::serializeToBytes());
    serializer.enqueue(senderUUID);
    return serializer.collect();
}

const size_t SenderMessage::kOffsetToInheritedBytes() const
    noexcept
{
    static const auto kOffset =
        Message::kOffsetToInheritedBytes()
        + NodeUUID::kBytesSize;

    return kOffset;
}
