#ifndef RT_TRACING
#define RT_TRACING

#include <ostream>
#include <iostream>
#include <time.h>
#include <vector>

enum e_rt_trace {

    TR_LEVEL_0 = 0, //OFF
    TR_LEVEL_ERRORS,
    TR_LEVEL_WARNINGS,
    TR_LEVEL_ALL
};

class t_rt_tracer
{
private:
    std::ostream &out;
    std::vector<clock_t> timers;
    const char *idnstr;
    e_rt_trace level;

public:
    t_rt_tracer(std::ostream &dest = std::cerr):
        out(dest),
        level(TR_LEVEL_0)
    {
        /*! \todo locks
         * default name from static counter of trace instances
         */
        idnstr = "unamed";
    }

    ~t_rt_tracer(){

    }

    void enable(e_rt_trace rise = TR_LEVEL_ERRORS, const char *name = ""){

        level = rise;
        idnstr = name;
    }

    void disable(){

        level = TR_LEVEL_0;
    }

    void put(std::string &trace, e_rt_trace gr = TR_LEVEL_ERRORS){

        if(gr <= level){

            clock_t t = clock();
            char line[128]; snprintf(line, sizeof(line), "%f> %s - %s\r\n", ((float)t)/CLOCKS_PER_SEC, idnstr, trace.data());
            out << line;
        }
    }

    void put(const char *trace, e_rt_trace gr = TR_LEVEL_ERRORS){

        std::string sline(trace);
        put(sline, gr);
    }

    std::ostream &operator<<(std::string &trace){

        this->put(trace, TR_LEVEL_0);
        return out;
    }

    unsigned start_meas(){

        timers.push_back(clock());
        int id = timers.size() - 1;
        char line[128]; snprintf(line, sizeof(line), "stopwatch[%d] start", id);

        std::string sline(line);
        put(sline, TR_LEVEL_ALL);
        return id;
    }

    void end_meas(unsigned id = 0){

        if(timers.size() <= id)
            return;

        clock_t t = timers[id] - clock();
        char line[64]; snprintf(line, sizeof(line), "stopwatch[%d] stoped, duration %f", id, ((float)t)/CLOCKS_PER_SEC);

        std::string sline(line);
        put(sline, TR_LEVEL_ALL);
        timers.erase(timers.begin() + id);
    }
};

#endif // RT_TRACING

