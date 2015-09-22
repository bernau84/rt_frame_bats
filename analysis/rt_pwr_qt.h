#ifndef RT_PWR_QT
#define RT_PWR_QT

#include "freq_filtering.h"
#include "base\rt_node.h"
#include "base\rt_base_slbuf_ex.h"

/*! \class t_rt_pwr_te template
 * \brief signal power/mean calc - norm and averaging time is optional
 */
template <typename T> class t_rt_pwr_te : public virtual i_rt_base_slbuf_ex<T>
{
private:
    t_rt_slice<T> row;  // active line
    t_AveragingFilter<T> avr;  //recirsive linaer->exponential averaging filter

    int     M;   //time slice length
    int     TN;  //averaging time in samples
    double  TA;  //config averagint time

    enum e_pwr_method {
        RT_PWR_IDENT,
        RT_PWR_AC_ABS,
        RT_PWR_AC_SQUARE,
        RT_PWR_ABS,
        RT_PWR_SQUARE
    } m_method;

    T m_dc;     //traced dc value
    T m_pwr;    //last value

    using i_rt_base_slbuf_ex<T>::par;
    using i_rt_base_slbuf_ex<T>::buf_resize;
    using i_rt_base_slbuf_ex<T>::buf_append;

public:
    virtual void update(t_rt_slice<T> &smp);  /*! \brief data to analyse/process */
    virtual void change();  /*! \brief someone changed setup or input signal property (sampling frequency for example) */

    /*! \brief constructor creates and initialize digital filters from predefined resource file configuration */
    t_rt_pwr_te(const QDir resource = QDir(":/config/js_config_filteranalysis.txt")):
        i_rt_base(resource, RT_QUEUED),  //!!because i_rt_base is virtual base class, constructor has to be defined here!!
        i_rt_base_slbuf_ex<T>(resource),
        row(0, 0),
        avr(0),
        m_dc(0),
        m_pwr(0)
    {
       //finish initialization of filters and buffer
       change();
    }

    virtual ~t_rt_pwr_te(){;}
};

/*! \brief  */
template <typename T> void t_rt_pwr_te<T>::update(t_rt_slice<T> &smp){

    for(unsigned i=0; i<smp.A.size(); i++){

        double t = smp.t + i/(2*smp.I[i]); //I[x] ~ freq. resol of sample = nyquist fr = fs/2

        if(row.A.size() == 0){ //

            row = (M) ? t_rt_slice<T>(t, M, (T)0) :   //version with fixed known slice size
                        t_rt_slice<T>(t, smp.A.size(), (T)0);  //auto-slice size (derived from input with respect to decimation)
        }

        int cTN = TA * smp.I[i];
        if(cTN <= 0) cTN = 1;

        if(cTN != TN) //time properties changed
            avr.modify((TN = cTN));

        T pwr = smp.A[i];

        if((m_method == RT_PWR_AC_ABS) || (m_method == RT_PWR_AC_SQUARE)){

            m_dc += (pwr > m_dc) ? +1 : -1; //slow notch nonlinedar dc removal - lower precision 1 bit minus
            pwr -= m_dc;
        }

        if((m_method == RT_PWR_AC_ABS) || (m_method == RT_PWR_ABS))
            pwr = (pwr > 0) ? pwr : -pwr;
        else if((m_method == RT_PWR_AC_SQUARE) || (m_method == RT_PWR_SQUARE))
            pwr = pwr * pwr;

        //average & save
        m_pwr = avr.process(pwr);
        row.append(m_pwr, smp.I[i]);

        if(row.isfull()){  //in auto mode (M == 0) with last sample of input slice

            buf_append(row); //write
            row.A.clear(); //force new row initializacion
       }
    }
}

/*! \brief  */
template <typename T> void t_rt_pwr_te<T>::change(){

    buf_resize(par["Multibuffer"].get().toInt());
    M = par["Slice"].get().toInt();
    TA = par["AveragingT"].get().toDouble();

    QString s_meth = par["Norm"].get().toString();
    if(s_meth.compare("ABS") == 0) m_method = RT_PWR_ABS;
    else if(s_meth.compare("SQUARE") == 0) m_method = RT_PWR_SQUARE;
    else if(s_meth.compare("AC_ABS") == 0) m_method = RT_PWR_AC_ABS;
    else if(s_meth.compare("AC_SQUARE") == 0) m_method = RT_PWR_AC_SQUARE;

    std::vector<T> zeros(0);
    avr.reset(zeros, 0);
}

/*! \class rt_pwr_fp
 * \brief final assembly of rt_node and template - floating point version*/
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

