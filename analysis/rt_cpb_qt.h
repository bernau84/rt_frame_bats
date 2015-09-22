#ifndef RT_CPB_QT_H
#define RT_CPB_QT_H

#include "base\rt_base_slbuf_ex.h"
#include "base\rt_node.h"
#include "freq_analysis.h"
#include "freq_filtering.h"

#define RT_CPB_MAX_OCTAVES      20
#define RT_CPB_MAX_BANDSINOCTAVE    24
#define RT_CPB_MAX_BANDS (RT_CPB_MAX_OCTAVES*RT_CPB_MAX_BANDSINOCTAVE)

template <typename T> class t_rt_cpb_te : public virtual i_rt_base_slbuf_ex<T>
{
private:
    int gd;    /*! groupdelay - number od deci fir taps */
    int octn;  /*! number of octaves */
    int octm;  /*! number of bands in one octave */
    double refr; /*! refresh rate */

    t_rt_slice<T> cpb;                     /*! active spectrum */

    t_DiadicFilterBank<T, RT_CPB_MAX_OCTAVES> *bank;   /*! decimation fir filters branch */
    t_DelayLine<T> *dline[RT_CPB_MAX_OCTAVES]; /*! group delay compensation set of fir filters */

    t_pFilter<T> *aline[RT_CPB_MAX_BANDS]; /*! analytic iir filters branch - band pass */
    t_pFilter<T> *aver[RT_CPB_MAX_BANDS];  /*! output averaging iir */

public:
    virtual void update(t_rt_slice<T> &smp);  /*! \brief data to analyse/process */
    virtual void change();  /*! \brief someone changed setup or input signal property (sampling frequency for example) */

    t_rt_cpb_te(const QDir resource = QDir(":/config/js_config_cpbanalysis.txt"));
    virtual ~t_rt_cpb_te();
};

/*! \brief constructor creates and initialize digital filters from predefined resource file configuration */
template<typename T> t_rt_cpb_te<T>::t_rt_cpb_te(const QDir resource):
    i_rt_base_slbuf_ex<T>(resource),
    cpb(0, 0)
{
    double decif[1024], passf[1] = {1.0};

    gd = par["__oct_deci_directfir"].db("num", decif, sizeof(decif)/sizeof(decif[0]));
    t_DirectFilter<double> deci(decif, gd, 2);
    t_DirectFilter<double> pass(passf, 1);

    bank = new t_DiadicFilterBank<double, RT_CPB_MAX_OCTAVES>(pass, deci);  //naddimenzujem pro nejvyssi pocet oktav

    for(int i=0; i<RT_CPB_MAX_OCTAVES; i++) dline[i] = 0; //vynulujem
    for(int i=0; i<RT_CPB_MAX_BANDS; i++) aver[i] = aline[i] = 0;

    //finish initialization of filters and buffer
    change();
}

/*! \brief destructor free digital filter allocation */
template<typename T> t_rt_cpb_te<T>::~t_rt_cpb_te(){

    if(bank) delete bank;

    for(int i=0; i<RT_CPB_MAX_OCTAVES; dline[i++] = 0)
        if(dline[i]) delete dline[i];

    for(int i=0; i<RT_CPB_MAX_BANDS; i++){

        if(aline[i]) delete aline[i];
        if(aver[i]) delete aver[i];
    }
}

/*! \brief implement cpb algortihm, assumes real time feeding
 * \param assumes the same type as internal buf is! */
template <typename T> void t_rt_cpb_te<T>::update(t_rt_slice<T> &smp){

    for(unsigned i=0; i<smp.A.size(); i++){

        T lp_st[RT_CPB_MAX_OCTAVES]; //
        int n = bank->process(smp.A[i], lp_st);  //deci bank, update [0] a [n-th] octave band

        if(n >= octn) //omits octaves exceed given freq. range (computation powere is the same do decimation)
            continue;

        T d_st_0 = dline[0]->process(lp_st[0]);  //delay line for gd compenzation
        T d_st_n = dline[n]->process(lp_st[n]);

        for(int j=0; j<octm; j++){

            T a_st_0 = aline[j]->process(d_st_0); //cpb analytic iir
            T a_st_n = aline[n*octm + j]->process(d_st_n);

            Q_UNUSED(a_st_0);
            Q_UNUSED(a_st_n);

            //double f_0 = *sta.fs_in * (pow(2, -j/octm-n) - pow(2, -(j+1)/octm-n));
            //double f_n = *sta.fs_in * (pow(2, -j/octm) - pow(2, -(j+1)/octm));

            //upgrade & average curent spectrum
            cpb.A[j] = d_st_0;  //aver[j]->Process(a_st_0, 0);
            cpb.I[j] = j; //f_0
            cpb.A[n*octm + j] = d_st_n; //aver[n*octm + j]->Process(a_st_n, 0);
            cpb.I[n*octm + j] = octm*n+j; //f_n
        }

        double t = smp.t + i/(2*smp.I[i]); //I[x] ~ freq. resol of sample = nyquist fr = fs/2
        //average time reached - update buffer
        if((t - cpb.t) >= refr){ //spektrum je starsi nez refresh rate

            buf->write(&cpb); //store new specrum slice
            cpb = t_rt_slice<T>(t, octm*octn); //new spectrum init, time & number of spectral line
        }
    }
}

/*! \brief refresh analysis setting and resising internal buffer
 * \warning is not thread safety, caller must ensure that all pointers and interface of old
 *          buffer are unused
 */
template <typename T> void t_rt_cpb_te<T>::change(){

    octn = par["Octaves"].get().toDouble();  //aktualni pocet oktav
    octm = par["Bands"].get().toDouble();  //pocet pasem na oktavu
    refr = par["Time"].get().toDouble();  //vystupni frekvence spektralnich rezu (prevracena hodnota casoveho rozliseni)

    int n = par["Multibuffer"].get().toInt();

    //v1 - resize internal buffer
//    if(buf) buf->resize(sl);
    //v2 - rellocate
    if(buf) delete(buf);
    buf = (rt_idf_circ_simo<t_rt_slice<T> > *) new rt_idf_circ_simo<t_rt_slice<T> >(n);

    cpb = t_rt_slice<T>(cpb.t, octm*octn); //keeps old time

    for(int i=0; i<RT_CPB_MAX_OCTAVES; dline[i++] = 0)
        if(dline[i]) delete dline[i];

    for(int i=0; i<RT_CPB_MAX_BANDS; i++){
        if(aline[i]) delete aline[i];
        if(aver[i]) delete aver[i];
    }

    //napocitame jen takove zpozdeni ktere je nezbytne
    int delay[octn];
    for(int i=0; i<octn; i++){

        delay[i] = round(gd * ((1 << (octn-i)) - 1));
        dline[i] = new t_DelayLine<T>(delay[i]); //inicializace delay line

        //podporujem jen IIR prumerovani; lin je nerealne pro pocet koef;
        //s vyssi oktavou staci rychlejsi filtr; pod casovou konstantu 1 se ale dostat nechceme, rychlejsi uz proste nebudem
        double avr_k = 1 /* *sta.fs_in/ (2 << i) */;  if(avr_k < 1.0) avr_k = 1.0;
        double avr_num[2] = {1.0 / avr_k, 0.0}, avr_den[2] = {1.0, 1.0 - 1/avr_k};
        for(int j=0; j<octm; j++)
            aver[i*octm + j] = (t_pFilter<T> *) new t_CanonFilter<T>(avr_num, avr_den, 2);

        int rn = 0;
        double num[256], den[256];
        char pcpb[64], pnum[16], pden[16];

        snprintf(pcpb, sizeof(pcpb), "__%d_cpb_biquadiir", octm);
        for(int j=0; j<octm; j++){

            snprintf(pnum, sizeof(pnum), "num%d", j+1);
            snprintf(pden, sizeof(pden), "den%d", j+1);
            rn = par[pcpb].db(pnum, num, sizeof(num)/sizeof(num[0]));
            rn = par[pcpb].db(pden, den, sizeof(den)/sizeof(den[0]));
            aline[i*octm + j] = (t_pFilter<T> *) new t_BiQuadFilter<double>(num, den, rn/3);
        }
    }
}

/*! \brief final assembly of rt_node and template of cpb - floating point version*/
class rt_cpb_fp : virtual public rt_node {

private:
    t_rt_cpb_te<double> worker;

public:
    rt_cpb_fp(QObject *parent = NULL, const QString &config = QString()):
        rt_node(parent),
        worker(config.toStdString())
    {
        init(&worker); //connect real objecr with abstract pointer
    }

    virtual ~rt_cpb_fp(){;}
};

#endif //RT_CPB_QT_H
