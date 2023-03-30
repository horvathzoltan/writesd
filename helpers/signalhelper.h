#ifndef COM_SIGNALHELPER_H
#define COM_SIGNALHELPER_H

#include <csignal>

class SignalHelper
{
public:
    static void setShutDownSignal(int);
    static constexpr int SIGINT_ = SIGINT;
    static constexpr int SIGTERM_  =SIGTERM;
private:
    static void handleShutDownSignal(int);
};


#endif // SIGNALHELPER_H
