/**
 * This file is part of GEO Protocol.
 * It is subject to the license terms in the LICENSE.md file found in the top-level directory
 * of this distribution and at https://github.com/GEO-Protocol/GEO-network-client/blob/master/LICENSE.md
 *
 * No part of GEO Protocol, including this file, may be copied, modified, propagated, or distributed
 * except according to the terms contained in the LICENSE.md file.
 */

#ifndef GEO_NETWORK_CLIENT_GETBLACKLISTTRANSACTION_H
#define GEO_NETWORK_CLIENT_GETBLACKLISTTRANSACTION_H


#include "../base/BaseTransaction.h"
#include "../../../interface/commands_interface/commands/blacklist/GetBlackListCommand.h"
#include "../../../io/storage/StorageHandler.h"


class GetBlackListTransaction :
    public BaseTransaction {

public:
    typedef shared_ptr<GetBlackListTransaction> Shared;

public:
    GetBlackListTransaction(
        NodeUUID &nodeUUID,
        GetBlackListCommand::Shared command,
        StorageHandler *storageHandler,
        Logger &logger)
    noexcept;

    GetBlackListCommand::Shared command() const;

    TransactionResult::SharedConst run();

protected:
    const string logHeader() const;

private:
    GetBlackListCommand::Shared mCommand;
    StorageHandler *mStorageHandler;
};


#endif //GEO_NETWORK_CLIENT_GETBLACKLISTTRANSACTION_H
