#ifndef RT_WEIGHTED_PWR_QT
#define RT_WEIGHTED_PWR_QT

#include "base\rt_base_slbuf_ex.h"
#include "base\rt_node.h"
#include "rt_filter_qt.h"
#include "rt_pwr_qt.h"

/*! \class t_rt_w_pwr_te - template of weighted power measurement
 * \brief according to filter design cam realize spl computation or frequency band eveluation
 */
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
    t_rt_w_pwr_te(const QDir filter_res = QDir(":/config/js_config_w_pwr_filter.txt"),
                  const QDir power_res = QDir(":/config/js_config_w_pwr_calc.txt")):
        i_rt_base(QDir(), RT_QUEUED),
        filter(filter_res),
        power(power_res)
    {
       //finish initialization of filters and buffer
       change();
       trc.enable(TR_LEVEL_ALL, "w_pwr");
       //filter.par["Filter"].sel("fir-half-hp-N160"); /*! \todo - put to main */
    }

    virtual ~t_rt_w_pwr_te(){;}
};

/*! \brief cascade callibg filter and power calculation
 *  uses direct call without callback or signal mechanism - both
 * parts are unconnected */
template <typename T> void t_rt_w_pwr_te<T>::update(const void *sample){

    trc.start_meas();

    filter.on_update(sample); //simplyfied - better is read all from source buffer
    while(filter.readSpace(0))
        power.on_update(filter.read(0));

    while(power.readSpace(0))
        signal(RT_SIG_SOURCE_UPDATED, power.read(0));

    trc.end_meas();
}

/*! \brief refresh analysis setting and resising internal buffer
 * \warning is not thread safety, caller must ensure that all pointers and interface of old
 *          buffer are unused
 */
template <typename T> void t_rt_w_pwr_te<T>::change(){

    filter.change();
    power.change();
}

/*! \class rt_w_pwr_fp
 * \brief final assembly of rt_node and template - floating point version
 */
class rt_w_pwr_fp : virtual public rt_node {

private:
    t_rt_w_pwr_te<double> worker;

public:
    rt_w_pwr_fp(QObject *parent = NULL):
        rt_node(parent),
        worker()
    {
        init(&worker); //connect real objecr with abstract pointer
    }

    virtual ~rt_w_pwr_fp(){;}
};

#endif // RT_WEIGHTED_PWR_QT

