#include "signalhelper.h"

#include <QString>
#include <QCoreApplication>
#include "helpers/logger.h"

void SignalHelper::handleShutDownSignal( int signalId )
{
    zInfo(QStringLiteral("EXIT: %1").arg(signalId));
    QCoreApplication::exit(0);
}

#ifdef __linux__
void SignalHelper::setShutDownSignal( int signalId )
{
    struct sigaction sa;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = handleShutDownSignal;
    if (sigaction(signalId, &sa, nullptr) == -1)
    {
        perror("setting up termination signal");
        exit(1);
    }
}
#endif

#ifdef _WIN32

void SignalHelper::setShutDownSignal( int signalId )
{
    signal(signalId, handleShutDownSignal);
}

#endif
