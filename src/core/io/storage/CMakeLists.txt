cmake_minimum_required(VERSION 3.6)

set(SOURCE_FILES
        record/base/Record.h
        record/base/Record.cpp
        record/trust_line/TrustLineRecord.h
        record/trust_line/TrustLineRecord.cpp
        record/payment/PaymentRecord.h
        record/payment/PaymentRecord.cpp
        record/payment/PaymentAdditionalRecord.h
        record/payment/PaymentAdditionalRecord.cpp
        record/audit/AuditRecord.h
        record/audit/AuditRecord.cpp
        record/audit/ReceiptRecord.h
        record/audit/ReceiptRecord.cpp

        StorageHandler.h
        StorageHandler.cpp
        IOTransaction.cpp
        IOTransaction.h

        TrustLineHandler.h
        TrustLineHandler.cpp
        TransactionsHandler.h
        TransactionsHandler.cpp
        HistoryStorage.h
        HistoryStorage.cpp

        CommunicatorStorageHandler.h
        CommunicatorStorageHandler.cpp
        CommunicatorIOTransaction.h
        CommunicatorIOTransaction.cpp

        CommunicatorMessagesQueueHandler.h
        CommunicatorMessagesQueueHandler.cpp

        OwnKeysHandler.h
        OwnKeysHandler.cpp
        ContractorKeysHandler.h
        ContractorKeysHandler.cpp
        AuditHandler.h
        AuditHandler.cpp
        PaymentKeysHandler.h
        PaymentKeysHandler.cpp
        IncomingPaymentReceiptHandler.h
        IncomingPaymentReceiptHandler.cpp
        OutgoingPaymentReceiptHandler.h
        OutgoingPaymentReceiptHandler.cpp
        PaymentTransactionsHandler.h
        PaymentTransactionsHandler.cpp
        PaymentParticipantsVotesHandler.h
        PaymentParticipantsVotesHandler.cpp
        AuditRulesHandler.h
        AuditRulesHandler.cpp

        ContractorsHandler.h
        ContractorsHandler.cpp
        AddressHandler.h
        AddressHandler.cpp

        FeaturesHandler.h
        FeaturesHandler.cpp)

add_library(io__storage ${SOURCE_FILES})

target_link_libraries(io__storage
        libsqlite3
        contractors)
