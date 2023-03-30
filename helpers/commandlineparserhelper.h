#ifndef COMMANDLINEPARSERHELPER_H
#define COMMANDLINEPARSERHELPER_H

#include <QCommandLineParser>

class CommandLineParserHelper
{
public:
    CommandLineParserHelper();
    static bool addOption(QCommandLineParser *p, const QString& o, const QString& description);
    static bool addOptionBool(QCommandLineParser *p, const QString& o, const QString& description);
};

#endif // COMMANDLINEPARSERHELPER_H
