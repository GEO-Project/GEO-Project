#ifndef GEO_NETWORK_CLIENT_OUTGOINGMESSAGESHANDLER_H
#define GEO_NETWORK_CLIENT_OUTGOINGMESSAGESHANDLER_H

#include "OutgoingNodesHandler.h"
#include "../../../../providing/ProvidingHandler.h"

#include "../common/Types.h"


class OutgoingMessagesHandler {
public:
    OutgoingMessagesHandler(
        IOService &ioService,
        UDPSocket &socket,
        ContractorsManager *contractorsManager,
        ProvidingHandler *providingHandler,
        Logger &log)
        noexcept;

    void sendMessage(
        const Message::Shared message,
        const ContractorID addressee);

    void sendMessage(
        const Message::Shared message,
        const BaseAddress::Shared address);

private:
    void onPingMessageToProviderReady(
        Provider::Shared provider);

    MsgEncryptor::Buffer pingMessage(
        Provider::Shared provider) const;

    MsgEncryptor::Buffer getRemoteNodeAddressMessage(
        Provider::Shared provider,
        GNSAddress::Shared gnsAddress) const;

protected:
    Logger &mLog;
    OutgoingNodesHandler mNodes;
    ContractorsManager *mContractorsManager;
    ProvidingHandler *mProvidingHandler;
};


#endif //GEO_NETWORK_CLIENT_OUTGOINGMESSAGESHANDLER_H
