#include "work1.h"
#include "helpers/logger.h"
#include "helpers/textfilehelper.h"
#include "helpers/processhelper.h"
//#include "sqlhelper.h"
//#include "settings.h"
//#include "environment.h"
#include <QTextStream>
#include <QVariant>
//#include <iostream>
#include <QProcess>
#include <QCoreApplication>
#include <QFileInfo>
#include <QDateTime>
#include <QDate>
#include <QDirIterator>

Work1Params Work1::params;

Work1::Work1() = default;



auto Work1::doWork() -> int
{
    if(params.passwd.isEmpty()){
        params.passwd = GetFileName("Add sudo password.");
    }

    if(params.passwd.isEmpty()) return NO_PASSWD;

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

    QList<UsbDriveModel> usbDrives_all = GetUsbDrives();
    if(usbDrives_all.isEmpty()) return ISEMPTY;

    //QString usbdrive = (usbDrives.count()>1)?SelectUsbDrive(usbDrives):usbDrives[0];

    QList<UsbDriveModel> usbdrives;

    if(usbDrives_all.count()>0){
        usbdrives = SelectUsbDrives(usbDrives_all);
    } else{
        usbdrives=QList<UsbDriveModel>();
    }

    if(usbdrives.isEmpty()) return NO_USBDRIVE;

    //QStringList usbDrives = {"a", "b", "c", "d"};
    //auto usbdrive = SelectUsbDrive(usbDrives);
    //int r=55;
    QList<RecModel> lastrecs = GetLastRecords(usbdrives); // megtudjuk a rekord méretet, ez kell a dd-hez

    bool isValidRecords = CheckRecords_Units(lastrecs);
    if(!isValidRecords) return NO_LASTREC;

    for(auto&usbdrive:usbdrives){
        QStringList mountedparts = MountedParts(usbdrive.devicePath);
        if(!mountedparts.isEmpty() && !UmountParts(mountedparts)) return CANNOT_UNMOUNT;
    }

    //zInfo("Last record on " + usbdrive + ": "+QString::number(lastrec+1));
    QString drivestxt = "";
    for(auto&usbdrive:usbdrives){
        if(!drivestxt.isEmpty()) drivestxt+=",";
        drivestxt+=usbdrive.devicePath;
    }

    zInfo("Writing data to "+drivestxt);
    QString msg;

    bool confirmed = false;

    if(params.ofile.isEmpty())
    {
        confirmed = true;
        auto most_recent = MostRecent(working_path);

        if(most_recent.isFile())zInfo("most recent:"+most_recent.fileName());
        params.ofile = GetFileName("Add output file name.");
    }

    if(params.ofile.isEmpty()) return NO_INPUTFILE;
    if(!params.ofile.endsWith(".img")) params.ofile+=".img";
    QFileInfo fi(params.ofile);
    if(!fi.exists()) return FILENOTEXIST;
    if(!confirmed) confirmed = ConfirmYes();

    qint64 b = fi.size();
    auto b_txt = BytesToString((double)b);
    zInfo(QStringLiteral("writing: %1 bytes (%2)").arg(b).arg(b_txt))

    QList<UsbDriveModel> smallcards = GetSmallUsbDrive(usbdrives, b);
    if(!smallcards.isEmpty())
    {
        zInfo("drive too small:")
        for(auto&a:smallcards){
            zInfo(a.toString());
        }
        return DRIVE_SIZE_ERROR;
    }

    auto fn = QDir(working_path).filePath(params.ofile);
    if(!confirmed) return NOT_CONFIRMED;

    auto sha_tmp_fn = QDir(working_path).filePath("temp.sha256");

    // ha van már temp és, töröljük
    TextFileHelper::Delete(sha_tmp_fn);

    int units = lastrecs[0].units;

    QStringList devpaths = GetDevPaths(usbdrives);
    // ***********************
    // dd if=/dev/sdb | tee >(dd of=/dev/sdc) | tee >(dd of=/dev/sdj) | dd of=/dev/sdh
    auto ddr = dd2(fn, devpaths, units, &msg);
    //if(ddr) return COPY_ERROR;
    //11381661696
    //22229807
    //512

    // a frissen kiírt imaget visszaolvassuk, az sha megy a tempbe
    qint64 lastrec_dest= b/units;

    //********************
    //sha256sumDevice(usbdrive.devicePath, r, lastrec_dest, sha_tmp_fn);
    QString sha_tmp = getSha(sha_tmp_fn);
    if(sha_tmp.isEmpty()) return NO_CHECK0;

    // a forrásfájl sha-ja
    QString sha_img = getSha(fn+".sha256");
    if(sha_img.isEmpty()) return NO_CHECK1;

    zInfo(QStringLiteral("sha_tmp: ")+sha_tmp)
    zInfo(QStringLiteral("sha_img: ")+sha_img)
    if(sha_tmp!=sha_img) return CHECKSUM_ERROR;

    return OK;
}

QString Work1::getSha(const QString& fn){
    QString fn2 = TextFileHelper::GetFileName(fn);
    zInfo("sha256sum from: "+fn2);
    QString txt;
    bool ok = TextFileHelper::Load(fn, &txt);
    if(!ok) return {};
    if(txt.isEmpty()) return {};
    auto ix0 = txt.indexOf(' ');
    if(ix0<0) return {};
    auto a =  txt.left(ix0);
    return a;
}

/*
$ dd if=/dev/sdb bs=4096 count=244992 | sha1sum

sudo dd bs=512 count=22229808 if=/dev/sdj | sha256sum > lastcopy.sha256
*/
// dd if=image.bin | tee >(dd of=/dev/sdc) | dd of=/dev/sdh

int Work1::sha256sumDevice(const QString& fn, int r, qint64 b, const QString& sha_fn)
{
    auto sha_fn2 = TextFileHelper::GetFileName(sha_fn);
    zInfo("sha256sum on dev: "+fn+" -> "+sha_fn2);
    auto cmd1 = QStringLiteral("dd bs=%2 count=%3 if=%1 status=progress").arg(fn).arg(r).arg(b);

    auto m = ProcessHelper::Model::ParseAsSudo(cmd1, params.passwd);
    m[1].showStdErr=false;
    m.append({.cmd="sha256sum", .args = {}, .timeout=-1, .showStdErr = false });

    auto out = ProcessHelper::Execute3(m);

    if(out.exitCode) return out.exitCode;
    if(out.stdOut.isEmpty()) return out.exitCode;
    TextFileHelper::Save(out.stdOut, sha_fn);
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

    auto tstamp = QDate(1980,1,1).startOfDay();
    auto tstamp_cd = tstamp;
    auto tstamp_cr = tstamp;

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
    auto cmd = QStringLiteral("partx -rb %1").arg(drive);
    auto m2 = ProcessHelper::Model::ParseAsSudo(cmd, params.passwd);
    //m2[0].showStdErr= false;
    m2[1].showStdErr= false;
    auto out = ProcessHelper::Execute3(m2);

    if(out.exitCode) return -1;
    if(out.stdOut.isEmpty()) return -1;

    int lastrec = -1;
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

QList<RecModel> Work1::GetLastRecords(const QList<UsbDriveModel> &usbdrives){
    QList<RecModel> e;
    for(auto&a:usbdrives){
        int units=-1;
        int lastrec = GetLastRecord(a.devicePath, &units);
        RecModel m;
        m.lastrec = lastrec;
        m.units = units;
        e.append(m);
    }
    return e;
}

bool Work1::CheckRecords_Units(const QList<RecModel>& records){
    //int p_lastrec=-1;
    int p_units = -1;
    for(auto&a:records){
        if(a.units==-1) return false;
        if(a.lastrec==-1) return false;
      //  if(p_lastrec!=-1 && p_lastrec != a.lastrec) return false;
      //  p_lastrec = a.lastrec;
        if(p_units!=-1 && p_units != a.units) return false;
        p_units = a.units;
    }
    return true;
}

QList<UsbDriveModel> Work1::GetSmallUsbDrive(const QList<UsbDriveModel>& usbdrives, long size){
    QList<UsbDriveModel> e;
    for(auto&a:usbdrives){
        if(a.size<size) e.append(a);
    }
    return e;
}

QStringList Work1::GetDevPaths(const QList<UsbDriveModel>& usbdrives){
    QStringList e;
    for(auto&a:usbdrives){
        e.append(a.devicePath);
    }
    return e;
}

QList<UsbDriveModel> Work1::GetUsbDrives()
{
    QList<UsbDriveModel> e;

    auto cmd = QStringLiteral("lsblk -dbro name,path,type,tran,rm,vendor,model,phy-sec,size,mountpoint");
    auto m2 = ProcessHelper::Model::Parse(cmd);
    auto out = ProcessHelper::Execute3(m2);
    if(out.exitCode) return e;
    if(out.stdOut.isEmpty()) return e;

    for(auto&i:out.stdOut.split('\n'))
    {
        if(i.isEmpty()) continue;
        auto j=i.split(' ');

        bool isBoot = j[9]=='/'||j[9]=="/boot";

        if(isBoot) continue;

        bool isRemovableDisk = j[2]==QLatin1String("disk")&&
                               j[4]==QLatin1String("1");

        if(!isRemovableDisk) continue;

        bool isUsb = j[3]==QLatin1String("usb");

        bool isMmc = j[0].startsWith("mmc");

        bool ok;
        long size = j[8].toLong(&ok);
        bool hasCard = ok && size>0;

        if(isRemovableDisk && (isUsb || isMmc) && hasCard){
            UsbDriveModel m;
            m.devicePath = j[1];
            m.usbPath = GetUsbPath(j[1]);
            m.partLabels = GetPartLabels(j[1]);
            m.size = size;
            e.append(m);
        }
    }

    return e;
}

QString Work1::GetUsbPath(const QString &dev)
{
    auto cmd = QStringLiteral("udevadm info -q path");
    cmd+=" "+dev;
    auto m2 = ProcessHelper::Model::Parse(cmd);
    auto out = ProcessHelper::Execute3(m2);
    if(out.exitCode) return "";
    if(out.stdOut.isEmpty()) return "";

    QString e = "";
    int ix0 = out.stdOut.indexOf("/usb");
    if(ix0>=0){
        int ix1 = out.stdOut.indexOf("/host", ix0);
        if(ix1>=0){
            e = out.stdOut.mid(ix0, ix1-ix0);
        }
    }

    return e;
}

QStringList Work1::GetPartLabels(const QString &dev)
{
    auto cmd = QStringLiteral("lsblk -r %1 -o NAME,path,LABEL,Type").arg(dev);
    auto m2 = ProcessHelper::Model::Parse(cmd);
    auto out = ProcessHelper::Execute3(m2);
    if(out.exitCode) return QStringList();
    if(out.stdOut.isEmpty()) return QStringList();

    QStringList e;
    for(auto&i:out.stdOut.split('\n'))
    {
        if(i.isEmpty()) continue;

        auto j=i.split(' ');

        bool isPart = j[3]=="part";

        if(isPart){
            e.append(j[2]);
        }
    }
    return e;
}



UsbDriveModel Work1::SelectUsbDrive(const QList<UsbDriveModel> &usbdrives)
{
    int j = 1;
    for(auto&i:usbdrives) zInfo(QString::number(j++)+": "+i.toString());j--;
    zInfo("select 1-"+QString::number(j))

        QTextStream in(stdin);
    auto intxt = in.readLine();
    bool isok;
    auto ix = intxt.toInt(&isok);
    if(!isok) return UsbDriveModel();
    if(ix<1||ix>j) return UsbDriveModel();
    return usbdrives[ix-1];
}

QList<UsbDriveModel> Work1::SelectUsbDrives(const QList<UsbDriveModel> &usbdrives)
{
    int j = 1;
    for(auto&i:usbdrives) zInfo(QString::number(j++)+": "+i.toString());j--;
    zInfo("select drive with usb path")

    QTextStream in(stdin);
    auto intxt = in.readLine();
    bool isok = !intxt.isEmpty();
    //auto ix = intxt.toInt(&isok);
    if(!isok) return QList<UsbDriveModel>();

    QList<UsbDriveModel> e;
    for(auto&device:usbdrives){
        QString usbPath = device.GetLastUsbTag();
        if(usbPath.startsWith(intxt)){
            e.append(device);
        }
    }
    return e;
}

bool Work1::ConfirmYes()
{
    zInfo("Say 'yes' to continue or any other to quit.")
        QTextStream in(stdin);
    auto intxt = in.readLine();
    return intxt.trimmed().toLower()=="yes";
}

auto Work1::GetFileName(const QString& msg) ->QString
{
    zInfo(msg);
    QTextStream in(stdin);
    return in.readLine();
}

QStringList Work1::MountedParts(const QString &src)
{
    QStringList e;
    auto cmd = QStringLiteral("mount -l");
    auto m2 = ProcessHelper::Model::ParseAsSudo(cmd, params.passwd);
    auto out = ProcessHelper::Execute3(m2);

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
        auto cmd = QStringLiteral("umount %1").arg(i);
        auto m2 = ProcessHelper::Model::ParseAsSudo(cmd, params.passwd);
        auto out = ProcessHelper::Execute3(m2);


        if(out.exitCode) isok = false;
    }
    return isok;
}

int Work1::dd(const QString& src, const QString& dst, int bs, QString *mnt)
{
    zInfo("copying...");//:"+src+"->"+dst );
    auto cmd = QStringLiteral("dd of=%1 bs=%3 if=%2 status=progress conv=fdatasync").arg(dst).arg(src).arg(bs);
    auto m2 = ProcessHelper::Model::ParseAsSudo(cmd, params.passwd);
    auto out = ProcessHelper::Execute3(m2);//2
    if(out.exitCode) return 1;

    zInfo("copy ready, syncing...");
    auto m3 = ProcessHelper::Model::Parse(QStringLiteral("sync"));
    ProcessHelper::Execute3(m3);//2

    if(mnt)*mnt = out.ToString();

    if(out.exitCode) return out.exitCode;
    if(out.stdOut.isEmpty()) return out.exitCode;
    return 0;
}

//dd if=/dev/sdb | tee >(dd of=/dev/sdc) | tee >(dd of=/dev/sdj) | dd of=/dev/sdh
int Work1::dd2(const QString& src, const QStringList& dsts, int bs, QString *mnt)
{
    zInfo("copying...");//:"+src+"->"+dst );

    QString cmd2;
    for(auto&dst:dsts){
        cmd2 = QStringLiteral("of=%1").arg(dst);
    }

    auto cmd = QStringLiteral("dcfldd %1 bs=%3 if=%2 status=progress").arg(cmd2).arg(src).arg(bs);

    zInfo("cmd:"+cmd);

    auto m2 = ProcessHelper::Model::ParseAsSudo(cmd, params.passwd);
    auto out = ProcessHelper::Execute3(m2);//2
    if(out.exitCode) return 1;

    zInfo("copy ready, syncing...");
    auto m3 = ProcessHelper::Model::Parse(QStringLiteral("sync"));
    ProcessHelper::Execute3(m3);//2

    if(mnt)*mnt = out.ToString();

    if(out.exitCode) return out.exitCode;
    if(out.stdOut.isEmpty()) return out.exitCode;
    return 0;
}

QString UsbDriveModel::GetLastUsbTag() const
{
    QString usbn;
    int ix0 = usbPath.lastIndexOf('/');
    int maxix = usbPath.length()-1;

    if(ix0>=0 && ix0<maxix){
        usbn = usbPath.mid(ix0+1);
    } else{
        usbn =  "";
    }

    return usbn;
}

QString UsbDriveModel::toString() const
{
    QString n =  devicePath;

    QString usbn = GetLastUsbTag();
    if(usbn.isEmpty()) usbn = usbPath;

    QString labels="";
    bool hasParts = !partLabels.isEmpty();
    if(hasParts){
        labels = partLabels.join(',');
    }
    return n+": "+usbn+": "+labels;
}

bool UsbDriveModel::isValid()
{
    if(devicePath.isEmpty()) return false;
    if(usbPath.isEmpty()) return false;
    return true;
}
