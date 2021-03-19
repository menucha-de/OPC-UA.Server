#include "OpcUaServerCore.h"
#include <common/logging/Logger.h>
#include <common/logging/LoggerFactory.h>
#include <statuscode.h> // UaStatus
#include <uacoreserverapplication.h> // UaServer
#include <ualocalizedtext.h> // UaLocalizedText
#include <uamodule.h> // UaModule
#include <uasession.h> // UaSession

#ifndef UA_BUILD_DATE_ZONE
#define UA_BUILD_DATE_ZONE 1 // Must match UTC offset and daylight saving time at build date
#endif

using namespace CommonNamespace;

class OpcUaServerCorePrivate {
    friend class OpcUaServerCore;
private:
    Logger* log;
    UaModule* uaModule;
};

/** Construction. */
OpcUaServerCore::OpcUaServerCore() {
    d = new OpcUaServerCorePrivate;
    d->log = LoggerFactory::getLogger("OpcUaServerCore");
    d->uaModule = NULL;
}

/** Destruction. */
OpcUaServerCore::~OpcUaServerCore() {
    // if the server runs
    if (isStarted()) {
        // stop the server
        UaLocalizedText reason("en", "Application shut down");
        stop(0, reason);
    }
    // delete the UA server module
    delete d->uaModule;
    delete d;
}

UaStatus OpcUaServerCore::afterInitialize() {
    // create UA server module
    d->uaModule = new UaModule();
    // check if the application provides an own UaServer implementation
    UaServer *pUaServer = NULL;
    UaServerApplicationCallback* pCallback = getServerApplicationCallback();
    if (pCallback) {
        pUaServer = pCallback->createUaServer();
    }
    // initialize UA server module
    if (0 != d->uaModule->initialize(getServerConfig(), pUaServer)) {
        return UaStatus(OpcUa_BadInternalError);
    }
    return UaStatus(OpcUa_Good);
}

UaStatus OpcUaServerCore::afterStartUp() {
    UaStatus ret;
    // start UA server module
    if (0 != d->uaModule->startUp(getCoreModule())) {
        d->uaModule->shutDown();
        delete d->uaModule;
        d->uaModule = NULL;
        return UaStatus(OpcUa_BadInternalError);
    }
    // log opened endpoints
    UaString sRejectedCertificateDirectory;
    OpcUa_UInt32 nRejectedCertificatesCount;
    UaEndpointArray uaEndpointArray;
    getServerConfig()->getEndpointConfiguration(sRejectedCertificateDirectory,
            nRejectedCertificatesCount, uaEndpointArray);
    if (uaEndpointArray.length() > 0) {
        for (OpcUa_UInt32 i = 0; i < uaEndpointArray.length(); i++) {
            if (uaEndpointArray[i]->isOpened()) {
                d->log->info("Opened endpoint for %s",
                        uaEndpointArray[i]->sEndpointUrl().toUtf8());
            } else {
                d->log->error("Cannot open endpoint for %s",
                        uaEndpointArray[i]->sEndpointUrl().toUtf8());
            }
        }
    }
    return ret;
}

UaStatus OpcUaServerCore::beforeShutdown() {
    UaStatus ret;
    // stop UA server module
    if (d->uaModule) {
        d->uaModule->shutDown();
        delete d->uaModule;
        d->uaModule = NULL;
    }
    return ret;
}

Session* OpcUaServerCore::createDefaultSession(OpcUa_Int32 sessionID, const UaNodeId &authenticationToken) {
    return new UaSession(sessionID, authenticationToken);
}

/** Get the build date from the static compiled in string.
 *  @return the build date from the static compiled in string.
 */
OpcUa_DateTime OpcUaServerCore::getBuildDate() const {
    static OpcUa_DateTime date;
    static const char szDate[] = __DATE__; /* "Mon DD YYYY" */
    static char szISO[] = "YYYY-MM-DDT"__TIME__"Z";
    static const char* Months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    char mon = 0;

    /* set year */
    szISO[0] = szDate[7];
    szISO[1] = szDate[8];
    szISO[2] = szDate[9];
    szISO[3] = szDate[10];

    /* set month */
    while ((strncmp(Months[(int) mon], szDate, 3) != 0) && (mon < 11)) {
        mon++;
    }
    mon++;
    szISO[5] = '0' + mon / 10;
    szISO[6] = '0' + mon % 10;

    /* set day */
    szISO[8] = szDate[4];
    szISO[9] = szDate[5];

    /* convert to UA time */
    OpcUa_DateTime_GetDateTimeFromString(szISO, &date);

    /* correct time */
    UaDateTime buildDate(date);
    buildDate.addSecs(UA_BUILD_DATE_ZONE * 3600 * -1);

    return buildDate;
}
