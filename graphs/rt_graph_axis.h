#ifndef RT_BASE_GRAPH_H
#define RT_BASE_GRAPH_H

#include "base\t_rt_base.h"

class rt_window;

class rt_autoscale : public t_rt_base {

    /*! \todo
     * - trivial rt object counting axis autoscale & parts optimal + axis labeling
     * - configuration import
     * - autoscale / axis base handling
     * - export text and coord
     */
    double refresh;

    /*! \brief defines grain of axis subparts */
    enum e_graph_base {

        INTEGER = 0,    //ie frequency band number, sample order
        REAL,       //ie time axis
        LOG2,       //octave/third octave frequency
        LOG10,       //dB scale, frequency in decades
        BASE_N
    } base;

    /*! \brief preprocces on data input */
    enum e_graph_unit_op {
        SHIFT = 0,
        DB,
        SPL,
        POWER,
        UNIT_N
    } postprocf;

    double mm_postproc(double){;}
    void mm_trace(double *min, double *max, v){;}
    void mm_round(double *min, double *max){;}

public slots:
    void start(){
        /*! autoscale on */
        t_rt_base::start();
    }

    void pause(){
        /*! autroscale off, freeze actual scale */
        t_rt_base::pause();
    }

    void process(){
        /*! recalculate scale */
        t_rt_base *pre = dynamic_cast<t_rt_base *>(parent());
        if(!pre) return;

        if(sta.state != t_rt_status::ActiveState){

            int n_dummy = pre->t_slcircbuf::readSpace(rd_i); //throw data
            pre->t_slcircbuf::readShift(n_dummy, rd_i);
            return;
        }

        t_rt_slice t_scl;   //actual scale [loscale, hiscale]
        t_slcircbuf::get(&t_scl, 1);  //read out
        double min = t_scl.v[0];
        double max = t_scl.v[1];
        double rc = 2 / (sta.fs_out * refresh);

        t_rt_slice t_amp;  //amplitudes
        while(pre->t_slcircbuf::read(&t_amp, rd_i)){ //read out

            for(int i=0; i<t_amp.size(); i++){

                double v = mm_postproc(t_amp.v[0]);
                mm_trace(&min, &max, v);
            }

            mm_round(&min, &max);

            /*! \todo - recount dividings for given axis base */

            t_slcircbuf::write(&t_scl, 1);  // out
        }
    }

    void change(){

        refresh = set["Dynamic"].get().toDouble();
        base = set["Base"].get().toInt();
        postprocf = set["RecountTo"].get().toInt();

        sta.fs_out = refresh;

        t_rt_status::t_rt_a_sta pre_sta = sta.state;

        pause();

        this->resize(pre->size(), false);  //follow previous

        emit on_change(); //poslem dal

        switch(pre_sta){

            case t_rt_status::SuspendedState:
            case t_rt_status::StoppedState:
                break;

            case t_rt_status::ActiveState:

                start();
                break;
        }
    }
};

class rt_graph_axis : public rt_autoscale {

private:
    double min_space;
    double max_space;

    rt_window *scene;
public:



public slots:
    /*! \brief on axis coord change
     *  enter relative coordination end point of axis
     */
    void on_process(QPoint coord){

        QPoint qdist = coord * coord;
        int dist = sqrt(qdist.manhattanLength());
        int min_n = dist / max_space;
        int max_n = dist / min_space;

        /*! \todo - optimum */
        for(int n = min_n; n<max_n; n++){
        }

    }

    void on_change(){

        min_space = set["SpaceLo"].get().toDouble();
        max_space = set["SpaceHi"].get().toDouble();
    }

    rt_graph_axis(rt_window *_scene){

        scene = _scene;
    }

    ~rt_graph_axis(){

    }
}

#endif // RT_BASE_GRAPH_H
