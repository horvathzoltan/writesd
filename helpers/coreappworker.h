#ifndef COM_COREAPPWORKER_H
#define COM_COREAPPWORKER_H

#include <QObject>
#include <QCommandLineParser>


class CoreAppWorker : public QObject
{
    Q_OBJECT
public:
    CoreAppWorker();
    CoreAppWorker(int (*fn)(), QCoreApplication *app, QCommandLineParser *parser);
    int doWork();
public slots:
    int run();
signals:
    void finished();
private:
    static int (*workerFn)();
    static QCommandLineParser* parser;
};


#endif // COREAPPWORKER_H
