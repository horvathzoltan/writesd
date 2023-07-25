#include "logger.h"
#include <QDebug>
#include <csignal>
#ifdef Q_OS_LINUX
#include <execinfo.h>
#include <cxxabi.h>
#include <QDebug>
#endif

Logger::ErrLevel Logger::_errlevel = Logger::ErrLevel::INFO;
Logger::DbgLevel Logger::_dbglevel = Logger::DbgLevel::TRACE;
bool Logger::_isBreakOnError = false;
bool Logger::_isVerbose = false;
bool Logger::_isInited = false;

void Logger::Init(ErrLevel errlevel,
               DbgLevel dbglevel,
               bool isBreakOnError, bool isVerbose)
{
    _isInited = false;
     _errlevel = errlevel;
     _dbglevel = dbglevel;
    _isBreakOnError = isBreakOnError;
    _isVerbose = isVerbose;
    _isInited = true;
    //_GUILogger = ez;
    //_ui=uiptr;
}

Logger::LocInfo::LocInfo(const char *func, const char *file, int line)
{
    this->func=QString(func);
    this->file =QString(file);
    this->line=line;
}

QString Logger::LocInfo::ToString() const
{
    return file+':'+QString::number(line)+' '+func;
}

QString Logger::ToString(const ErrLevel &l){
    switch(l)
    {
    case ERROR_: return QStringLiteral("ERROR");
    case WARNING: return QStringLiteral("WARNING");
    case INFO: return QStringLiteral("INFO");
    default: return QStringLiteral("unknown");
    }
};

QString Logger::ToString(const DbgLevel &l){
    switch(l){
        case NONE: return QStringLiteral("TRACE");
        case TRACE: return QStringLiteral("TRACE");
        case DEBUG: return QStringLiteral("DEBUG");
        default: return QStringLiteral("unknown");
    }
};

QString Logger::ToString(DbgLevel level, const QString &msg, const QString &loci, const QString &st){
    auto txt = ToString(level);
    QString msg3;
    switch(level){
    case DbgLevel::TRACE:
        msg3= txt+": "+loci;
        break;
    case DbgLevel::DEBUG:
        msg3= txt+": "+msg+"\n"+loci+"\n"+st;
        break;
    default: break;
    }
    return msg3;
}

QString Logger::ToString(ErrLevel errlevel, const QString &msg, const QString &loci, const QString &st)
{
    auto level = ToString(errlevel);
    QString msg3;

    switch(errlevel)
    {
    case ErrLevel::ERROR_:
        msg3= level+": "+msg+"\n"+loci+"\n"+st;
        break;
    case ErrLevel::WARNING:
        msg3= level+": "+msg+"\n"+loci;
        break;

    case ErrLevel::INFO:
        msg3= level+": "+msg;
        if(_isVerbose) msg3+="\n"+loci;
        break;
    default: break;
    }

    return msg3;
}

void Logger::dbg_message(DbgLevel level, const QString& msg)
{

#ifdef QT_DEBUG
#ifdef Q_OS_WIN
        auto a = __FUNCTION__;
#elif defined(Q_OS_LINUX)
    auto a = static_cast<const char*>(__PRETTY_FUNCTION__);
#endif
    switch(level){
    case DbgLevel::DEBUG:
    case DbgLevel::TRACE:
        QMessageLogger(QT_MESSAGELOG_FILE, QT_MESSAGELOG_LINE, a).debug().noquote()<<msg;
        break;
    default:
        break;
    }

#ifdef Q_OS_LINUX
    if((level ==DbgLevel::DEBUG) && _isBreakOnError) std::raise(SIGTRAP);
#endif
#endif
}


void Logger::err_message(ErrLevel level, const QString& msg)
{
//    GUIModes::Modes guimode;

#ifdef QT_DEBUG
#ifdef Q_OS_WIN
    auto a = __FUNCTION__;
#elif defined(Q_OS_LINUX)
    auto a = static_cast<const char*>(__PRETTY_FUNCTION__);
#endif
    switch(level)
    {
    case ErrLevel::ERROR_:
        QMessageLogger(QT_MESSAGELOG_FILE, QT_MESSAGELOG_LINE, a).critical().noquote()<<msg;
  //      guimode = GUIModes::ERROR;
        break;
    case ErrLevel::WARNING:
        QMessageLogger(QT_MESSAGELOG_FILE, QT_MESSAGELOG_LINE, a).warning().noquote()<<msg;
    //    guimode = GUIModes::WARNING;
        break;
    case ErrLevel::INFO:
        QMessageLogger(QT_MESSAGELOG_FILE, QT_MESSAGELOG_LINE, a).info().noquote()<<msg;
      //  guimode = GUIModes::INFO;
        break;
    default:
        //guimode = GUIModes::INFO;
        break;
    }
//    if(_GUILogger) {
//        _GUILogger(guimode, msg, "", "", _ui, flag);
//    }
#ifdef Q_OS_LINUX
    if((level==ErrLevel::ERROR_) && _isBreakOnError) std::raise(SIGTRAP);
#endif
#endif
}


#ifdef Q_OS_LINUX
QString Logger::zStackTrace()
{
    QStringList e;

    unsigned int max_frames = 64;

    e << QStringLiteral("stack trace:");

    // storage array for stack trace address data
    void* addrlist[max_frames+1];

    // retrieve current stack addresses
    int addrlen = backtrace(static_cast<void**>(addrlist), sizeof(addrlist) / sizeof(void*));

    if (addrlen == 0)
    {
        e << QStringLiteral("<empty, possibly corrupt>");
        return e.join('\n');
    }

    // resolve addresses into strings containing "filename(function+address)", this array must be free()-ed
    auto symbollist = backtrace_symbols(static_cast<void**>(addrlist), addrlen);

    // allocate string which will be filled with the demangled function name
    size_t funcnamesize = 256;
    auto funcname = static_cast<char*>(malloc(funcnamesize));

    // iterate over the returned symbol lines. skip the first, it is the address of this function.
    for (int i = 1; i < addrlen; i++)
    {
    char *begin_name = nullptr, *begin_offset = nullptr, *end_offset = nullptr;

    for (char *p = symbollist[i]; *p; ++p)
    {
        if (*p == '(')
        {
            begin_name = p;
        }
        else if (*p == '+')
        {
            begin_offset = p;
        }
        else if (*p == ')' && begin_offset)
        {
            end_offset = p;
            break;
        }
    }

    if (begin_name && begin_offset && end_offset
        && begin_name < begin_offset)
    {
        *begin_name++ = '\0';
        *begin_offset++ = '\0';
        *end_offset = '\0';

        int status;
        char* ret = abi::__cxa_demangle(begin_name,funcname, &funcnamesize, &status);
        if (status == 0)
        {
            funcname = ret; // use possibly realloc()-ed string
            e << QStringLiteral("%1: %2 + %3").arg(symbollist[i],ret,begin_offset);
            //e << QStringLiteral("%1").arg(funcname);
        }
        else
        {
            // demangling failed. Output function name as a C function with
            // no arguments.
            e << QStringLiteral("%1: %2 + %3").arg(symbollist[i],begin_name,begin_offset);
            //e << QStringLiteral("%1").arg(begin_name);
        }
    }
    else
    {
        // couldn't parse the line? print the whole line.
        e << QString(symbollist[i]);
    }
    }

    free(funcname);
    free(symbollist);

    auto a =  e.join('\n');
    return a;
}

#endif

#ifdef Q_OS_WIN
QString Log::zStackTrace(){
    QStringList e;

   // unsigned int max_frames = 64;

    e << QStringLiteral("stack trace:");

    // WIN implementációt adni
    e << QStringLiteral("?");

    auto a =  e.join('\n');

    return a;
}
#endif

void Logger::info2(const QString& msg, const LocInfo& locinfo)
{
    if(!_isInited) return;
    if(_errlevel>ErrLevel::INFO) return;
    QString li;
    if(_isVerbose)
    {
        li = locinfo.ToString();
    }

    auto msg2 = ToString(ErrLevel::INFO, msg, li, nullptr);
    err_message(ErrLevel::INFO, msg);
}

void Logger::info2(const QStringList& msgl, const LocInfo& locinfo)
{
    if(!_isInited) return;
    if(_errlevel>ErrLevel::INFO) return;
    QString li;
    if(_isVerbose)
    {
        li = locinfo.ToString();
    }

    for(auto&msg:msgl)
    {
        auto msg2 = ToString(ErrLevel::INFO, msg, nullptr, nullptr);
        err_message(ErrLevel::INFO, msg2);
    }
}


void Logger::error2(const QString& msg,  const LocInfo& locinfo){
    if(!_isInited) return;
    if(_errlevel>ErrLevel::ERROR_) return;

    auto li = locinfo.ToString();
    auto st = Logger::zStackTrace();

    auto msg2 = ToString(ErrLevel::ERROR_, msg, li, st);
    err_message(ErrLevel::ERROR_, msg2);
    }

void Logger::warning2(const QString& msg, const LocInfo& locinfo){
    if(!_isInited) return;
    if(_errlevel>ErrLevel::WARNING) return;
    auto li = locinfo.ToString();
    auto msg2 = ToString(ErrLevel::WARNING, msg, li, nullptr);
    err_message(ErrLevel::WARNING, msg);
    }

void Logger::debug2(const LocInfo& locinfo){
    if(_dbglevel==DbgLevel::NONE || _dbglevel>DbgLevel::DEBUG) return;
    auto li = locinfo.ToString();
    auto st = Logger::zStackTrace();

    auto msg2 = ToString(DbgLevel::DEBUG, nullptr, li, st);
    dbg_message(DbgLevel::DEBUG, msg2);
    }

void Logger::trace2(const LocInfo& locinfo){
    if(_dbglevel==DbgLevel::NONE || _dbglevel>DbgLevel::TRACE) return;
    auto li = locinfo.ToString();
    auto msg2 = ToString(DbgLevel::TRACE, nullptr, li, nullptr);
    dbg_message(DbgLevel::TRACE, msg2);
    }
