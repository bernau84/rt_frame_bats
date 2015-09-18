#ifndef RT_DETECTOR_QT
#define RT_DETECTOR_QT

#include "base\rt_base_slbuf_ex.h"
#include "rt_filter_qt.h"
#include "rt_pwr_qt.h"

template <typename T> class t_rt_detector_te : public virtual i_rt_base_slbuf_ex<T>
{
private:
    i_rt_base_slbuf_ex<T> &m_sys;  //unconnected system pro process input, result will trigger original input data

    t_rt_slice<T> row;  /*! active line */

    int M;  //time slice lenght

    int m_time_holdon;     //
    int m_time_holdoff;     //time hold on
    int m_threshold_on;     //
    int m_threshold_off;     //

    bool m_state;  //true - updating
    double m_timestamp;    //of last change of state

public:
    virtual void update(t_rt_slice<T> &smp);  /*! \brief data to analyse/process */
    virtual void change();  /*! \brief someone changed setup or input signal property (sampling frequency for example) */

    t_rt_detector_te(i_rt_base<T> *sys, const QDir resource = QDir(":/config/js_config_detector.txt"));

    virtual ~t_rt_detector_te(){;}
};

/*! \brief constructor creates and initialize digital filters from predefined resource file configuration */
template<typename T> t_rt_detector_te<T>::t_rt_detector_te(i_rt_base<T> &sys, const QDir resource):
    i_rt_base_slbuf_ex<T>(resource),
    m_sys(sys)
{
   m_time_holdon = m_time_holdoff = m_threshold_on = m_threshold_off = 0;
   m_state = false;
   //finish initialization of filters and buffer

   /*! \todo v sys vynutit aby byla delka slice identicka se vstupem */
   change();
}

/*! \brief implement cpb algortihm, assumes real time feeding
 * \param assumes the same type as internal buf is! */
template <typename T> void t_rt_detector_te<T>::update(t_rt_slice<T> &smp){

    m_sys.update(smp);
    if(0 == m_sys.readSpace())
        return;

    const t_rt_slice<T> *pwr = (t_rt_slice<T> *)(m_sys.read());
    if(pwr == NULL) return;

    if(smp.A.size() != pwr->A.size())
        return;     //necessery condition

    for(unsigned i=0; i<pwr->A.size(); i++){

        double t = smp.t + i/(2*smp.I[i]); //I[x] ~ freq. resol of sample = nyquist fr = fs/2

        if(row.A.size() == 0){ //

            row = (M) ? t_rt_slice<T>(t, M, (T)0) :   //version with fixed known slice size
                        t_rt_slice<T>(t, smp.A.size(), (T)0);  //auto-slice size (derived from input with respect to decimation)
        }

        if((m_state) && ((t - m_timestamp) > m_time_holdon))  //m_time_holdon quarding time
            if(pwr->A[i] <= m_threshold_off){

                m_state = false;
                m_timestamp = t;
            }

        if((!m_state) && ((t - m_timestamp) > m_time_holdoff)) //m_time_holdoff quarding time
            if(pwr->A[i] >= m_threshold_on){

                m_state = true;
                m_timestamp = t;
            }

        if(m_state) row.append(smp->A[i], smp->I[i]); //valid sample for forwarding
            else if(false == row.isempty()) row.append(T(0), smp->I[i]);  //fill zeros to end if was already something written

        if(row.isfull()){  //in auto mode (M == 0) with last sample of input slice

            buf->write(&row); //write
            row.A.clear(); //force new row initializacion
        }
    }
}

/*! \brief refresh analysis setting and resising internal buffer
 * \warning is not thread safety, caller must ensure that all pointers and interface of old
 *          buffer are unused
 */
template <typename T> void t_rt_detector_te<T>::change(){

    int n = par["Multibuffer"].get().toInt();
    M = par["Slice"].get().toInt();

    if(buf) delete(buf);
    buf = (rt_idf_circ_simo<t_rt_slice<T> > *) new rt_idf_circ_simo<t_rt_slice<T> >(n);

    m_time_holdon = par["HoldOn"].get().toInt();
    m_time_holdoff = par["HoldOff"].get().toInt();
    m_threshold_on = par["ThresholdOn"].get().toInt();
    m_threshold_off = par["ThresholdOff"].get().toInt();

    m_state = false;
}


/*! \brief final assembly of rt_node and template - floating point version*/
class rt_detector_filter_pwr_fp : virtual public rt_node {

private:
    t_rt_filter_pwr_te<double> fpwr;
    t_rt_detector_te<double> worker;

public:
    rt_filter_fp(QObject *parent = NULL):
        rt_node(parent),
        worker()
    {
        init(&worker); //connect real objecr with abstract pointer
    }

    virtual ~rt_filter_fp(){;}
};

#endif // RT_DETECTOR_QT

