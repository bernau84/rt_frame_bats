#ifndef RT_BASE_GRAPH_H
#define RT_BASE_GRAPH_H

#include "base\t_rt_base.h"

class rt_axis : public t_rt_base {

    /*! \todo
     * - trivial rt object counting axis autoscale & parts optimal + axis labeling
     * - configuration import
     * - autoscale / axis base handling
     * - export text and coord
     */
    double refresh;

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

                sta.nn_run += 1;
                sta.nn_tot += 1;

                if(min > t_amp) min = t_amp.v[i];
                    else if(max < t_amp) max = t_amp.v[i];
            }

            t_scl.v[0] += (t_scl.v[0] - min)*rc;  //floating average
            t_scl.v[1] += (t_scl.v[1] - max)*rc;

            /*! \todo - recount dividings for given axis base

            t_slcircbuf::write(&t_scl, 1);  // out
        }
    }

    void change(){

        refresh = set["Dynamic"].get().toDouble();

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

class rt_graph_axe : public rt_graph_object {



}

#endif // RT_BASE_GRAPH_H
