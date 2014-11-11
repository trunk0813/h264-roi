#include "zlog.h"
#include <ctime>
#include <cstdio>

#if COMPILER == MSVC
    #include <windows.h>
#endif

namespace LibChaos {

bool ZLog::_init = false;
ZLogWorker ZLog::worker;
AsArZ ZLog::thread_ids;

ZLog::ZLog(zlog_source source) : source_mode(source), stdiolog(false), write_on_destruct(false), newline(true), rawlog(false), /*synclog(false),*/ noqueue(false){}

ZLog::~ZLog(){
    if(write_on_destruct)
        flushLog();
}

void ZLog::flushLog(){
    LogJob out;
    out.source = source_mode;
    out.stdio = stdiolog;
    out.newln = newline;
    out.raw = rawlog;
    out.log = buffer;
    if(info[ZLogWorker::clock].isEmpty())
        info[ZLogWorker::clock] = getClock();
    if(info[ZLogWorker::date].isEmpty())
        info[ZLogWorker::date] = getDate();
    if(info[ZLogWorker::time].isEmpty())
        info[ZLogWorker::time] = getTime();
    if(info[ZLogWorker::thread].isEmpty())
        info[ZLogWorker::thread] = getThread();
    out.pinfo = info;

    if(_init && !noqueue){
        worker.queue(out);
    } else {
        ZLogWorker::doLog(out);
    }
}

ZLog &ZLog::operator<<(zlog_flag flag){
    if(flag == flush){
        flushLog();
    } else if(flag == newln){
        log("\n");
    } else if(flag == flushln){
        log("\n");
        flushLog();
    } else if(flag == noln){
        newline = false;
    } else if(flag == raw){
        rawlog = true;
    } else if(flag == stdio){
        stdiolog = true;
    } else if(flag == this_thread){
        noqueue = true;
    }
    return *this;
}

ZLog &ZLog::log(ZString logtext){
    buffer << logtext;
    write_on_destruct = true;
    return *this;
}

ZLog &ZLog::operator<<(ZBinary bin){
    ZString text;
    for(zu64 i = 0; i < bin.size(); ++i){
        unsigned char tmp = bin[i];
        // Log in hexadecimal pairs
        text += ZString::ItoS(tmp, 16, 2);
    }
    return log(text);
}

ZLog::zlog_preproc ZLog::makePreProc(info_type type, ZString dat){
    return { type, dat };
}
ZLog &ZLog::operator<<(zlog_preproc in){
    info[in.type] = in.info;
    return *this;
}

ZString ZLog::pullBuffer(){
    ZString tmp = buffer;
    buffer.clear();
    write_on_destruct = false;
    return tmp;
}

ZString ZLog::getDate(){
#if COMPILER == MSVC
    SYSTEMTIME time;
    GetLocalTime(&time);
    ZString str = ZString::ItoS(time.wMonth, 10, 2) + "/" + ZString::ItoS(time.wDay, 10, 2) + "/" + ZString::ItoS(time.wYear, 10, 2);
    return str;
#else
    time_t raw;
    time(&raw);
    struct tm *time;
    time = localtime(&raw);
    char buffer[20];
    sprintf(buffer, "%02d/%02d/%02d", time->tm_mon + 1, time->tm_mday, time->tm_year - 100);
    ZString out(buffer);
    return out;
#endif
}

ZString ZLog::getTime(){
#if COMPILER == MSVC
    SYSTEMTIME time;
    GetLocalTime(&time);
    ZString str = ZString::ItoS(time.wHour, 10, 2) + ":" + ZString::ItoS(time.wMinute, 10, 2) + ":" + ZString::ItoS(time.wSecond, 10, 2);
    return str;
#else
    time_t raw;
    time(&raw);
    struct tm *time;
    time = localtime(&raw);
    char buffer[20];
    sprintf(buffer, "%02d:%02d:%02d", time->tm_hour, time->tm_min, time->tm_sec);
    ZString out(buffer);
    return out;
#endif
}

ZString ZLog::getClock(){
    clock_t raw = clock();
    float rawsecs = (float)raw / (float)CLOCKS_PER_SEC;
    int secs = (int)rawsecs;
    int msecs = (int)(rawsecs - (float)secs) * 1000;
    int mins = secs / 60;
    secs = secs - (mins * 60);
    int hrs = mins / 60;
    mins = mins - (hrs * 60);
    //char buffer[20];
    //sprintf(buffer, "%02d:%02d:%02d:%03d", hrs, mins, secs, msecs);
    //ZString out(buffer);
    ZString out = ZString::ItoS((zu64)hrs, 10, 2) + ":" + ZString::ItoS((zu64)mins, 10, 2) + ":" + ZString::ItoS((zu64)secs, 10, 2) + ":" + ZString::ItoS((zu64)msecs, 10, 3);
    return out;
}

ZString ZLog::getThread(){
    ZString thread = ZThreadA::thisTid();
    zu64 id;
    bool found = false;
    for(zu64 i = 0; i < thread_ids.size(); ++i){
        if(thread_ids[i] == thread){
            id = i;
            found = true;
            break;
        }
    }
    if(!found){
        id = thread_ids.size();
        thread_ids.push(thread);
    }
    return ZString(id);
}

ZString ZLog::genLogFileName(ZString prefix){
    time_t raw;
    char buffer[20];
    time(&raw);
    strftime(buffer, 20, "%m-%d-%y_%H-%M-%S", localtime(&raw));
    ZString out(prefix);
    out << buffer << ".log";
    return out;
}

void ZLog::init(){
    if(!_init){
        _init = true;
        worker.run();
    }
}
void ZLog::init(ZPath dlgfl, ZString format){
    if(!_init){
        init();
        addLogFile(dlgfl, ZLogSource::all, format);
    }
}

void ZLog::formatStdout(zlog_source type, ZString fmt){
    ZLogWorker::formatStdout(type, fmt);
}
void ZLog::formatStderr(zlog_source type, ZString fmt){
    ZLogWorker::formatStderr(type, fmt);
}
void ZLog::addLogFile(ZPath pth, zlog_source type, ZString fmt){
    ZLogWorker::addLogFile(pth, type, fmt);
}

void ZLog::waitEnd(){
    worker.waitEnd();
}

}
