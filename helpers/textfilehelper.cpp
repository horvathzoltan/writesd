#include "textfilehelper.h"
#include "helpers/logger.h"

#include <QFileInfo>

bool TextFileHelper::_verbose = false;
QString TextFileHelper::_lastError;

const QString TextFileHelper::PNE = QStringLiteral("path is not absolute: %1"),
TextFileHelper::FNE = QStringLiteral("file not exists: %1"),
TextFileHelper::CRF = QStringLiteral("cannot read file (%1): %2"),
TextFileHelper::CWF = QStringLiteral("cannot write file (%1): %2"),
TextFileHelper::LOK = QStringLiteral("loaded: %1");

void TextFileHelper::setVerbose(bool v)
{
    _verbose = v;
}

void TextFileHelper::SetUtf8Encoding(QTextStream* st)
{
    if(st==nullptr) return;
#if QT_VERSION >= 0x050000 && QT_VERSION < 0x060000
        st->setCodec("UTF-8"); //5.15.2
#elif QT_VERSION >= 0x06
        st->setEncoding(QStringConverter::Utf8);
#endif
}


bool TextFileHelper::Load(const QString& filename, QString* e)
{
    if(filename.isEmpty()) return false;
    if(e==nullptr) return false;

    QFileInfo fi(filename);
    if(!fi.isAbsolute())
    {
        _lastError = PNE.arg(filename);
        if(_verbose) zInfo(_lastError);
        return false;
    }

    if(!fi.exists())
    {
        _lastError = FNE.arg(filename);
        if(_verbose) zInfo(_lastError);
        return false;
    }

    QFile f(filename);

    // TODO ha relatív a filename, akkor abszolúttá kell tenni
    // egyébként megnyitható azaz

    bool ok = f.open(QIODevice::ReadOnly);
    if(!ok) {
        _lastError = FNE.arg(f.errorString(),filename);
        if(_verbose) zInfo(_lastError);
        return false;
    }

    _lastError.clear();
    if(_verbose) zInfo(LOK.arg(filename));

    QTextStream st(&f);
    SetUtf8Encoding(&st);
    *e = st.readAll();
    f.close();
    return true;
}

bool TextFileHelper::LoadLines(const QString& filename, QStringList* e) {
    if(filename.isEmpty()) return false;
    if(e==nullptr) return false;

    QFileInfo fi(filename);
    if(!fi.isAbsolute())
    {
        _lastError = PNE.arg(filename);
        if(_verbose) zInfo(_lastError);
        return false;
    }

    if(!fi.exists())
    {
        _lastError = FNE.arg(filename);
        if(_verbose) zInfo(_lastError);
        return false;
    }

    QFile f(filename);

    // TODO ha relatív a filename, akkor abszolúttá kell tenni
    // egyébként megnyitható azaz

    bool ok = f.open(QIODevice::ReadOnly);
    if(!ok){
        _lastError = FNE.arg(f.errorString(),filename);
        if(_verbose) zInfo(_lastError);
        return false;
    }

    _lastError.clear();
    if(_verbose) zInfo(LOK.arg(filename));

    QTextStream st(&f);

    SetUtf8Encoding(&st);

    do e->append(st.readLine()); while (!st.atEnd());
    f.close();
    return true;
}

bool TextFileHelper::LoadLinesContains(const QString& filename, const QStringList& t1, QStringList* e) {
    if(filename.isEmpty()) return false;
    if(e==nullptr) return false;

    QFileInfo fi(filename);
    if(!fi.isAbsolute())
    {
        _lastError = PNE.arg(filename);
        if(_verbose) zInfo(_lastError);
        return false;
    }

    if(!fi.exists())
    {
        _lastError = FNE.arg(filename);
        if(_verbose) zInfo(_lastError);
        return false;
    }

    QFile f(filename);

    // TODO ha relatív a filename, akkor abszolúttá kell tenni
    // egyébként megnyitható azaz

    QStringList t2;
    for(auto&t:t1) t2<<t.toUpper();
    bool opened = f.open(QIODevice::ReadOnly);
    if(!opened){
        _lastError = FNE.arg(f.errorString(),filename);
        if(_verbose) zInfo(_lastError);
        return false;
    }

    _lastError.clear();
    if(_verbose) zInfo(LOK.arg(filename));

    QTextStream st(&f);
    SetUtf8Encoding(&st);

    do
    {
        auto line = st.readLine();
        auto l2 = line.toUpper();
        bool minden_van = true;
        for(auto&t:t2){
            if(!l2.contains(t))
            {
                minden_van = false;
                break;
            }
        }
        if(minden_van) e->append(line);

    } while (!st.atEnd());
    f.close();
    return true;
}

bool TextFileHelper::Save(const QString& txt, const QString& fn, bool isAppend) {

    QFile f(fn);

    auto om = QIODevice::WriteOnly | QIODevice::Text; // openmode
    if(isAppend) om |= QIODevice::Append;

    bool opened = f.open(om);
    if (!opened){
        _lastError = CWF.arg(fn);
        if(_verbose) zInfo(_lastError);
        return false;
        }

    QTextStream out(&f);
    SetUtf8Encoding(&out);
    //out.setCodec(QTextCodec::codecForName("UTF-8"));
    //out.setGenerateByteOrderMark(true);
    out << txt;//.toUtf8();
    f.close();
    return true;
}

QString TextFileHelper::GetFileName(const QString &fn)
{
    QFileInfo fi(fn);
    QString e = fi.fileName();
    return e;
}
