#include "OutgoingRemoteAddressNode.h"

OutgoingRemoteAddressNode::OutgoingRemoteAddressNode(
    BaseAddress::Shared address,
    UDPSocket &socket,
    IOService &ioService,
    Logger &logger)
    noexcept :

    mAddress(address),
    OutgoingRemoteBaseNode(
        socket,
        ioService,
        logger
    )
{}

void OutgoingRemoteAddressNode::sendMessage(
    Message::Shared message)
    noexcept
{
    try {
        // In case if queue already contains packets -
        // then async handler is already scheduled.
        // Otherwise - it must be initialised.
        bool packetsSendingAlreadyScheduled = !mPacketsQueue.empty();

        auto bytesAndBytesCount = preprocessMessage(message);
        if (bytesAndBytesCount.second > Message::maxSize()) {
            errors() << "Message is too big to be transferred via the network";
            return;
        }

#ifdef DEBUG_LOG_NETWORK_COMMUNICATOR
        const Message::SerializedType kMessageType =
            *(reinterpret_cast<Message::SerializedType*>(
                bytesAndBytesCount.first.get() + sizeof(SerializedProtocolVersion)));

        debug()
            << "Message of type "
            << static_cast<size_t>(kMessageType)
            << " postponed for the sending";
#endif

        populateQueueWithNewPackets(
            bytesAndBytesCount.first.get(),
            bytesAndBytesCount.second);

        if (not packetsSendingAlreadyScheduled) {
            beginPacketsSending();
        }

    } catch (exception &e) {
        errors()
            << "Exception occurred: "
            << e.what();
    }
}

void OutgoingRemoteAddressNode::beginPacketsSending()
{
    if (mPacketsQueue.empty()) {
        return;
    }

    UDPEndpoint endpoint;
    try {
        endpoint = as::ip::udp::endpoint(
            as::ip::address_v4::from_string(mAddress->host()),
            mAddress->port());
        debug() << "Endpoint address " << endpoint.address().to_string();
        debug() << "Endpoint port " << endpoint.port();
    } catch  (exception &) {
        errors()
            << "Endpoint can't be fetched from Contractor. "
            << "No messages can be sent. Outgoing queue cleared.";

        while (!mPacketsQueue.empty()) {
            const auto packetDataAndSize = mPacketsQueue.front();
            free(packetDataAndSize.first);
            mPacketsQueue.pop();
        }

        return;
    }

    // The next code inserts delay between sending packets in case of high traffic.
    const auto kShortSendingTimeInterval = boost::posix_time::milliseconds(20);
    const auto kTimeoutFromLastSending = boost::posix_time::microsec_clock::universal_time() - mCyclesStats.first;
    if (kTimeoutFromLastSending < kShortSendingTimeInterval) {
        // Increasing short sendings counter.
        mCyclesStats.second += 1;

        const auto kMaxShortSendings = 30;
        if (mCyclesStats.second > kMaxShortSendings) {
            mCyclesStats.second = 0;
            mSendingDelayTimer.expires_from_now(kShortSendingTimeInterval);
            mSendingDelayTimer.async_wait([this] (const boost::system::error_code &_){
                this->beginPacketsSending();
                debug() << "Sending delayed";

            });
            return;
        }

    } else {
        mCyclesStats.second = 0;
    }

    const auto packetDataAndSize = mPacketsQueue.front();
    mSocket.async_send_to(
        boost::asio::buffer(
            packetDataAndSize.first,
            packetDataAndSize.second),
        endpoint,
        [this, endpoint] (const boost::system::error_code &error, const size_t bytesTransferred) {

            const auto packetDataAndSize = mPacketsQueue.front();
            if (bytesTransferred != packetDataAndSize.second) {
                if (error) {
                    errors()
                        << "OutgoingRemoteNode::beginPacketsSending: "
                        << "Next packet can't be sent to the node (" << mAddress->fullAddress() << "). "
                        << "Error code: " << error.value();
                }

                // Removing packet from the memory
                free(packetDataAndSize.first);
                mPacketsQueue.pop();
                if (!mPacketsQueue.empty()) {
                    beginPacketsSending();
                }

                return;
            }

#ifdef DEBUG_LOG_NETWORK_COMMUNICATOR
            const PacketHeader::ChannelIndex channelIndex =
                 *(new(packetDataAndSize.first + PacketHeader::kChannelIndexOffset) PacketHeader::ChannelIndex);

            const PacketHeader::PacketIndex packetIndex =
                 *(new(packetDataAndSize.first + PacketHeader::kPacketIndexOffset) PacketHeader::PacketIndex) + 1;

            const PacketHeader::TotalPacketsCount totalPacketsCount =
                 *(new(packetDataAndSize.first + PacketHeader::kPacketsCountOffset) PacketHeader::TotalPacketsCount);

            this->debug()
                << setw(4) << bytesTransferred <<  "B TX [ => ] "
                << endpoint.address() << ":" << endpoint.port() << "; "
                << "Channel: " << setw(10) << static_cast<size_t>(channelIndex) << "; "
                << "Packet: " << setw(3) << static_cast<size_t>(packetIndex)
                << "/" << static_cast<size_t>(totalPacketsCount);
#endif

            // Removing packet from the memory
            free(packetDataAndSize.first);
            mPacketsQueue.pop();

            mCyclesStats.first = boost::posix_time::microsec_clock::universal_time();
            if (!mPacketsQueue.empty()) {
                beginPacketsSending();
            }
        });
}

LoggerStream OutgoingRemoteAddressNode::errors() const
{
    return mLog.warning(
        string("Communicator / OutgoingRemoteAddressNode [")
        + mAddress->fullAddress()
        + string("]"));
}

LoggerStream OutgoingRemoteAddressNode::debug() const
{
#ifdef DEBUG_LOG_NETWORK_COMMUNICATOR
    return mLog.debug(
        string("Communicator / OutgoingRemoteAddressNode [")
        + mAddress->fullAddress()
        + string("]"));
#endif

#ifndef DEBUG_LOG_NETWORK_COMMUNICATOR
    return LoggerStream::dummy();
#endif
}
