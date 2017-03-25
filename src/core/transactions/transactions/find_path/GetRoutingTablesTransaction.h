#ifndef GEO_NETWORK_CLIENT_GETROUTINGTABLESTRANSACTION_H
#define GEO_NETWORK_CLIENT_GETROUTINGTABLESTRANSACTION_H

#include "../base/BaseTransaction.h"
#include "../../../trust_lines/manager/TrustLinesManager.h"
#include "../../../io/storage/StorageHandler.h"
#include "../../../network/messages/find_path/RequestRoutingTablesMessage.h"
#include "../../../network/messages/find_path/ResultRoutingTablesMessage.h"

class GetRoutingTablesTransaction : public BaseTransaction {

public:
    typedef shared_ptr<GetRoutingTablesTransaction> Shared;

public:

    GetRoutingTablesTransaction(
        NodeUUID &nodeUUID,
        RequestRoutingTablesMessage::Shared message,
        TrustLinesManager *manager,
        StorageHandler *storageHandler,
        Logger *logger);

    RequestRoutingTablesMessage::Shared message() const;

    TransactionResult::SharedConst run();

protected:

    const string logHeader() const;

private:

    RequestRoutingTablesMessage::Shared mMessage;
    TrustLinesManager *mTrustLinesManager;
    StorageHandler *mStorageHandler;

};


#endif //GEO_NETWORK_CLIENT_GETROUTINGTABLESTRANSACTION_H
