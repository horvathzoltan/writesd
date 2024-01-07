#ifndef WORK1_H
#define WORK1_H

#include <QFileInfo>
#include <QStringList>

struct Work1Params{
public:
    //QString tmpfile;
    QString ofile;
    //QString projname;
    QString workingpath;
    QString passwd;
};

struct UsbDriveModel{
    QString devicePath;
    QString usbPath;
    QStringList partLabels;
    long size;

    QString toString() const;
    bool isValid();
    QString GetLastUsbTag() const;
};

struct RecModel{
    int units;
    int lastrec;
};

class Work1
{
public:
    enum Result : int{
        OK=0,
        ISEMPTY,
        NO_LASTREC,
        CANNOT_UNMOUNT,
        NO_UNITS,
        NO_INPUTFILE,
        FILENOTEXIST,
        NOT_CONFIRMED,
        COPY_ERROR,
        NO_CHECK0,
        NO_CHECK1,
        CHECKSUM_ERROR,
        NO_PASSWD,
        NO_USBDRIVE,
        DRIVE_SIZE_ERROR
    };
public:
    Work1();
    static int doWork();
    static Work1Params params;

    static QList<UsbDriveModel> GetUsbDrives();
    static UsbDriveModel SelectUsbDrive(const QList<UsbDriveModel>& usbdrives);
    static QString GetUsbPath(const QString& dev);
    static QStringList GetPartLabels(const QString& dev);

    static int GetLastRecord(const QString &drive, int* units);    
    static QList<RecModel> GetLastRecords(const QList<UsbDriveModel> &usbdrives);
    static bool CheckRecords_Units(const QList<RecModel>& records);

    static QList<UsbDriveModel> GetSmallUsbDrive(const QList<UsbDriveModel>& usbdrives, long size);
    static QStringList GetDevPaths(const QList<UsbDriveModel>& usbdrives);

    static int dd(const QString& src, const QString& dst, int bs, QString *mnt);
    static int dd2(const QString& src, const QStringList& dst, int bs, QString *mnt);
    static bool ConfirmYes();
    static QString GetFileName(const QString& msg);
    static QStringList MountedParts(const QString &src);
    static bool UmountParts(const QStringList &src);    
    static QFileInfo MostRecent(const QString&);
    static QString BytesToString(double b);
    static int sha256sumDevice(const QString &fn, int r, qint64 b, const QString& sha_fn);
    static QString getSha(const QString &fn);

    //static ProcessHelper::Output Execute2(const QString& cmd);
    //static com::helper::ProcessHelper::Output Execute2Pipe(const QString &cmd1, const QString &cmd2);
private:
    static QList<UsbDriveModel> SelectUsbDrives(const QList<UsbDriveModel>& drives);
};

#endif // WORK1_H
