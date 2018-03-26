/**
 * This file is part of GEO Project.
 * It is subject to the license terms in the LICENSE.md file found in the top-level directory
 * of this distribution and at https://github.com/GEO-Project/GEO-Project/blob/master/LICENSE.md
 *
 * No part of GEO Project, including this file, may be copied, modified, propagated, or distributed
 * except according to the terms contained in the LICENSE.md file.
 */

#include "TransactionsHandler.h"

TransactionsHandler::TransactionsHandler(
    sqlite3 *dbConnection,
    const string &tableName,
    Logger &logger):

    mDataBase(dbConnection),
    mTableName(tableName),
    mLog(logger)
{
    string query = "CREATE TABLE IF NOT EXISTS " + mTableName +
                   " (transaction_uuid BLOB NOT NULL, "
                       "transaction_body BLOB NOT NULL, "
                       "transaction_bytes_count INT NOT NULL);";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(mDataBase, query.c_str(), -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        throw IOError("TransactionsHandler::creating table: "
                          "Bad query; sqlite error: " + to_string(rc));
    }
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_DONE) {
    } else {
        throw IOError("TransactionsHandler::creating table: Run query");
    }
    query = "CREATE UNIQUE INDEX IF NOT EXISTS " + mTableName
            + "_transaction_uuid_idx on " + mTableName + " (transaction_uuid);";
    rc = sqlite3_prepare_v2(mDataBase, query.c_str(), -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        throw IOError("TransactionsHandler::creating index for TransactionUUID: "
                          "Bad query; sqlite error: " + to_string(rc));
    }
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_DONE) {
    } else {
        throw IOError("TransactionsHandler::creating index for TransactionUUID: "
                          "Run query; sqlite error: " + to_string(rc));
    }
    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);
}

void TransactionsHandler::saveRecord(
    const TransactionUUID &transactionUUID,
    BytesShared transaction,
    size_t transactionBytesCount)
{
    string query = "INSERT OR REPLACE INTO " + mTableName +
                   " (transaction_uuid, transaction_body, transaction_bytes_count) VALUES(?, ?, ?);";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(mDataBase, query.c_str(), -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        throw IOError("TransactionsHandler::insert or replace: "
                          "Bad query; sqlite error: " + to_string(rc));
    }
    rc = sqlite3_bind_blob(stmt, 1, transactionUUID.data, NodeUUID::kBytesSize, SQLITE_STATIC);
    if (rc != SQLITE_OK) {
        throw IOError("TransactionsHandler::insert or replace: "
                          "Bad binding of TransactionUUID; sqlite error: " + to_string(rc));
    }
    rc = sqlite3_bind_blob(stmt, 2, transaction.get(), (int)transactionBytesCount, SQLITE_STATIC);
    if (rc != SQLITE_OK) {
        throw IOError("TransactionsHandler::insert or replace: "
                          "Bad binding of Transaction body; sqlite error: " + to_string(rc));
    }
    rc = sqlite3_bind_int(stmt, 3, (int)transactionBytesCount);
    if (rc != SQLITE_OK) {
        throw IOError("TransactionsHandler::insert or replace: "
                          "Bad binding of Transaction bytes count; sqlite error: " + to_string(rc));
    }
    rc = sqlite3_step(stmt);
    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);
    if (rc == SQLITE_DONE) {
#ifdef STORAGE_HANDLER_DEBUG_LOG
        info() << "prepare inserting or replacing is completed successfully";
#endif
    } else {
        throw IOError("TransactionsHandler::insert or replace: "
                          "Run query; sqlite error: " + to_string(rc));
    }
}

void TransactionsHandler::deleteRecord(
    const TransactionUUID &transactionUUID)
{
    string query = "DELETE FROM " + mTableName + " WHERE transaction_uuid = ?;";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(mDataBase, query.c_str(), -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        throw IOError("TransactionsHandler::delete: "
                          "Bad query; sqlite error: " + to_string(rc));
    }
    rc = sqlite3_bind_blob(stmt, 1, transactionUUID.data, NodeUUID::kBytesSize, SQLITE_STATIC);
    if (rc != SQLITE_OK) {
        throw IOError("TransactionsHandler::delete: "
                          "Bad binding of TransactionUUID; sqlite error: " + to_string(rc));
    }
    rc = sqlite3_step(stmt);
    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);
    if (rc == SQLITE_DONE) {
#ifdef STORAGE_HANDLER_DEBUG_LOG
        info() << "prepare deleting is completed successfully";
#endif
    } else {
        throw IOError("PaymentOperationStateHandler::delete: "
                          "Run query; sqlite error: " + to_string(rc));
    }
}

pair<BytesShared, size_t> TransactionsHandler::getTransaction(
    const TransactionUUID &transactionUUID)
{
    string query = "SELECT transaction_body, transaction_bytes_count FROM "
                   + mTableName + " WHERE transaction_uuid = ?;";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(mDataBase, query.c_str(), -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        throw IOError("TransactionsHandler::getTransaction: "
                          "Bad query; sqlite error: " + to_string(rc));
    }
    rc = sqlite3_bind_blob(stmt, 1, transactionUUID.data, NodeUUID::kBytesSize, SQLITE_STATIC);
    if (rc != SQLITE_OK) {
        throw IOError("TransactionsHandler::getTransaction: "
                          "Bad binding of TransactionUUID; sqlite error: " + to_string(rc));
    }
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        size_t stateBytesCount = (size_t)sqlite3_column_int(stmt, 1);
        BytesShared state = tryMalloc(stateBytesCount);
        memcpy(
            state.get(),
            sqlite3_column_blob(stmt, 0),
            stateBytesCount);
        sqlite3_reset(stmt);
        sqlite3_finalize(stmt);
        return make_pair(state, stateBytesCount);
    } else {
        sqlite3_reset(stmt);
        sqlite3_finalize(stmt);
        throw NotFoundError("TransactionsHandler::getTransaction: "
                                "There are now records with requested transactionUUID");
    }
}

vector<pair<BytesShared, size_t>> TransactionsHandler::allTransactions()
{
    string queryCount = "SELECT count(*) FROM " + mTableName;
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2( mDataBase, queryCount.c_str(), -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        throw IOError("TrustLineHandler::allTransactions: "
                          "Bad count query; sqlite error: " + to_string(rc));
    }
    sqlite3_step(stmt);
    uint32_t rowCount = (uint32_t)sqlite3_column_int(stmt, 0);
    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);
    vector<pair<BytesShared, size_t>> result;
    result.reserve(rowCount);
    string query = "SELECT transaction_body, transaction_bytes_count FROM "
                   + mTableName + ";";
    rc = sqlite3_prepare_v2(mDataBase, query.c_str(), -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        throw IOError("TransactionsHandler::allTransactions: "
                          "Bad query; sqlite error: " + to_string(rc));
    }
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        size_t stateBytesCount = (size_t) sqlite3_column_int(stmt, 1);
        BytesShared state = tryMalloc(stateBytesCount);
        memcpy(
            state.get(),
            sqlite3_column_blob(stmt, 0),
            stateBytesCount);

        result.push_back(
            make_pair(
                state,
                stateBytesCount));
    }
    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);
    return result;
}

LoggerStream TransactionsHandler::info() const
{
    return mLog.info(logHeader());
}

LoggerStream TransactionsHandler::warning() const
{
    return mLog.warning(logHeader());
}

const string TransactionsHandler::logHeader() const
{
    stringstream s;
    s << "[TransactionsHandler]";
    return s.str();
}

const string TransactionsHandler::tableName() const {
    return mTableName;
}
