#include "networkhelper.h"
#include "helpers/processhelper.h"


auto NetworkHelper::Ping(const QString& ip) -> bool
{
    //ProcessHelper::Model m = {.cmd = "ping", .args = {"-c1","-W1",ip}, .timeout=-1};
    auto out = ProcessHelper::ShellExecute("ping -c1 -W1", -1);
    return !out.exitCode;
}
