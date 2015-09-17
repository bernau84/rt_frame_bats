#ifndef RT_FILTER_QT
#define RT_FILTER_QT

#include "base\rt_node.h"
#include "analysis\rt_filter_te.h"

/*! \brief final assembly of rt_node and template - floating point version*/
class rt_filter_fp : virtual public rt_node {

private:
    t_rt_filter_te<double> worker;

public:
    rt_filter_fp(QObject *parent = NULL):
        rt_node(parent),
        worker()
    {
        init(&worker); //connect real objecr with abstract pointer
    }

    virtual ~rt_filter_fp(){;}
};

#endif // RT_FILTER_QT

