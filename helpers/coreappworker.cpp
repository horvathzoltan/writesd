#include "helpers/coreappworker.h"
#include <QCoreApplication>
#include <QDebug>

CoreAppWorker::CoreAppWorker() = default;

int (*CoreAppWorker::workerFn)();
QCommandLineParser* CoreAppWorker::parser;

CoreAppWorker::CoreAppWorker(int (*fn)(), QCoreApplication *app, QCommandLineParser *p)
{
    workerFn = fn;
    parser = p;
    QObject::connect(this, &CoreAppWorker::finished, app, &QCoreApplication::quit, Qt::QueuedConnection);
    //qDebug() << "new AppWorker";
}

int CoreAppWorker::doWork()
{
    qDebug() << "Class1 doWork()";
    return 0;
}

int CoreAppWorker::run()
{
    int e = workerFn();
    emit finished();
    return e;
}
