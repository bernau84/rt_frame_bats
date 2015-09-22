#ifndef RT_FILTER_QT
#define RT_FILTER_QT

#include "base\rt_base_slbuf_ex.h"
#include "base\rt_node.h"
#include "freq_analysis.h"
#include "freq_filtering.h"

template <typename T> class t_rt_filter_te : public virtual i_rt_base_slbuf_ex<T>
{
private:
    t_rt_slice<T> row;  /*! active line */
    t_pFilter<T> *sys;  /*! general digital filter */

    int D;  //decimation factor
    int M;  //slice size

    using i_rt_base_slbuf_ex<T>::par;
    using i_rt_base_slbuf_ex<T>::buf_resize;
    using i_rt_base_slbuf_ex<T>::buf_append;

public:

    virtual void update(t_rt_slice<T> &smp);  /*! \brief data to analyse/process */
    virtual void change();  /*! \brief someone changed setup or input signal property (sampling frequency for example) */

    /*! \brief constructor creates and initialize digital filters from predefined resource file configuration */
    t_rt_filter_te( const std::string &conf):
        i_rt_base(conf, ":/config/js_config_filteranalysis.txt", RT_QUEUED),  //!!because i_rt_base is virtual base class, constructor has to be defined here!!
        i_rt_base_slbuf_ex<T>(conf, ":/config/js_config_filteranalysis.txt"),
        row(0, 0),
        sys(NULL)
    {
       //finish initialization of filters and buffer
       change();
    }

    virtual ~t_rt_filter_te(){

       if(sys) delete sys;
       sys = NULL;
    }
};


/*! \brief implement cpb algortihm, assumes real time feeding
 * \param assumes the same type as internal buf is! */
template <typename T> void t_rt_filter_te<T>::update(t_rt_slice<T> &smp){

    for(unsigned i=0; i<smp.A.size(); i++){

        if(row.A.size() == 0){ //

            double t = smp.t + i/(2*smp.I[i]); //I[x] ~ freq. resol of sample = nyquist fr = fs/2

            row = (M) ? t_rt_slice<T>(t, M, (T)0) :   //version with fixed known slice size
                        t_rt_slice<T>(t, smp.A.size() / D, (T)0);  //auto-slice size (derived from input with respect to decimation)
        }

        int32_t n_2_output;
        T res = sys->process(smp.A[i], &n_2_output);
        if(0 == n_2_output){

            row.append(res, smp.I[i]/D); //D can reduce freq. resolution because of change fs
            if(row.isfull()){  //in auto mode (M == 0) with last sample of input slice

                buf_append(row); //write + send notifications
                row.A.clear(); //force new row initializacion
           }
        }
    }
}

/*! \brief refresh analysis setting and resising internal buffer
 * \warning is not thread safety, caller must ensure that all pointers and interface of old
 *          buffer are unused
 */
template <typename T> void t_rt_filter_te<T>::change(){


    /*! \todo - memni se mi trab jen rozmer multibuferu a kvuli tomu budu
     * mazat filtry?! pokud nechci budu si muset predchozi volbu nekde pamatovat ->
     * mit fi jako member promennou
     */

    buf_resize(par["Multibuffer"].get().toInt());
    QJsonValue fi = par["Filter"].get();
    D = par["Decimation"].get().toInt();
    M = par["Slice"].get().toInt();

    if(!fi.isArray())
        return;

    int N = 0;
    QJsonArray num_ar = fi.toArray().at(1).toArray();
    T num[num_ar.size()];  //json array to c-style array
    foreach(QJsonValue vv, num_ar)
        if(vv.isDouble()) num[N++] = (T)vv.toDouble();

    N = 0;
    QJsonArray den_ar = fi.toArray().at(2).toArray();
    T den[den_ar.size()];     //json array to c-style array
    foreach(QJsonValue vv, den_ar)
        if(vv.isDouble()) den[N++] = (T)vv.toDouble();

    QString fi_prop = fi.toArray().at(0).toString();
    if(0 == fi_prop.compare("FIR_DIRECT1")) {

        if(sys) delete sys;
        sys = (t_pFilter<T> *) new t_DirectFilter<T>(num, num_ar.size(), D);
    } else if(0 == fi_prop.compare("IIR_BIQUAD")) {

        if(sys) delete sys;
        sys = (t_pFilter<T> *) new t_BiQuadFilter<T>(num, den, num_ar.size(), D);
    }
}

/*! \brief final assembly of rt_node and template - floating point version*/
class rt_filter_fp : virtual public rt_node {

private:
    t_rt_filter_te<double> worker;

public:
    rt_filter_fp(QObject *parent = NULL, const QString &config = QString()):
        rt_node(parent),
        worker(config.toStdString())
    {
        init(&worker); //connect real objecr with abstract pointer
    }

    virtual ~rt_filter_fp(){;}
};

#endif // RT_FILTER_QT

