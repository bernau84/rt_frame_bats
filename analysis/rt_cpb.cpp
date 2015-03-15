#include "rt_cpb.h"


//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
template<typename T> t_rt_cpb_te<T>::t_rt_cpb_te(QObject *parent):
    i_rt_analysis(parent, QDir(":/config/js_config_cpbanalysis.txt"))
{
    double decif[1024], passf[1] = {1.0};

    gd = par["__oct_deci_directfir"].db("num", decif, sizeof(decif)/sizeof(decif[0]));
    t_DirectFilter<double> deci(decif, gd, 2);
    t_DirectFilter<double> pass(passf, 1);

    bank = new t_DiadicFilterBank<double>(pass, deci, RT_MAX_OCTAVES_NUMBER);  //naddimenzujem pro nejvyssi pocet oktav

    for(int i=0; i<RT_MAX_OCTAVES_NUMBER;                           i++) dline[i] = 0; //vynulujem
    for(int i=0; i<RT_MAX_OCTAVES_NUMBER * RT_MAX_BANDS_PER_OCTAVE; i++) aline[i] = 0;
    for(int i=0; i<RT_MAX_OCTAVES_NUMBER * RT_MAX_BANDS_PER_OCTAVE; i++) aver[i] = 0;

    change(); //doinicializuje analyticke pasmove filtry a delay line
}

//---------------------------------------------------------------------------
template<typename T> void t_rt_cpb_te<T>::~t_rt_cpb_te(){

    if(bank)
        delete bank;

    for(int i=0; i<RT_MAX_OCTAVES_NUMBER; dline[i++] = 0)
        if(dline[i]) delete dline[i];

    for(int i=0; i<RT_MAX_OCTAVES_NUMBER * RT_MAX_BANDS_PER_OCTAVE; aline[i++] = 0)
        if(aline[i]) delete aline[i];

    for(int i=0; i<RT_MAX_OCTAVES_NUMBER * RT_MAX_BANDS_PER_OCTAVE; aver[i++] = 0)
        if(aver[i]) delete aver[i];
}

//---------------------------------------------------------------------------
template<typename T> void t_rt_cpb_te<T>::update(const void *dt, int size){

    const T *w = (const T *)dt; /*! assumes the same type as internal cbuf is! */
    t_rt_slice<T> p_cpb = get<t_rt_slice<T> >(0);  //vyctem aktualni cpb spektrum

    for(int i=0; i<size; i++){

        T lp_st[RT_MAX_OCTAVES_NUMBER]; //
        int n = bank->Process(w[i], lp_st);  //decimacni banka; updatuje vzorek [0] a [n]
        sta.nn_tot += 1;  //vzorek hotov
        sta.nn_run += 1;

        if(n >= octn) //dal pocitame jen ty oktavy ktere jsme si porucily - decimace probiha v max rozliseni (na vykonu to neubira)
            continue;

        T d_st_0 = dline[0]->Process(lp_st[0], 0);  //zpozdovaci linka
        T d_st_n = dline[n]->Process(lp_st[n], 0);

        for(int j=0; j<octm; j++){

            T a_st_0 = aline[j]->Process(d_st_0, 0); //cpb analyticke filtru
            T a_st_n = aline[n*octm + j]->Process(d_st_n, 0);

            //double f_0 = *sta.fs_in * (pow(2, -j/octm-n) - pow(2, -(j+1)/octm-n));
            //double f_n = *sta.fs_in * (pow(2, -j/octm) - pow(2, -(j+1)/octm));

            //upgrade hodnot v bufferu
            p_cpb.A[j] = aver[j]->Process(a_st_0, 0);
            p_cpb.I[j] = j; //f_0
            p_cpb.A[n*octm + j] = aver[n*octm + j]->Process(a_st_n, 0);
            p_cpb.I[n*octm + j] = octm*n+j; //f_n
        }

        double s_time = sta.nn_run++ / *sta.fs_in;  //aktualne zpracovavany cas

        //prepis vypocetnych na nove pokud uplynula doba prumerovani
        if(s_time <= (p_cpb.t + 1 / sta.fs_out)){ //je cas vzorku vetsi nez doba od dalsiho spektra

            write<t_rt_slice<T> >(p_cpb); //zapisem novy jeden radek
            read<t_rt_slice<T> >(p_cpb); //a novy pracovni si hned vyctem (na ctecim bufferu 0)
            p_cpb = t_rt_slice<T>(s_time, octm*octn); //inicializace noveho spektra
            update_clb(); //poslem dal
        }
    }

    set<t_rt_slice<T> >(p_cpb);  //vratime aktualni cpb spektrum
}

//------------------------------------------------------------------------
template<typename T> void t_rt_cpb_te<T>::change(t_collection &par){

    octn = par["Octaves"].get().toDouble();  //aktualni pocet oktav
    octm = par["Bands"].get().toDouble();  //pocet pasem na oktavu

    sta.fs_out = 1.0 / par["Time"].get().toDouble();  //vystupni frekvence spektralnich rezu (prevracena hodnota casoveho rozliseni)

    resize(par["Slices"].get().toDouble()); //"novy" vnitrni multibuffer; data se ale pokusime zachovat

    t_rt_slice<T> s_empty(0.0, octm*octn);    //defaultni/prazdny radek
    set<t_rt_slice<T> >(s_empty);  //pripravime (vynulujem) si aktualni radek (nedopocitany nejspis)

    for(int i=0; i<RT_MAX_OCTAVES_NUMBER; dline[i++] = 0)
        if(dline[i]) delete dline[i];

    for(int i=0; i<RT_MAX_OCTAVES_NUMBER * RT_MAX_BANDS_PER_OCTAVE; aline[i++] = 0)
        if(aline[i]) delete aline[i];

    for(int i=0; i<RT_MAX_OCTAVES_NUMBER * RT_MAX_BANDS_PER_OCTAVE; aver[i++] = 0)
        if(aver[i]) delete aver[i];

    //napocitame jen takove zpozdeni ktere je nezbytne
    T delay[octn];
    for(int i=0; i<octn; i++){

        delay[i] = gd * ((1 << (octn-i)) - 1);
        dline[i] = new t_DelayLine<T>(round(delay[i])); //inicializace delay line

        //podporujem jen IIR prumerovani; lin je nerealne pro pocet koef;
        //s vyssi oktavou staci rychlejsi filtr; pod casovou konstantu 1 se ale dostat nechceme, rychlejsi uz proste nebudem
        double avr_k = 1 /* *sta.fs_in/ (2 << i) */;  if(avr_k < 1.0) avr_k = 1.0;
        double avr_num[2] = {1.0 / avr_k, 0.0}, avr_den[2] = {1.0, 1.0 - 1/avr_k};
        for(int j=0; j<octm; j++)
            aver[i*octm + j] = (t_pFilter<T> *) new t_CanonFilter<T>(avr_num, avr_den, 2);

        int rn = 0;
        double num[256], den[256];
        char pcpb[64], pnum[16], pden[16];

        snprintf(pcpb, sizeof(pcpb), "__%d_cpb_biquadiir", i);
        for(int j=0; j<octm; j++){

            snprintf(pnum, sizeof(pnum), "num%d", j);
            snprintf(pden, sizeof(pden), "den%d", j);
            rn = par[pcpb].db(pnum, num, sizeof(num)/sizeof(num[0]));
            rn = par[pcpb].db(pden, den, sizeof(den)/sizeof(den[0]));
            aline[i*octm + j] = (t_pFilter<T> *) new t_BiQuadFilter<double>(num, den, rn/3);
        }
    }

    change_clb(); //poslem dal
}

