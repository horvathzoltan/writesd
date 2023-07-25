#ifndef TEXTFILEHELPER_H
#define TEXTFILEHELPER_H

#include <QStringList>
#include <QTextStream>

class TextFileHelper
{
    static bool _verbose;
    static QString _lastError;
    static const QString PNE,FNE,CRF,CWF,LOK;
public:
    static void setVerbose(bool v);
    static const QString& LastError(){return _lastError;}

    static bool Load(const QString& filename, QString* e);
    static bool LoadLines(const QString& filename, QStringList* e);
    static bool LoadLinesContains(const QString &filename, const QStringList &t1, QStringList* e);    
    static bool Save(const QString &txt, const QString &fn, bool isAppend = false);
    static QString GetFileName(const QString& fn);
private:
    static void SetUtf8Encoding(QTextStream* st);
};

#endif // TEXTFILEHELPER_H
