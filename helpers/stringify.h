#ifndef STRINGIFY_H
#define STRINGIFY_H

#define STRINGIFY(msg) #msg
#define STRING(msg) QStringLiteral(STRINGIFY(msg))
#define L(msg) QStringLiteral(msg)

#endif // STRINGIFY_H
