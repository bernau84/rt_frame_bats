#ifndef RT_PWR_TE
#define RT_PWR_TE

#include "base\rt_base.h"

template <typename T> class t_rt_pwr_te : public virtual i_rt_base
{
private:
    t_rt_slice<T> row;  /*! active line */
    rt_idf_circ_simo<t_rt_slice<T> > *buf;  /*! output buffer */

    int M;   //time slice length
    double T;  //averaging time
    enum e_pwr_method {
        IDENT,
        AC_ABS,
        AC_SQUARE,
        ABS,
        SQUARE
    } m_method;

    T m_dc;     //traced dc value
    T m_pwr;    //last value
    uint64_t m_count; //number of sample

public:
    virtual const void *read(int n){ return (buf) ? buf->read(n) : NULL; }
    virtual const void *get(int n){ return (buf) ? buf->get(n) : NULL;  }
    virtual int readSpace(int n){ return (buf) ? buf->readSpace(n) : -1; }

    virtual void update(const void *sample);  /*! \brief data to analyse/process */
    virtual void change();  /*! \brief someone changed setup or input signal property (sampling frequency for example) */

    t_rt_pwr_te(const QDir resource = QDir(":/config/js_config_filteranalysis.txt"));

    ~t_rt_pwr_te(){
    }
};

/*! \brief constructor creates and initialize digital filters from predefined resource file configuration */
template<typename T> t_rt_filter_te<T>::t_rt_filter_te(const QDir resource):
    i_rt_base(resource),
    row(0, 0),
    buf(NULL),
    m_dc(0),
    m_pwr(0),
    m_count(0)
{
   //finish initialization of filters and buffer
   change();
}

/*! \brief implement cpb algortihm, assumes real time feeding
 * \param assumes the same type as internal buf is! */
template <typename T> void t_rt_filter_te<T>::update(const void *sample){

    //convert & test sample; we uses pointer instead of references because reference can be tested
    //only with slow try catch block which would be cover all processing loop
    const t_rt_slice<T> *smp = (t_rt_slice<T> *)(sample);
    if(smp == NULL) return;

    for(unsigned i=0; i<smp->A.size(); i++){

        double t = smp->t + i/(2*smp->I[i]); //I[x] ~ freq. resol of sample = nyquist fr = fs/2

        if(row.A.size() == 0){ //

            row = (M) ? t_rt_slice<T>(t, M, (T)0) :   //version with fixed known slice size
                        t_rt_slice<T>(t, smp->A.size(), (T)0);  //auto-slice size (derived from input with respect to decimation)
        }

        T pwr = smp->A[i];

        switch(m_method){

            case AC_ABS:
            case AC_SQUARE:
                m_dc += (pwr > m_dc) ? +1 : -1; //slon notch nonlinedar dc removal - lower precision 1 bit minus
            break;
        }

        pwr -= m_dc;

        switch(m_method){

            case AC_ABS:
            case ABS:
                pwr = (pwr > 0) ? prc : -pwr;
            break;

            case AC_SQUARE:
            case SQUARE:
                pwr = pwr * pwr;
            break;
        }

        //averaging
        if(t > T){ //exponential

            m_pwr += (pwr - m_pwr) / (T * smp->I[0]); //assumes constat fs
        } else { //linear

            m_count += 1;
            m_pwr = m_pwr * (m_count - 1)/m_count + pwr / m_count;
        }

        row.append(m_pwr, smp->I[i]);
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
template <typename T> void t_rt_filter_te<T>::change(){

    M = par["Slice"].get().toInt();

    int n = par["Multibuffer"].get().toInt();
    if(buf) delete(buf);
    buf = (rt_idf_circ_simo<t_rt_slice<T> > *) new rt_idf_circ_simo<t_rt_slice<T> >(n);

    QString meth = par["Method"].get().toString();
    if(meth.compare("IDENT") == 0) m_method = IDENT;
    else if(meth.compare("AC_ABS") == 0) m_method = AC_ABS;
    else if(meth.compare("AC_SQUARE") == 0) m_method = AC_SQUARE;
    else if(meth.compare("ABS") == 0) m_method = ABS;
    else if(meth.compare("SQUARE") == 0) m_method = SQUARE;

    m_count = 0;
    m_dc = 0;
    m_pwr = 0;
}

#endif // RT_PWR_TE

