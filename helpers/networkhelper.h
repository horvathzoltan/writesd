#ifndef NETWORKHELPER_H
#define NETWORKHELPER_H

#include <QString>

class NetworkHelper
{
public:
    NetworkHelper(){};
    static bool Ping(const QString &ip);
};

#endif // NETWORKHELPER_H
