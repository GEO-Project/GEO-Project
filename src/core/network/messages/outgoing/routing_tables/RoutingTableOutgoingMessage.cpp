﻿#include "RoutingTableOutgoingMessage.h"

RoutingTableOutgoingMessage::RoutingTableOutgoingMessage(
    NodeUUID &senderUUID,
    TrustLineUUID &trustLineUUID) :

    RoutingTablesMessage(
        senderUUID,
        trustLineUUID
    ) {}

void RoutingTableOutgoingMessage::pushBack(
    const NodeUUID &node,
    vector<pair<NodeUUID, TrustLineDirection>> &table) {

    mRecords.insert(
        make_pair(
            node,
            table
        )
    );
}

pair<BytesShared, size_t> RoutingTableOutgoingMessage::serializeToBytes() {

    auto parentBytesAndCount = RoutingTablesMessage::serializeToBytes();
    size_t bytesCount = parentBytesAndCount.second;

    bytesCount += sizeof(RecordsCount);
    for (const auto &nodeAndRecord : mRecords) {
        bytesCount += NodeUUID::kBytesSize + sizeof(RecordsCount);
        for(const auto &neighborAndDirect : nodeAndRecord.second) {
            bytesCount += NodeUUID::kBytesSize + sizeof(TrustLineDirection);
        }
    }

    BytesShared dataBytesShared = tryCalloc(bytesCount);
    size_t dataBytesOffset = 0;
    //----------------------------------------------------
    memcpy(
        dataBytesShared.get(),
        parentBytesAndCount.first.get(),
        parentBytesAndCount.second
    );
    dataBytesOffset += parentBytesAndCount.second;
    //----------------------------------------------------
    RecordsCount nodesCount = mRecords.size();
    memcpy(
        dataBytesShared.get() + dataBytesOffset,
        &nodesCount,
        sizeof(RecordsCount)
    );
    dataBytesOffset += sizeof(RecordsCount);
    //----------------------------------------------------
    for (const auto &nodeAndRecord : mRecords) {
        memcpy(
            dataBytesShared.get() + dataBytesOffset,
            nodeAndRecord.first.data,
            NodeUUID::kBytesSize
        );
        dataBytesOffset += NodeUUID::kBytesSize;
        //----------------------------------------------------
        RecordsCount recordsCount = nodeAndRecord.second.size();
        memcpy(
            dataBytesShared.get() + dataBytesOffset,
            &recordsCount,
            sizeof(RecordsCount)
        );
        dataBytesOffset += sizeof(RecordsCount);
        //----------------------------------------------------
        for(const auto &neighborAndDirect : nodeAndRecord.second) {
            memcpy(
                dataBytesShared.get() + dataBytesOffset,
                neighborAndDirect.first.data,
                NodeUUID::kBytesSize
            );
            dataBytesOffset += NodeUUID::kBytesSize;
            //----------------------------------------------------
            SerializedTrustLineDirection direction = neighborAndDirect.second;
            memcpy(
                dataBytesShared.get() + dataBytesOffset,
                &direction,
                sizeof(SerializedTrustLineDirection)
            );
            dataBytesOffset += sizeof(SerializedTrustLineDirection);
        }
        //----------------------------------------------------
    }
    //----------------------------------------------------
    return make_pair(
        dataBytesShared,
        bytesCount
    );
}

void RoutingTableOutgoingMessage::deserializeFromBytes(
    BytesShared buffer) {

    throw Exception("RoutingTableOutgoingMessage::deserializeFromBytes: "
                                  "Method not implemented.");
}
