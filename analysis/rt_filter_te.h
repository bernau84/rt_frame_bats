#ifndef RT_FILTER_TE_H
#define RT_FILTER_TE_H

#include "base\rt_base.h"
#include "freq_analysis.h"
#include "freq_filtering.h"

template <typename T> class t_rt_filter_te : public virtual i_rt_base
{
private:
    t_rt_slice<T> row;  /*! active line */
    t_pFilter<T> *sys;  /*! general digital filter */
    rt_idf_circ_simo<t_rt_slice<T> > *buf;  /*! output buffer */

    int D;  //decimation factor
    int M;  //slice size
public:
    virtual const void *read(int n){ return (buf) ? buf->read(n) : NULL; }
    virtual const void *get(int n){ return (buf) ? buf->get(n) : NULL;  }
    virtual int readSpace(int n){ return (buf) ? buf->readSpace(n) : -1; }

    virtual void update(const void *sample);  /*! \brief data to analyse/process */
    virtual void change();  /*! \brief someone changed setup or input signal property (sampling frequency for example) */

    t_rt_filter_te(const QDir resource = QDir(":/config/js_config_filteranalysis.txt"));

    ~t_rt_filter_te(){

       if(sys) delete sys;
    }
};

/*! \brief constructor creates and initialize digital filters from predefined resource file configuration */
template<typename T> t_rt_filter_te<T>::t_rt_filter_te(const QDir resource):
    i_rt_base(resource),
    row(0, 0),
    sys(NULL),
    buf(NULL)
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

        if(row.A.size() == 0){ //

            double t = smp->t + i/(2*smp->I[i]); //I[x] ~ freq. resol of sample = nyquist fr = fs/2

            row = (M) ? t_rt_slice<T>(t, M, (T)0) :   //version with fixed known slice size
                        t_rt_slice<T>(t, smp->A.size() / D, (T)0);  //auto-slice size (derived from input with respect to decimation)
        }

        int32_t n_2_output;
        T res = sys->process(smp->A[i], &n_2_output);
        if(0 == n_2_output){

            row.append(res, smp->I[i]/D); //D can reduce freq. resolution because of change fs
            if(row.isfull()){  //in auto mode (M == 0) with last sample of input slice

                buf->write(&row); //write
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

    int n = par["Multibuffer"].get().toInt();
    QJsonValue fi = par["Filter"].get();

    D = par["Decimation"].get().toInt();
    M = par["Slice"].get().toInt();

    if(buf) delete(buf);
    buf = (rt_idf_circ_simo<t_rt_slice<T> > *) new rt_idf_circ_simo<t_rt_slice<T> >(n);

    /*! \todo - memni se mi trab jen rozmer multibuferu a kvuli tomu budu
     * mazat filtry?! pokud nechci budu si muset predchozi volbu nekde pamatovat ->
     * mit fi jako member promennou
     */
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

#endif // RT_FILTER_TE_H

/*
 *    //vycist jako pole a prevest na double resp typ T a takto jak to je to nepude
   par["Numerators"].db("__val", decif, sizeof(nu)/sizeof(nu[0]));
   par["Denumerators"].db("__val", decif, sizeof(de)/sizeof(de[0]));
   int D = par["Decimation"].get().toInt();

   //sel by vybrat filter jako trida - aby mel vse jak typ tak decimaci tak koeficienty predvyplnene...

   //par["Multibuffer"];

   switch((t_rt_slice<T>::e_filter_struct)par["Type"].get().toInt()){

        case t_rt_slice<T>::IIR_DIRECT1: break;
        case t_rt_slice<T>::IIR_DIRECT2: break;
        case t_rt_slice<T>::IIR_LATTICE: break;
        case t_rt_slice<T>::IIR_BIQUADR: break;
        case t_rt_slice<T>::FIR_DIRECT1: break;
        case t_rt_slice<T>::FIR_LATTICE: break;
   }
*/