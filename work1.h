#ifndef WORK1_H
#define WORK1_H
#include "common/helper/ProcessHelper/processhelper.h"


#include <QFileInfo>
#include <QStringList>

struct Work1Params{
public:
    QString tmpfile;
    QString ofile;
    QString projname;
    QString workingpath;
};



class Work1
{
public:
    enum Result : int{
      OK=0,ISEMPTY,NOLASTREC,CANNOTUNMOUNT,NOUNITS,NOINPUTFILE,FILENOTEXIST,NOTCONFIRMED,COPYERROR,NOCHECK0,NOCHECK1,CHECKSUMERROR
    };
public:
    Work1();
    static int doWork();
    static Work1Params params;

    static QStringList GetUsbDrives();    
    static QString SelectUsbDrive(const QStringList& usbdrives);
    static int GetLastRecord(const QString &drive, int* units);
    static int dd(const QString &src, const QString &dst, int bs);
    static bool ConfirmYes();
    static QString GetFileName();
    static QStringList MountedParts(const QString &src);
    static bool UmountParts(const QStringList &src);    
    static QFileInfo MostRecent(const QString&);
    static QString BytesToString(double b);
    static int sha256sumDevice(const QString &fn, int r, qint64 b, const QString& sha_fn);
    static QString getSha(const QString &fn);

    static com::helper::ProcessHelper::Output Execute2(const QString& cmd);
    //static com::helper::ProcessHelper::Output Execute2Pipe(const QString &cmd1, const QString &cmd2);
};

#endif // WORK1_H
