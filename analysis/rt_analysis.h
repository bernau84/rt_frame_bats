#ifndef RT_ANALYSIS_H
#define RT_ANALYSIS_H


#include "base\rt_node.h"
#include "analysis\rt_cpb.h"
//#include "analysis\rt_shift.h"

/*! \brief final assembly of rt_node and template of cpb */

class rt_cpb_fp : virtual public rt_node,
            virtual public t_rt_cpb_te<double>{

public:
    rt_cpb_fp(QObject *parent, const QDir &resource = QDir(":/config/js_config_cpb")):
        rt_node(parent),
        t_rt_cpb_te<double>(parent, resource)
    {

    }

    virtual ~rt_cpb_fp(){;}

};

/*! \brief final assembly of rt_node and template of shift */

//class rt_shift_fp : virtual public rt_node,
//            virtual public t_rt_shift_te<double>{

//public:
//    rt_shift_fp(QObject *parent, const QDir &resource = QDir(":/config/js_config_freqshift.txt")):
//        rt_node(parent),
//        t_rt_shift_te<double>(parent, resource)
//    {

//    }

//    virtual ~rt_shift_fp(){;}
//};

#endif // RT_ANALYSIS_H
