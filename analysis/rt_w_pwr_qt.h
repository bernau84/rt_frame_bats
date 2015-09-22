#ifndef RT_WEIGHTED_PWR_QT
#define RT_WEIGHTED_PWR_QT

#include "base\rt_base_slbuf_ex.h"
#include "base\rt_node.h"
#include "rt_filter_qt.h"
#include "rt_pwr_qt.h"


template <typename T> class t_rt_w_pwr_te : public virtual i_rt_base
{
private:
    t_rt_filter_te<T> filter;
    t_rt_pwr_te<T> power;

public:
    virtual const void *read(int n){ return power.read(n); }
    virtual const void *get(int n){ return power.get(n);  }
    virtual int readSpace(int n){ return power.readSpace(n); }

    virtual void update(const void *sample);  /*! \brief data to analyse/process */
    virtual void change();  /*! \brief someone changed setup or input signal property (sampling frequency for example) */

    /*! \brief constructor creates and initialize digital filters from predefined resource file configuration */
    t_rt_w_pwr_te(const std::string &conf_filt, const std::string &conf_pwr):
        i_rt_base("", RT_QUEUED),
        filter(conf_filt, ":/config/js_config_w_pwr_filter.txt"),
        power(conf_pwr, ":/config/js_config_w_pwr_power.txt")
    {
       //finish initialization of filters and buffer
       change();
    }

    virtual ~t_rt_w_pwr_te(){;}
};

/*! \class rt_pwr_fp
 * \brief final assembly of rt_node and template - floating point version*/
class rt_w_pwr_fp : virtual public rt_node {

private:
    t_rt_pwr_te<double> worker;

public:
    rt_pwr_fp(QObject *parent = NULL, const QString &conf_filter = QString(), , const QString &conf_pwr = QString()):
        rt_node(parent),
        worker(conf_filter.toStdString(), conf_pwr.toStdString())
    {
        init(&worker); //connect real objecr with abstract pointer
    }

    virtual ~rt_pwr_fp(){;}
};

#endif // RT_WEIGHTED_PWR_QT

