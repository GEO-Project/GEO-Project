/**
 * This file is part of GEO Project.
 * It is subject to the license terms in the LICENSE.md file found in the top-level directory
 * of this distribution and at https://github.com/GEO-Project/GEO-Project/blob/master/LICENSE.md
 *
 * No part of GEO Project, including this file, may be copied, modified, propagated, or distributed
 * except according to the terms contained in the LICENSE.md file.
 */

#ifndef GEO_NETWORK_CLIENT_RESULTSINTERFACE_H
#define GEO_NETWORK_CLIENT_RESULTSINTERFACE_H

#include "../../BaseFIFOInterface.h"

#include "../../../logger/Logger.h"

#include "../../../common/exceptions/IOError.h"
#include "../../../common/exceptions/MemoryError.h"

#include <boost/bind.hpp>

#include <string>

#ifndef TESTS__TRUSTLINES
#include "../../../common/Types.h"
#include "../../../common/time/TimeUtils.h"
#endif

using namespace std;

class ResultsInterface: public BaseFIFOInterface {

public:
    explicit ResultsInterface(
        Logger &logger);

    ~ResultsInterface();

    void writeResult(
        const char *bytes,
        const size_t bytesCount);

private:
    virtual const char* FIFOName() const;

public:
    static const constexpr char *kFIFOName = "results.fifo";
    static const constexpr unsigned int kPermissionsMask = 0755;

private:
    Logger &mLog;
};

#endif //GEO_NETWORK_CLIENT_RESULTSINTERFACE_H
