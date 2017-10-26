#ifndef GEO_NETWORK_CLIENT_NODEFEATURESHANDLER_H
#define GEO_NETWORK_CLIENT_NODEFEATURESHANDLER_H

#include "../../logger/Logger.h"
#include "../../transactions/transactions/base/TransactionUUID.h"
#include "../../common/Types.h"
#include "../../common/memory/MemoryUtils.h"
#include "../../common/exceptions/IOError.h"
#include "../../common/exceptions/NotFoundError.h"

#include "../../../libs/sqlite3/sqlite3.h"

class NodeFeaturesHandler {

public:
    NodeFeaturesHandler(
        sqlite3 *dbConnection,
        const string &tableName,
        Logger &logger);

    void saveRecord(
        const string &featureName,
        const string &featureValue = "");

    string featureValue(
        const string &featureName);

    void deleteRecord(
        const string &featureName);

private:
    LoggerStream info() const;

    LoggerStream warning() const;

    const string logHeader() const;

private:
    sqlite3 *mDataBase = nullptr;
    string mTableName;
    Logger &mLog;
};


#endif //GEO_NETWORK_CLIENT_NODEFEATURESHANDLER_H
