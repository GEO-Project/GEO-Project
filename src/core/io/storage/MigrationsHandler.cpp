#include "MigrationsHandler.h"

MigrationsHandler::MigrationsHandler(
    sqlite3 *dbConnection,
    const string &tableName,
    Logger &logger):

    mLog(logger),
    mDataBase(dbConnection),
    mTableName(tableName)
{
    enshureMigrationsTable();
}

/**
 * Creates table for storing apllied migrations, and unique index for thier UUIDs.
 * In case if this table is already present - does nothing.
 *
 * @throws IOError in case of error.
 */
void MigrationsHandler::enshureMigrationsTable()
{
    sqlite3_stmt *stmt;

    /*
     * Main table creation.
     */
    string query = "CREATE TABLE IF NOT EXISTS " + mTableName + " (migration_uuid BLOB NOT NULL);";

    int rc = sqlite3_prepare_v2(mDataBase, query.c_str(), -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        throw IOError(
            "MigrationsHandler::enshureMigrationsTable: "
            "Can't create migrations table: sqlite error code: " + to_string(rc) + ".");
    }

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        throw IOError(
            "MigrationsHandler::enshureMigrationsTable: "
            "Can't create migrations table: sqlite error code: " + to_string(rc) + ".");
    }


    /*
     * Creating unique index for preventing duplicating of migrations UUIDs.
     */
    query = "CREATE UNIQUE INDEX IF NOT EXISTS " + mTableName + "_migration_uuid_index on " + mTableName + "(migration_uuid);";
    rc = sqlite3_prepare_v2(mDataBase, query.c_str(), -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        throw IOError(
            "MigrationsHandler::enshureMigrationsTable: "
            "Can't create index for migrations table: sqlite error code: " + to_string(rc) + ".");
    }
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        throw IOError(
            "MigrationsHandler::enshureMigrationsTable: "
            "Can't create index for migrations table: sqlite error code: " + to_string(rc) + ".");
    }


    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);
}

/**
 * @returns vector with UUIDs of all applied migrations.
 * Returns empty vector in case if no migration was applied in the past.
 */
vector<MigrationUUID> MigrationsHandler::allMigrationsUUIDS() {
    vector<MigrationUUID> migrationsUUIDs;
    string query = "SELECT migration_uuid FROM " + mTableName + ";";

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(mDataBase, query.c_str(), -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        throw IOError(
            "MigrationsHandler::allMigrationUUIDS:"
            "Can't select applied migrations list.  "
            "sqlite error code: " + to_string(rc));
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        MigrationUUID migrationUUID(
            static_cast<const uint8_t *>(
                sqlite3_column_blob(stmt, 0)));

        migrationsUUIDs.push_back(migrationUUID);
    }

    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);

    return migrationsUUIDs;
}

/**
 * Attempts to apply all migrations, that are listed in the method body.
 * The order, in which migrations would be applied, corresponds to the order, in which they are placed in the code.
 * In case, if some migration was already applied in the past - it would be skipped.
 *
 * @param ioTransaction - IO Transaction in scope of which the migrations must be applied.
 *
 * @throws RuntimeError in case of internal migration error.
 * The exception message would describe the error more detailed.
 */
void MigrationsHandler::applyMigrations(
    IOTransaction::Shared ioTransaction)
{
    auto fullMigrationsUUIDsList = {
        MigrationUUID("0a889a5b-1a82-44c7-8b85-59db6f60a12d"),

        // ...
        // the rest migrations must be placed here.
    };


    try {
        auto migrationsUUIDS = allMigrationsUUIDS();

        for (auto const &kMigrationUUID : fullMigrationsUUIDsList) {
            if (std::find(migrationsUUIDS.begin(), migrationsUUIDS.end(), kMigrationUUID) != migrationsUUIDS.end()) {
                info() << "Migration " << kMigrationUUID << " is already applied. Skipped.";
                continue;

            }

            try {
                applyMigration(kMigrationUUID, ioTransaction);
                info() << "Migration " << kMigrationUUID << " successfully applied.";

            } catch (Exception &e) {
                error() << "Migration " << kMigrationUUID << " can't be applied. Details: " << e.message();
                throw RuntimeError(e.message());
            }
        }

    } catch (const Exception &e) {
        // allMigrationsUUIDS() might throw IOError.

        mLog.logException("MigrationsHandler", e);
        throw RuntimeError(e.message());
    }
}

/**
 * Tries to apply migration with received UUID.
 *
 * @param migrationUUID - UUID of the migration that must be applied.
 * @param ioTransaction - IO transactions in scope of which the operation must be performed.
 *
 * @throws ValueError - in case if received UUID is not present in expected UUIDs list.
 * @throws RuntimeError - in case of migration error.
 */
void MigrationsHandler::applyMigration(
    MigrationUUID migrationUUID,
    IOTransaction::Shared ioTransaction)
{

    try {

        if (migrationUUID.stringUUID() == string("0a889a5b-1a82-44c7-8b85-59db6f60a12d")){
            auto migration = make_shared<AmountAndBalanceSerializationMigration>(
                mDataBase,
                mLog);

            migration->apply(ioTransaction);
            saveMigration(migrationUUID);

        // ...
        // Other migrations must be placed here
        //

        } else {
            throw ValueError(
                "MigrationsHandler::applyMigration: "
                "can't recognize received migration UUID.");
        }

    } catch (std::exception &e) {
        throw RuntimeError(
            "MigrationsHandler::applyMigration: "
            "Migration can't be applied. Details are: " + string(e.what()));
    }
}

void MigrationsHandler::saveMigration(
    MigrationUUID migrationUUID)
{
    string query = "INSERT INTO " + mTableName +
                        "(migration_uuid) VALUES (?);";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2( mDataBase, query.c_str(), -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        throw IOError("MigrationsHandler::insert : "
                              "Bad query; sqlite error: " + to_string(rc));
    }
    rc = sqlite3_bind_blob(stmt, 1, migrationUUID.data, NodeUUID::kBytesSize, SQLITE_STATIC);
    if (rc != SQLITE_OK) {
        throw IOError("MigrationsHandler::insert : "
                              "Bad binding of Contractor; sqlite error: "+ to_string(rc));
    }
    rc = sqlite3_step(stmt);
    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) {
        throw IOError("MigrationsHandler::insert : "
                              "Run query; sqlite error: "+ to_string(rc));
    }
}

const string MigrationsHandler::logHeader() const
{
    stringstream s;
    s << "[MigrationsHandler]";
    return s.str();
}

LoggerStream MigrationsHandler::info() const
{
    return mLog.info(logHeader());
}

LoggerStream MigrationsHandler::debug() const
{
    return mLog.debug(logHeader());
}

LoggerStream MigrationsHandler::error() const
{
    return mLog.error(logHeader());
}


