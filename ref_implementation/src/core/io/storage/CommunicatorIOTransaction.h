/**
 * This file is part of GEO Protocol.
 * It is subject to the license terms in the LICENSE.md file found in the top-level directory
 * of this distribution and at https://github.com/GEO-Protocol/GEO-network-client/blob/master/LICENSE.md
 *
 * No part of GEO Protocol, including this file, may be copied, modified, propagated, or distributed
 * except according to the terms contained in the LICENSE.md file.
 */

#ifndef GEO_NETWORK_CLIENT_COMMUNICATORIOTRANSACTION_H
#define GEO_NETWORK_CLIENT_COMMUNICATORIOTRANSACTION_H

#include "../../common/Types.h"
#include "CommunicatorMessagesQueueHandler.h"

#include "../../../libs/sqlite3/sqlite3.h"

class CommunicatorIOTransaction {

public:
    typedef shared_ptr<CommunicatorIOTransaction> Shared;
    typedef unique_ptr<CommunicatorIOTransaction> Unique;

public:
    CommunicatorIOTransaction(
        sqlite3 *dbConnection,
        CommunicatorMessagesQueueHandler *communicatorMessagesQueueHandler,
        Logger &logger);

    ~CommunicatorIOTransaction();

    CommunicatorMessagesQueueHandler *communicatorMessagesQueueHandler();


    void rollback();

private:
    void commit();

    void beginTransactionQuery();

    LoggerStream info() const;

    LoggerStream warning() const;

    const string logHeader() const;

private:
    sqlite3 *mDBConnection;
    CommunicatorMessagesQueueHandler *mCommunicatorMessagesQueueHandler;
    bool mIsTransactionBegin;
    Logger &mLog;
};


#endif //GEO_NETWORK_CLIENT_COMMUNICATORIOTRANSACTION_H
