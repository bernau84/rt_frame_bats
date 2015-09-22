#ifndef RT_DETECTOR_QT
#define RT_DETECTOR_QT

#include "base\rt_base_slbuf_ex.h"
#include "rt_filter_qt.h"
#include "rt_pwr_qt.h"

/*! \brief time & value condition checker
 *  if values are inside limit tanh for given time signals true
 *  many options for tuning - min/max/mean statistic for reference of signal and triggering
 */

template <typename T> class t_rt_detector_te : public virtual i_rt_base_slbuf_ex<T>
{
private:
    i_rt_base_slbuf_ex<T> &m_preproc;  //unconnected system pro process input, result will trigger original input data

    t_rt_slice<T> row;  // active line

    int M;  //time slice lenght
    int TN; //last averaraging constant for staticstics (time * fs)

    double m_time_holdon;     //time hold on []
    double m_time_holdoff;     //
    T m_threshold_on;     //
    T m_threshold_off;     //

    bool m_state;  //true - updating
    double m_timestamp;    //of last change of state
    double m_timeaver;      //averaging time for min/mean/max statistics

    enum e_refenece_type {

      RT_DET_ABS,
      RT_DET_MIN,
      RT_DET_MEAN,
      RT_DET_MAX
    } m_reference;

    t_AveragingFilter sta_min;
    t_AveragingFilter sta_mean;
    t_AveragingFilter sta_max;

    using i_rt_base_slbuf_ex<T>::par;
    using i_rt_base_slbuf_ex<T>::buf_resize;
    using i_rt_base_slbuf_ex<T>::buf_append;

public:
    virtual void update(t_rt_slice<T> &smp);  /*! \brief data to analyse/process */
    virtual void change();  /*! \brief someone changed setup or input signal property (sampling frequency for example) */

    /*! \brief constructor creates and initialize digital filters from predefined resource file configuration */
    t_rt_detector_te(i_rt_base_slbuf_ex<T> &sys, const std::string &conf):
        i_rt_base(conf, ":/config/js_config_detector.txt", RT_QUEUED),  //!!because i_rt_base is virtual base class, constructor has to be defined here!!
        i_rt_base_slbuf_ex<T>(conf, ":/config/js_config_detector.txt"),
        m_preproc(sys),
        sta_min(1),
        sta_max(1),
        sta_mean(1)
    {
       TN = 1;
       change();       //finish initialization of filters and buffer
    }

    virtual ~t_rt_detector_te(){;}
};


/*! \brief implement cpb algortihm, assumes real time feeding
 * \param assumes the same type as internal buf is! */
template <typename T> void t_rt_detector_te<T>::update(t_rt_slice<T> &smp){

    m_preproc.update(smp);
    if(0 == m_preproc.readSpace())
        return;

    const t_rt_slice<T> *pwr = (t_rt_slice<T> *)(m_preproc.read());
    if(pwr == NULL) return;

    if(smp.A.size() != pwr->A.size())
        return;     //necessery condition

    for(unsigned i=0; i<pwr->A.size(); i++){

        double t = smp.t + i/(2*smp.I[i]); //I[x] ~ freq. resol of sample = nyquist fr = fs/2

        if(row.A.size() == 0){ //

            row = (M) ? t_rt_slice<T>(t, M, (T)0) :   //version with fixed known slice size
                        t_rt_slice<T>(t, smp.A.size(), (T)0);  //auto-slice size (derived from input with respect to decimation)
        }

        int cTN = 2*m_timeaver*smp.I[i];
        if(cTN <= 0) cTN = 1;

        if(TN != cTN){  //change of time property -> new averaging constant

            TN = cTN;
            sta_min.modify(TN);
            sta_mean.modify(TN);
            sta_max.modify(TN);
        }

        //normalization
        T val = smp->A[i];
        switch(m_reference){
            case RT_DET_MIN: val /= sta_mean.getLast(); break;
            case RT_DET_MAX: val /= sta_max.getLast(); break;
            case RT_DET_MEAN: val /= sta_mean.getLast(); break;
            default: break;
        }

        //averaging sthe statistic
        m_sta.mean->process(smp->A[i]);
        m_sta.min->process((smp->A[i] < sta_min.getLast()) ? smp->A[i] : sta_min.getLast());
        m_sta.min->process((smp->A[i] > sta_max.getLast()) ? smp->A[i] : sta_max.getLast());

        //value & time condition detector
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

            buf_append(&row); //write
            row.A.clear(); //force new row initializacion
        }
    }
}

/*! \brief refresh analysis setting and resising internal buffer
 * \warning is not thread safety, caller must ensure that all pointers and interface of old
 *          buffer are unused
 */
template <typename T> void t_rt_detector_te<T>::change(){

    buf_resize(par["Multibuffer"].get().toInt());

    M = par["Slice"].get().toInt();  /*! \todo idea - may act as pretrigger (defined as minus values of HoldOn)! */
    m_time_holdon = par["HoldOn"].get().toDouble();
    m_time_holdoff = par["HoldOff"].get().toDouble();
    m_threshold_on = T(par["ThresholdOn"].get().toDouble());
    m_threshold_off = T(par["ThresholdOff"].get().toDouble());

    m_timeaver = par["ReferenceT"].get().toDouble();
    QString s_meth = par["ReferenceV"].get().toString();

    if(s_meth.compare("ABS") == 0) m_reference = RT_DET_ABS;
    else if(s_meth.compare("MIN") == 0) m_reference = RT_DET_MIN;
    else if(s_meth.compare("MEAN") == 0) m_reference = RT_DET_MEAN;
    else if(s_meth.compare("MAX") == 0) m_reference = RT_DET_MAX;

    std::vector<T> zeros(0);
    if(sta_min) sta_min.reset(zeros, T(-1.0));
    if(sta_max) sta_max.reset(zeros, T(+1.0));
    if(sta_mean) sta_mean.reset(zeros, T(0.0));

    m_state = false;
}


/*! \brief final assembly of rt_node and template - floating point version*/
class rt_detector_fp : virtual public rt_node {

private:
    t_rt_detector_te<double> worker;

public:
    rt_detector_fp(QObject *parent = NULL, const QString &config = QString()):
        rt_node(parent),
        worker(config.toStdString())
    {
        init(&worker); //connect real objecr with abstract pointer
    }

    virtual ~rt_filter_fp(){;}
};

#endif // RT_DETECTOR_QT

