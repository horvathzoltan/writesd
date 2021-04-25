#include "work1.h"
#include "common/logger/log.h"
#include "common/helper/textfilehelper/textfilehelper.h"
#include "common/helper/ProcessHelper/processhelper.h"
//#include "sqlhelper.h"
//#include "settings.h"
//#include "environment.h"
#include <QTextStream>
#include <QVariant>
#include <iostream>
#include <QProcess>
#include <QCoreApplication>
#include <QFileInfo>
#include <QDateTime>
#include <QDirIterator>

Work1Params Work1::params;

Work1::Work1() = default;



auto Work1::doWork() -> int
{
    // TODO 1. megtudni a kártyát
    // lsblk -dro name,path,type,tran,rm,vendor,model,phy-sec,mountpoint
    // ha több van, lista, választani
    // felírja:
    // sudo fdisk -l /dev/sdh
    // utolsó partíció utolsó szektora +1-ig írunk
    // https://stackoverflow.com/questions/22433257/windows-dd-esque-implementation-in-qt-5
    // mngm ~$ sudo dd if=/dev/sdb of=backup.img bs=512 count=15759360 conv=fsync
    // sudo dd of=/dev/sdm bs=512 if=/media/zoli/mentes/QT_raspi_anti/raspicam3.img status=progress oflag=sync
    //if(params.ofile.isEmpty()) return NOOUTFILE;
    //if(!params.ofile.endsWith(".img")) params.ofile+=".img";
    QString working_path = params.workingpath;
    if(working_path.isEmpty()) working_path = qApp->applicationDirPath();

    auto usbDrives = GetUsbDrives();
    if(usbDrives.isEmpty()) return ISEMPTY;
    QString usbdrive = (usbDrives.count()>1)?SelectUsbDrive(usbDrives):usbDrives[0];    
    //QStringList usbDrives = {"a", "b", "c", "d"};
    //auto usbdrive = SelectUsbDrive(usbDrives);
    int r=55;
    auto lastrec = GetLastRecord(usbdrive, &r); // megtudjuk a rekord méretet, ez kell a dd-hez
    if(lastrec==-1) return NOLASTREC;
    if(r==0) return NOUNITS;

    QStringList mountedparts = MountedParts(usbdrive);
    if(!mountedparts.isEmpty() && !UmountParts(mountedparts)) return CANNOTUNMOUNT;

    //zInfo("Last record on " + usbdrive + ": "+QString::number(lastrec+1));
    zInfo("Writing data to " + usbdrive);
    QString msg;

    bool confirmed = false;        

    if(params.ofile.isEmpty())
    {
        confirmed = true;
        auto most_recent = MostRecent(working_path);

        if(most_recent.isFile())zInfo("most recent:"+most_recent.fileName());
        params.ofile = GetFileName();
    }
    if(params.ofile.isEmpty()) return NOINPUTFILE;
    if(!params.ofile.endsWith(".img")) params.ofile+=".img";
    QFileInfo fi(params.ofile);
    if(!fi.exists()) return FILENOTEXIST;
    if(!confirmed) confirmed = ConfirmYes();

    auto b = fi.size();
    auto b_txt = BytesToString((double)b);
    zInfo(QStringLiteral("writing: %1 bytes (%2)").arg(b).arg(b_txt))

    auto fn = QDir(working_path).filePath(params.ofile);
    if(!confirmed) return NOTCONFIRMED;

    auto ddr = dd(fn, usbdrive, r, &msg);
    if(ddr) return COPYERROR;
//11381661696
//22229807
//512
    qint64 lastrec_dest= b/r;
    auto sha_fn1 = QDir(working_path).filePath("lastcopy.sha256");
    sha256sumDevice(usbdrive, r, lastrec_dest, sha_fn1);

    QString sha1 = getSha(sha_fn1);
    if(sha1.isEmpty()) return NOCHECK1;
    QString sha0 = getSha(fn+".sha256");
    if(sha1.isEmpty()) return NOCHECK0;
    zInfo(QStringLiteral("sha0: ")+sha0)
    zInfo(QStringLiteral("sha1: ")+sha1)
    if(sha1!=sha0) return CHECKSUMERROR;
    return OK;
}

QString Work1::getSha(const QString& fn){
    auto txt = com::helper::TextFileHelper::load(fn);
    if(txt.isEmpty()) return QString();
    auto ix0 = txt.indexOf(' ');
    if(ix0<0) return QString();
    return txt.left(ix0);
}
/*
$ dd if=/dev/sdb bs=4096 count=244992 | sha1sum

sudo dd bs=512 count=22229808 if=/dev/sdj | sha256sum > lastcopy.sha256
*/

int Work1::sha256sumDevice(const QString& fn, int r, qint64 b, const QString& sha_fn)
{
    QString e;
    zInfo("creating sha256 checksum...");
    auto cmd = QStringLiteral("/bin/sh -c \"sudo dd bs=%2 count=%3 if=%1 | sha256sum > %4\"").arg(fn).arg(r).arg(b).arg(sha_fn);
    auto out = Execute2(cmd);
    if(out.exitCode) return out.exitCode;
    if(out.stdOut.isEmpty()) return out.exitCode;
    zInfo("ok");
    return 0;
}

QString Work1::BytesToString(double b)
{
    double gb = b;

    if(gb<1024) return QStringLiteral("%1 Bytes").arg(gb, 0, 'f', 1);
    gb = gb/1024;
    if(gb<1024) return QStringLiteral("%1 KB").arg(gb, 0, 'f', 1);
    gb = gb/1024;
    if(gb<1024) return QStringLiteral("%1 MB").arg(gb, 0, 'f', 1);
    gb = gb/1024;
    if(gb<1024) return QStringLiteral("%1 GB").arg(gb, 0, 'f', 1);
    gb = gb/1024;
    return QStringLiteral("%1 TB").arg(gb, 0, 'f', 1);
}

QFileInfo Work1::MostRecent(const QString& path){
    QFileInfo most_recent;
    QFileInfo most_recent_cd;
    QFileInfo most_recent_cr;
    auto tstamp = QDateTime(QDate(1980,1,1));// ::currentDateTimeUtc().addYears(-1);//f1.lastModified();
    auto tstamp_cd = tstamp;//QDateTime::currentDateTimeUtc().addYears(-1);//f1.lastModified();
    auto tstamp_cr = tstamp;//QDateTime::currentDateTimeUtc().addYears(-1);//f1.lastModified();

    QDirIterator it(path);
    while (it.hasNext()) {
        QFileInfo fi(it.next());
        if(!fi.isFile()) continue;
        auto fn = fi.fileName();
        if(fn.endsWith(QLatin1String(".img")))
        {
            //if(!fi.completeSuffix().isEmpty()) continue;
            auto ts = fi.lastModified();
            if(ts>tstamp){ tstamp=ts; most_recent = fi;}
        }
    }
    return most_recent;
}

int Work1::GetLastRecord(const QString& drive, int* units)
{
    //sudo partx -r /dev/sdh
    /*
     * zoli@ubul:~$ sudo partx -r /dev/sdh
NR START END SECTORS SIZE NAME UUID
1 8192 532479 524288 256M  5e3da3da-01
2 532480 18702335 18169856 8,7G  5e3da3da-02

*/
    //auto cmd = QStringLiteral("sudo fdisk -l %1").arg(drive);
    auto cmd = QStringLiteral("sudo partx -rb %1").arg(drive);
    int lastrec = -1;
    auto out = com::helper::ProcessHelper::Execute(cmd);
    if(out.exitCode) return -1;
    if(out.stdOut.isEmpty()) return -1;

    for(auto&i:out.stdOut.split('\n'))
    {
        if(i.isEmpty()) continue;

        auto j = i.split(' ');
        bool isok;
        auto k = j[2].toInt(&isok);
        if(isok && k>lastrec)
        {
             lastrec = k;
             if(units!=nullptr)
             {
                auto sectors = j[3].toULong();
                auto size = j[4].toULong();
                int r = size/sectors;
                *units =r;
             }
        }

//        if(units!=nullptr && i.startsWith(QStringLiteral("Units:")))
//        {
//            auto j = i.split('=');

//            auto k = j[1].split(' ');
//            bool isok;
//            auto u = k[1].toInt(&isok);
//            if(isok) *units = u;
//        }
    }

    return lastrec;

}

auto Work1::GetUsbDrives() -> QStringList
{
    QStringList e;

    auto cmd = QStringLiteral("lsblk -dro name,path,type,tran,rm,vendor,model,phy-sec,mountpoint");
    auto out = com::helper::ProcessHelper::Execute(cmd);
    if(out.exitCode) return e;
    if(out.stdOut.isEmpty()) return e;

    for(auto&i:out.stdOut.split('\n'))
    {
        if(i.isEmpty()) continue;
        auto j=i.split(' ');
        if(j[8]=='/'||j[8]=="/boot") continue;
        if(            
            j[2]==QLatin1String("disk")&&
            j[3]==QLatin1String("usb")&&
            j[4]==QLatin1String("1")) e.append(j[1]);
        else if(j[0].startsWith("mmc")) e.append(j[1]);
    }

    return e;
}

QString Work1::SelectUsbDrive(const QStringList &usbdrives)
{
    int j = 1;
    for(auto&i:usbdrives) zInfo(QString::number(j++)+": "+i);j--;
    zInfo("select 1-"+QString::number(j))

    QTextStream in(stdin);
    auto intxt = in.readLine();
    bool isok;
    auto ix = intxt.toInt(&isok);
    if(!isok) return QString();
    if(ix<1||ix>j) return QString();
    return usbdrives[ix-1];
}

bool Work1::ConfirmYes()
{
    zInfo("Say 'yes' to continue or any other to quit.")
    QTextStream in(stdin);
    auto intxt = in.readLine();
    return intxt.trimmed().toLower()=="yes";
}

auto Work1::GetFileName() ->QString
{
    zInfo("Add output file name.")
    QTextStream in(stdin);
    return in.readLine();
}

QStringList Work1::MountedParts(const QString &src)
{
    QStringList e;
    auto cmd = QStringLiteral("sudo mount -l");
    auto out = com::helper::ProcessHelper::Execute(cmd);
    if(out.exitCode) return e;
    if(out.stdOut.isEmpty()) return e;

    for(auto&i:out.stdOut.split('\n'))
    {
        auto k = i.split(' ');
        if(k[0].startsWith(src)) e.append(k[0]);
    }
    return e;
}

bool Work1::UmountParts(const QStringList &src)
{
    bool isok = true;
    for(auto&i:src)
    {
        auto cmd = QStringLiteral("sudo umount %1").arg(i);
        auto out = com::helper::ProcessHelper::Execute(cmd);

        if(out.exitCode) isok = false;
    }
    return isok;
}

int Work1::dd(const QString& src, const QString& dst, int bs, QString *mnt)
{
    QString e;
    auto cmd = QStringLiteral("sudo dd of=%1 bs=%3 if=%2 status=progress conv=fdatasync").arg(dst).arg(src).arg(bs);
    //zInfo(cmd);
    //return 1;    
    auto out = Execute2(cmd);
    zInfo("copy ready, syncing...");
    Execute2(QStringLiteral("sync"));
    if(mnt)*mnt = out.ToString();

    if(out.exitCode) return out.exitCode;
    if(out.stdOut.isEmpty()) return out.exitCode;
    return 0;
}

com::helper::ProcessHelper::Output Work1::Execute2(const QString& cmd){
    qint64 pid;
    QProcess process;
    process.setProcessChannelMode(QProcess::ForwardedChannels);
    static QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("LD_LIBRARY_PATH", "/usr/lib"); // workaround - https://bugreports.qt.io/browse/QTBUG-2284
    process.setProcessEnvironment(env);
    static auto path = QCoreApplication::applicationDirPath();
    process.setWorkingDirectory(path);

    process.start(cmd);
    process.waitForFinished(-1); // will wait forever until finished
    return {process.readAllStandardOutput(), process.readAllStandardError(), process.exitCode()};
}
//https://stackoverflow.com/questions/20901884/piping-or-command-chaining-with-qprocess
//com::helper::ProcessHelper::Output Work1::Execute2Pipe(const QString& cmd1, const QString& cmd2){
//    //qint64 pid;
//    QProcess process1;
//    QProcess process2;


//    static QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
//    env.insert("LD_LIBRARY_PATH", "/usr/lib"); // workaround - https://bugreports.qt.io/browse/QTBUG-2284
//    static auto path = QCoreApplication::applicationDirPath();

//    //process1.setProcessChannelMode(QProcess::ForwardedChannels);
//    process1.setProcessEnvironment(env);
//    process1.setWorkingDirectory(path);
//    process1.setProcessChannelMode(QProcess::MergedChannels);

//    process2.setProcessEnvironment(env);
//    process2.setWorkingDirectory(path);
//    process2.setProcessChannelMode(QProcess::);

//    process1.setStandardOutputProcess(&process2);
//    process1.start(cmd1);
//    process2.start(cmd2);

//    //process1.waitForFinished(-1); // will wait forever until finished

//    process2.waitForFinished(); // will wait forever until finished

//    //QByteArray output = process2.readAll(); // now the output is found in the 2nd process


//    auto a0 = process2.readAllStandardOutput();
//    auto a1 = process2.readAllStandardError();
//    auto a2 = process2.exitCode();

//    process1.close();
//    process2.close();

//    com::helper::ProcessHelper::Output a = {a0,a1,a2};
//    return a;
//}

/*
NAME PATH TYPE TRAN RM VENDOR MODEL PHY-SEC
loop0 /dev/loop0 loop  0   512
loop1 /dev/loop1 loop  0   512
loop2 /dev/loop2 loop  0   512
loop3 /dev/loop3 loop  0   512
loop4 /dev/loop4 loop  0   512
loop5 /dev/loop5 loop  0   512
loop6 /dev/loop6 loop  0   512
loop7 /dev/loop7 loop  0   512
loop8 /dev/loop8 loop  0   512
loop9 /dev/loop9 loop  0   512
loop10 /dev/loop10 loop  0   512
loop11 /dev/loop11 loop  0   512
loop12 /dev/loop12 loop  0   512
loop13 /dev/loop13 loop  0   512
loop14 /dev/loop14 loop  0   512
loop15 /dev/loop15 loop  0   512
loop16 /dev/loop16 loop  0   512
loop17 /dev/loop17 loop  0   512
loop18 /dev/loop18 loop  0   512
loop19 /dev/loop19 loop  0   512
loop20 /dev/loop20 loop  0   512
loop21 /dev/loop21 loop  0   512
loop22 /dev/loop22 loop  0   512
loop23 /dev/loop23 loop  0   512
loop24 /dev/loop24 loop  0   512
loop25 /dev/loop25 loop  0   512
loop26 /dev/loop26 loop  0   512
loop27 /dev/loop27 loop  0   512
loop28 /dev/loop28 loop  0   512
loop29 /dev/loop29 loop  0   512
loop30 /dev/loop30 loop  0   512
loop31 /dev/loop31 loop  0   512
loop32 /dev/loop32 loop  0   512
loop33 /dev/loop33 loop  0   512
loop34 /dev/loop34 loop  0   512
loop35 /dev/loop35 loop  0   512
loop36 /dev/loop36 loop  0   512
loop37 /dev/loop37 loop  0   512
loop38 /dev/loop38 loop  0   512
loop39 /dev/loop39 loop  0   512
loop40 /dev/loop40 loop  0   512
loop41 /dev/loop41 loop  0   512
loop42 /dev/loop42 loop  0   512
loop43 /dev/loop43 loop  0   512
loop44 /dev/loop44 loop  0   512
loop45 /dev/loop45 loop  0   512
loop46 /dev/loop46 loop  0   512
loop47 /dev/loop47 loop  0   512
sda /dev/sda disk sata 0 ATA\x20\x20\x20\x20\x20 KINGSTON_SNVP325S264GB 512
sdb /dev/sdb disk sata 0 ATA\x20\x20\x20\x20\x20 WDC_WD30EZRX-00DC0B0 4096
sdc /dev/sdc disk sata 0 ATA\x20\x20\x20\x20\x20 WDC_WD30EZRX-00DC0B0 4096
sdd /dev/sdd disk sata 0 ATA\x20\x20\x20\x20\x20 WDC_WD1600YS-23SHB0 512
sde /dev/sde disk sata 0 ATA\x20\x20\x20\x20\x20 Hitachi_HDS721010CLA330 512
sdh /dev/sdh disk usb 1 Generic\x20 STORAGE_DEVICE 512
sr0 /dev/sr0 rom sata 1 HL-DT-ST HL-DT-STDVD-RAM_GSA-H60N 512
sr1 /dev/sr1 rom sata 1 TSSTcorp TSSTcorp_CDDVDW_SH-222AB 512
sr2 /dev/sr2 rom sata 1 TSSTcorp TSSTcorp_CDDVDW_SH-224DB 51
*/


/*
 * sudo fdisk -l /dev/sdh
[sudo] zoli jelszava:
Disk /dev/sdh: 14,86 GiB, 15931539456 bytes, 31116288 sectors
Disk model: STORAGE DEVICE
Units: sectors of 1 * 512 = 512 bytes
Sector size (logical/physical): 512 bytes / 512 bytes
I/O size (minimum/optimal): 512 bytes / 512 bytes
Disklabel type: dos
Disk identifier: 0x5e3da3da

Eszköz     Indítható  Start     Vége Szektorok  Size Id Típus
    /dev/sdh1              8192   532479    524288  256M  c W95 FAT32 (LBA)
    /dev/sdh2            532480 18702335  18169856  8,7G 83 Linux

*/
