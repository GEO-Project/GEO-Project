#ifndef GEO_NETWORK_CLIENT_ROUGHTINGTABLESBLUERAYEDITION_H
#define GEO_NETWORK_CLIENT_ROUGHTINGTABLESBLUERAYEDITION_H

#include "../common/NodeUUID.h"
#include "../logger/Logger.h"

#include <boost/signals2.hpp>
#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>

#include <map>
#include <set>

namespace as = boost::asio;
namespace signals = boost::signals2;


class RoutingTableManager {

public:
    RoutingTableManager(
        as::io_service &ioService,
        Logger &logger);

public:
    typedef signals::signal<void()> UpdateRoutingTableSignal;

    void updateMapAddOneNeighbor(
        const NodeUUID &firstLevelContractor,
        const NodeUUID &secondLevelContractor);

    void updateMapAddSeveralNeighbors(
        const NodeUUID &firstLevelContractor,
        set<NodeUUID> secondLevelContractors);

    void clearMap();

    set<NodeUUID> secondLevelContractorsForNode(const NodeUUID &contractorUUID);

    void runSignalUpdateTimer(const boost::system::error_code &err);

public:
    mutable UpdateRoutingTableSignal updateRoutingTableSignal;

protected:
    const uint32_t kUpdatingTimerPeriodSeconds = 60 * 60 * 24 * 3;

protected:
    as::io_service &mIOService;
    map<NodeUUID, set<NodeUUID>> mRoughtingTable;
    Logger &mLog;

    unique_ptr<as::steady_timer> mUpdatingTimer;
};

#endif //GEO_NETWORK_CLIENT_ROUGHTINGTABLESBLUERAYEDITION_H
