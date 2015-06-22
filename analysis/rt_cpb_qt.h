#ifndef RT_CPB_QT_H
#define RT_CPB_QT_H

#include "base\rt_node.h"
#include "analysis\rt_cpb_te.h"

/*! \brief final assembly of rt_node and template of cpb - floating point version*/
class rt_cpb_fp : virtual public rt_node {

private:
    t_rt_cpb_te<double> worker;

public:
    rt_cpb_fp(QObject *parent = NULL):
        rt_node(parent),
        worker()
    {
        init(&worker); //connect real objecr with abstract pointer
    }

    virtual ~rt_cpb_fp(){;}
};

#endif //RT_CPB_QT_H
