#include <QCoreApplication>
#include "helpers/logger.h"
#include "helpers/signalhelper.h"
#include "helpers/commandlineparserhelper.h"
#include "helpers/coreappworker.h"

#include "work1.h"
#include "typekey.h"
#include "helpers/stringify.h"

struct ParserKeyValueDesc{
    QString key;
    QString value;
    QString desc;
};

void ParserInit(QCommandLineParser *p, QCoreApplication *a, const QString& desc, const QList<ParserKeyValueDesc>& opts)
{
    p->setApplicationDescription(desc);
    p->addHelpOption();
    p->addVersionOption();

    //const QString OPTION_OUT = QStringLiteral("output");
    for(auto&i:opts) CommandLineParserHelper::addOption(p, i.key, i.desc);

    p->process(*a);
}

auto main(int argc, char *argv[]) -> int
{
#if defined (STRING) && defined (TARGI)
    auto target = STRING(TARGI);
#else
    auto target=QStringLiteral("ApplicationNameString");
#endif
    QCoreApplication a(argc, argv);
    QCoreApplication::setApplicationName(target);
    QCoreApplication::setApplicationVersion("1");
    QCoreApplication::setOrganizationName("LogControl Kft.");
    QCoreApplication::setOrganizationDomain("https://www.logcontrol.hu/");

    SignalHelper::setShutDownSignal(SignalHelper::SIGINT_); // shut down on ctrl-c
    SignalHelper::setShutDownSignal(SignalHelper::SIGTERM_); // shut down on killall
    Logger::Init(Logger::ErrLevel::INFO, Logger::DbgLevel::TRACE, true, true);

    QString user = qgetenv("USER");
    zInfo(QStringLiteral("started ")+target+" as "+user);

    QCommandLineParser parser;

    ParserInit(&parser, &a, QStringLiteral("writes a sd card"),
                   {
                       {
                           zkey(Work1Params::ofile),
                           QStringLiteral("output"),
                           QStringLiteral("file as output")
                       }
                   });
   const QString OPTION_PATH = QStringLiteral("path");

    CommandLineParserHelper::addOption(&parser, OPTION_PATH, QStringLiteral("output folder"));

    Work1::params.workingpath = parser.value(OPTION_PATH);

    CoreAppWorker c(Work1::doWork, &a, &parser);
    volatile auto errcode = c.run();

    switch(errcode)
    {
        case Work1::OK: zInfo("ok"); break;
        case Work1::ISEMPTY: zInfo("no block device to write"); break;
        case Work1::NOINPUTFILE: zInfo("no input file to read"); break;
        case Work1::NOLASTREC: zInfo("cannot find last record"); break;
        case Work1::NOUNITS: zInfo("unknown block size"); break;
        case Work1::FILENOTEXIST: zInfo("file not exist"); break;

        case Work1::CANNOTUNMOUNT: zInfo("cannot unmount device"); break;
        case Work1::NOCHECK0: zInfo("no checksum0"); break;
        case Work1::NOCHECK1: zInfo("no checksum1"); break;
        case Work1::CHECKSUMERROR: zInfo("checksum error"); break;
        case Work1::NO_PASSWD: zInfo("cannot sudo"); break;

        default: zInfo("an error occured: "+QString::number(errcode)); break;
    }

    auto e = QCoreApplication::exec();
    return e;
}
