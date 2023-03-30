#ifndef PROCESSHELPER_H
#define PROCESSHELPER_H

#include <QStringList>

class ProcessHelper
{
public:
    struct Model{
        QString cmd;
        QStringList args;
        int timeout;
        bool showStdErr = true;

        static Model Parse(const QString& str);
        static QList<Model> ParseAsSudo(const QString& str, const QString& pwd);
        //void InsertArg(int ix, const QString& s);
        void Sudo();
        Model InsertPasswd(const QString& p);
        QString toString() const;
    };

    struct Output{
            QString stdOut;
            QString stdErr;
            int exitCode = 1;
            QString ToString();
        };

    //static Output Execute(const QString& cmd);
    //static QString Execute(const QStringList &cmds);
    //static Output ExecuteSudo(const QString &cmd);
    //static Output Execute3(const QString& cmd, const QStringList& args = {}, int timeout = -1);
    static Output Execute3(const Model& m);
    static Output Execute3(const QList<Model>& m);
    static void setVerbose(bool v){_verbose = v;}
private:
    static bool _verbose;
    static const QString SEPARATOR;

    //static Output Execute3(const QString& cmd, const QStringList& args = {}, int timeout = -1);
    //static Output Execute2(const QString &cmd);
};

#endif // PROCESSHELPER_H
