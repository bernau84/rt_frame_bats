#ifndef RT_BASE_GRAPH_H
#define RT_BASE_GRAPH_H

#include "base\t_rt_base.h"

enum e_axis_coord {

    EAXIS_X = 0,
    EAXIS_Y,
    EAXIS_Z,
    EAXIS_NUMB
};


class rt_window;

/* \todo - consider inherit rt_ogl_frame
 * to trace axis length naturaly */
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
