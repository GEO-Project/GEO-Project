#include "FindPathTransaction.h"

FindPathTransaction::FindPathTransaction(
    NodeUUID &nodeUUID,
    FindPathCommand::Shared command,
    PathsManager *pathsManager,
    Logger *logger) :

    BaseTransaction(
        BaseTransaction::TransactionType::FindPathTransactionType,
        nodeUUID,
        logger),

    mCommand(command),
    mPathManager(pathsManager),
    mRequestCounter(0) {}

FindPathCommand::Shared FindPathTransaction::command() const {

    return mCommand;
}

TransactionResult::SharedConst FindPathTransaction::run() {

    info() << "run\t" << UUID();

    if (!mContext.empty()) {
        return checkTransactionContext();

    } else {
        if (mRequestCounter < kMaxRequestsCount) {
            sendMessageToRemoteNode();
            increaseRequestsCounter();

        } else {
            return noResponseResult();
        }

    }
    return waitingForResponseState();

}

TransactionResult::SharedConst FindPathTransaction::checkTransactionContext() {

    info() << "context size\t" << mContext.size();
    if (mContext.size() == 1) {
        auto responseMessage = *mContext.begin();

        if (responseMessage->typeID() == Message::MessageTypeID::ResultRoutingTablesMessageType) {
            ResultRoutingTablesMessage::Shared response = static_pointer_cast<ResultRoutingTablesMessage>(
                responseMessage);

            vector<pair<const NodeUUID, const TrustLineDirection >> rt1 = response->rt1();
            info() << "receive RT1 size: " << rt1.size();
            for (auto &nodeUUIDAndDirection : rt1) {
                info() << "\t" << nodeUUIDAndDirection.first << "\t" << nodeUUIDAndDirection.second;
            }
            info() << "receive RT2 size: " << response->rt2().size();
            info() << "receive RT3 size: " << response->rt3().size();

            vector<NodeUUID> path;
            path.push_back(mNodeUUID);
            path.push_back(mCommand->contractorUUID());
            return resultOk(
                path);
        }

        return unexpectedErrorResult();

    } else {
        throw ConflictError("TotalBalancesFromRemoutNodeTransaction::checkTransactionContext: "
                                    "Unexpected context size.");
    }
}

void FindPathTransaction::sendMessageToRemoteNode() {

    info() << "sendMessageToRemoteNode\t" << mCommand->contractorUUID();
    sendMessage<RequestRoutingTablesMessage>(
        mCommand->contractorUUID(),
        mNodeUUID,
        UUID());
}

TransactionResult::SharedConst FindPathTransaction::waitingForResponseState() {

    info() << "waitingForResponseState";
    TransactionState *transactionState = new TransactionState(
        microsecondsSinceGEOEpoch(
            utc_now() + pt::microseconds(kConnectionTimeout * 1000)),
            Message::MessageTypeID::ResultRoutingTablesMessageType,
            false);

    return transactionResultFromState(
        TransactionState::SharedConst(
            transactionState));
}

void FindPathTransaction::increaseRequestsCounter() {

    mRequestCounter += 1;
    info() << "increaseRequestsCounter\t" << mRequestCounter;
}

TransactionResult::SharedConst FindPathTransaction::resultOk(
        vector<NodeUUID> path) {

    stringstream s;
    for (auto &nodeUUID : path) {
        s << nodeUUID << "\t";
    }
    string pathResult = s.str();
    info() << "resultOk\t" << pathResult;
    return transactionResultFromCommand(mCommand->resultOk(pathResult));
}

TransactionResult::SharedConst FindPathTransaction::noResponseResult() {

    info() << "noResponseResult";
    return transactionResultFromCommand(
            mCommand->resultNoResponse());
}

TransactionResult::SharedConst FindPathTransaction::unexpectedErrorResult() {

    return transactionResultFromCommand(
            mCommand->unexpectedErrorResult());
}

const string FindPathTransaction::logHeader() const
{
    stringstream s;
    s << "[FindPathTA]";

    return s.str();
}
