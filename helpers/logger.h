#ifndef LOGGER_H
#define LOGGER_H

#include <QString>

//#if defined(Q_OS_LINUX)
//#define getLocInfo LocInfo(static_cast<const char*>(__PRETTY_FUNCTION__),__FILE__,__LINE__)
//#elif defined(Q_OS_WIN)
    #if defined(__GNUC__)
    #define getLocInfo Logger::LocInfo(static_cast<const char*>(__PRETTY_FUNCTION__) ,__FILE__,__LINE__)
    #elif defined(__MINGW32__)
    #define getLocInfo LocInfo(__PRETTY_FUNCTION__ ,__FILE__,__LINE__)
    #elif defined (_MSC_VER )
    #define getLocInfo LocInfo(__FUNCSIG__ ,__FILE__,__LINE__)
    #endif
//#endif


#define zError(msg) Logger::error2((msg), getLocInfo)

#define zWarning(msg) Logger::warning2((msg), getLocInfo);
#define zInfo(msg) Logger::info2((msg), getLocInfo);
#define zDebug() Logger::debug2(getLocInfo);
#define zTrace() Logger::trace2(getLocInfo);

#define zError2(msg,i) Logger::error2((msg), getLocInfo,(i));
#define zWarning2(msg,i) Logger::warning2((msg), getLocInfo,(i));
#define zInfo2(msg,i) Logger::info2((msg), getLocInfo,(i));

class Logger
{
public:
    class LocInfo
    {
    public:
        QString func;
        QString file;
        int line;

        LocInfo(const char *func, const char *file, int line);
        QString ToString() const;
    };

    enum ErrLevel:int {
        ERROR_,
        WARNING,
        INFO,
    };

    enum DbgLevel:int {
        NONE=0,
        TRACE,
        DEBUG,
    };

static void Init(ErrLevel level, DbgLevel dbglevel,
                 bool isBreak, bool isVerbose);
private:
    static ErrLevel _errlevel;
    static DbgLevel _dbglevel;
    static bool _isBreakOnError;
    static bool _isVerbose;
    static bool _isInited;

    static QString ToString(ErrLevel, const QString&, const QString&, const QString&);
    static QString ToString(DbgLevel level, const QString &msg, const QString &loci, const QString &st);
    static QString ToString(const ErrLevel &l);
    static QString ToString(const DbgLevel &l);
    static void err_message(ErrLevel level, const QString &msg);
    static void dbg_message(DbgLevel level, const QString& msg);
    static QString zStackTrace();

public:
    static void error2(const QString& msg, const LocInfo& l);
    static void warning2(const QString& msg, const LocInfo& l);
    static void debug2(const LocInfo& l);
    static void trace2(const LocInfo& l);
    static void info2(const QString& msg, const LocInfo& l);
    static void info2(const QStringList& msg, const LocInfo& l);
};

#endif // LOGGER_H
