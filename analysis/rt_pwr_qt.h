#ifndef RT_PWR_QT
#define RT_PWR_QT

#include "base\rt_node.h"
#include "analysis\rt_filter_te.h"

/*! \brief final assembly of rt_node and template - floating point version*/
class rt_pwr_fp : virtual public rt_node {

private:
    t_rt_pwr_te<double> worker;

public:
    rt_pwr_fp(QObject *parent = NULL):
        rt_node(parent),
        worker()
    {
        init(&worker); //connect real objecr with abstract pointer
    }

    virtual ~rt_pwr_fp(){;}
};

#endif // RT_PWR_QT

