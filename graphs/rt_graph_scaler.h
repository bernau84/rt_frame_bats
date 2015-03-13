#ifndef RT_GRAPH_SCALER_H
#define RT_GRAPH_SCALER_H

#include "base/rt_base.h"
#include "base/rt_multibuffer.h"
#include "graphs/rt_graph.h"


enum e_axis_coord {

    EAXIS_X = 0,
    EAXIS_Y,
    EAXIS_Z,
    EAXIS_NUMB
};

typedef t_multibuffer<QVector<float>, 2> t_mvectorbuffer;

class rt_autoscale : public t_rt_empty, t_mvectorbuffer {

    /*! \todo
     * - trivial rt object counting axis autoscale & parts optimal + axis labeling
     * - configuration import
     * - autoscale / axis base handling
     * - export text and coord
     */

private:

    double refresh;
    double last_refresh;
    e_axis_coord no;

    struct { //for automatic granuity purpose

        float mean_min;
        float mean_max;
        float round_min;
        float round_max;
    } statistic;


    /*! \brief defines grain of axis subparts */
    enum e_graph_base {
        INTEGER = 0,    //ie frequency band number, sample order
        REAL,       //ie time axis
        LOG2,       //octave/third octave frequency
        LOG10,       //dB scale, frequency in decades
        BASE_N
    } base;


    int mm_trace(float u, float rc){

        //fast autoscale to longer axis
        if(statistic.mean_max < u){

            statistic.mean_max = u;
            goto on_update;
        }

        if(statistic.mean_min > u){

            statistic.mean_min = u;
            goto on_update;
        }

        //slow autoscale to shorter axis
        float mean = (statistic.mean_max - statistic.mean_min) / 2;
        if(u < mean){

            statistic.mean_min += (u - statistic.mean_min) / rc;
        } else {

            statistic.mean_max += (u - statistic.mean_max) / rc;
        }

        double ltime = sta.nn_run / *sta.fs_in;
        if(ltime < (last_refresh + rc))
            return 0;

on_update:
        last_refresh = ltime;
        return 1;
    }

    void mm_round(){

        //rozdil mezi min a max zaokrouhlime nahoru
        //tak ze budou zaokrouhleny i hodnoty pro min a max

        //zaokrouhlujem mantisu a exponent podle zvolene baze

        //do nothing for now
        return;
    }

public slots:
    void start(){
        /*! autoscale on */
        i_rt_base::start();
    }

    void pause(){
        /*! autroscale off, freeze actual scale */
        i_rt_base::pause();
    }

    void resume(){
        /*! autroscale off, freeze actual scale */
        i_rt_base::resume();
    }

    void process(){

        /*! recalculate scale */
        t_3dcircbuf *source = dynamic_cast<t_3dcircbuf *>pre;
        if(!source) return;

        if(sta.state != t_rt_status::ActiveState){

            int n_dummy = source->readSpace(rd_i); //throw data
            source->readShift(n_dummy, rd_i);
            return;
        }

        double rc = 2 / (*sta.fs_in * refresh);
        int nv = source->readSpace(rd_i);

        while(nv > 0){ //read out

            t_rt_graph_v v = source->read(rd_i);
            if(mm_trace(v, rc)){ //averaging of min & max

                mm_round();  //min round-down, max round-up

                QVector<float> d;
                d << statistic.mean_min << statistic.mean_max;
                t_multibuffer::write(d);

                emit on_update();
            }

            nv -= 1;
        }
    }

    void change(){

        refresh = set["Dynamic"].get().toDouble();
        base = set["Base"].get().toInt();

        pause();

        sta.fs_out = refresh;  //approximately (slowest)
        last_refresh = 0;

        emit on_change(); //poslem dal
        resume();
    }

    rt_autoscale(e_axis_coord _no):
        no(_no),
        t_mvectorbuffer(2)  //actual and previous records
    {

    }
};

//todle tedy neni promyslene - mohu zaokrouhlovat
//prepocitane udaje ale nejam bych mel ulozit i
//puvodni hodnoty...melo by se resit jinak - prepocitavacim blokem
//mezi analyzou a grafem, ktery bude zpracovatat vsechna data
//    /*! \brief preprocces on data input */
//    enum e_graph_unit_op {
//        SHIFT = 0,
//        DB,
//        SPL,
//        POWER,
//        UNIT_N
//    } postprocf;

//    float mm_postproc(float v){  //unit conversion

//        switch(postprocf){
//            case SHIFT:
//            case DB:
//            case SPL:
//            default:
//                return v;  //do nothong for now
//        }
//    }

#endif // RT_GRAPH_SCALER_H
