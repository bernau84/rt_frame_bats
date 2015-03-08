#include "rt_analysis.h"
#include "base\rt_basictypes.h"

//---------------------------------------------------------------------------
t_rt_analysis::t_rt_analysis(QObject *parent, const QDir &config):
    t_rt_base(parent, config)
{

}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
t_rt_cpb::t_rt_cpb(QObject *parent):
    t_rt_analysis(parent, QDir(":/config/js_config_cpbanalysis.txt"))
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
t_rt_cpb::~t_rt_cpb(){

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
void t_rt_cpb::analyse(const t_rt_slice<double> &w){

    t_rt_slice<double> p_cpb = t_slcircbuf::get(0);  //vyctem aktualni cpb spektrum

    for(int i=0; i<w.A.size(); i++){

        double lp_st[RT_MAX_OCTAVES_NUMBER]; //
        int n = bank->Process(w.A[i], lp_st);  //decimacni banka; updatuje vzorek [0] a [n]
        sta.nn_tot += 1;  //vzorek hotov
        sta.nn_run += 1;

        if(n >= octn) //dal pocitame jen ty oktavy ktere jsme si porucily - decimace probiha v max rozliseni (na vykonu to neubira)
            continue;

        double d_st_0 = dline[0]->Process(lp_st[0], 0);  //zpozdovaci linka
        double d_st_n = dline[n]->Process(lp_st[n], 0);

        for(int j=0; j<octm; j++){

            double a_st_0 = aline[j]->Process(d_st_0, 0); //cpb analyticke filtru
            double a_st_n = aline[n*octm + j]->Process(d_st_n, 0);

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

            t_slcircbuf::write(p_cpb); //zapisem novy jeden radek
            t_slcircbuf::read(p_cpb); //a novy pracovni si hned vyctem (na ctecim bufferu 0)
            p_cpb = t_rt_slice<double>(s_time, octm*octn); //inicializace noveho spektra
        }
    }

    t_slcircbuf::set(p_cpb);  //vratime aktualni cpb spektrum
}

//------------------------------------------------------------------------
void t_rt_cpb::change(){

    octn = par["Octaves"].get().toDouble();  //aktualni pocet oktav
    octm = par["Bands"].get().toDouble();  //pocet pasem na oktavu

    pause();

    sta.fs_out = 1.0 / par["Time"].get().toDouble();  //vystupni frekvence spektralnich rezu (prevracena hodnota casoveho rozliseni)

    t_slcircbuf::resize(par["Slices"].get().toDouble()); //"novy" vnitrni multibuffer; data se ale pokusime zachovat

    t_rt_slice<double> s_empty(0.0, octm*octn);    //defaultni/prazdny radek
    t_slcircbuf::set(s_empty);  //pripravime (vynulujem) si aktualni radek (nedopocitany nejspis)

    for(int i=0; i<RT_MAX_OCTAVES_NUMBER; dline[i++] = 0)
        if(dline[i]) delete dline[i];

    for(int i=0; i<RT_MAX_OCTAVES_NUMBER * RT_MAX_BANDS_PER_OCTAVE; aline[i++] = 0)
        if(aline[i]) delete aline[i];

    for(int i=0; i<RT_MAX_OCTAVES_NUMBER * RT_MAX_BANDS_PER_OCTAVE; aver[i++] = 0)
        if(aver[i]) delete aver[i];

    //napocitame jen takove zpozdeni ktere je nezbytne
    double delay[octn];
    for(int i=0; i<octn; i++){

        delay[i] = gd * ((1 << (octn-i)) - 1);
        dline[i] = new t_DelayLine<double>(round(delay[i])); //inicializace delay line

        //podporujem jen IIR prumerovani; lin je nerealne pro pocet koef;
        //s vyssi oktavou staci rychlejsi filtr; pod casovou konstantu 1 se ale dostat nechceme, rychlejsi uz proste nebudem
        double avr_k = 1 /* *sta.fs_in/ (2 << i) */;  if(avr_k < 1.0) avr_k = 1.0;
        double avr_num[2] = {1.0 / avr_k, 0.0}, avr_den[2] = {1.0, 1.0 - 1/avr_k};
        for(int j=0; j<octm; j++)
            aver[i*octm + j] = (t_pFilter<double> *) new t_CanonFilter<double>(avr_num, avr_den, 2);

        int rn = 0;
        double num[256], den[256];
        char pcpb[64], pnum[16], pden[16];

        snprintf(pcpb, sizeof(pcpb), "__%d_cpb_biquadiir", i);
        for(int j=0; j<octm; j++){

            snprintf(pnum, sizeof(pnum), "num%d", j);
            snprintf(pden, sizeof(pden), "den%d", j);
            rn = par[pcpb].db(pnum, num, sizeof(num)/sizeof(num[0]));
            rn = par[pcpb].db(pden, den, sizeof(den)/sizeof(den[0]));
            aline[i*octm + j] = (t_pFilter<double> *) new t_BiQuadFilter<double>(num, den, rn/3);
        }
    }

    emit on_change(); //poslem dal
    resume();
}


//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
t_rt_shift::t_rt_shift(QObject *parent):
    t_rt_analysis(parent, QDir(":/config/js_config_freqshift.txt"))
{
    change(); //doinicializuje analyticke pasmove filtry a delay line
}

//---------------------------------------------------------------------------
void t_rt_shift::analyse(const t_rt_slice<double> &w){

    t_rt_slice<double> p_shift = t_slcircbuf::get(0);  //vyctem aktualni rez
    t_rt_slice<double>::t_rt_ai last = p_shift.last();  //posledni zapsany

    for(int i=0; i<w.A.size(); i++){

        for(int m=0; m<decif; m++){

            int dd;
            double t_filt = bank[m]->Process(w.A[i], &dd);  //band pass & decimace

            sta.nn_run += 1;
            sta.nn_tot += 1;

            if((dd == 0) && (mask & (1 << m))){ //z filtru vypadl decimovany vzorek ktery chcem

                if(m & 0x1) //mirroring u lichych pasem
                    t_filt *= -1;

                last.A += t_filt;  //scitame s prispevky od jinych filtru (pokud jsou vybrany)
                last.I += 1;
            }

            if(last.I == (decif-1)){  //mame hotovo (decimace D) vzorek muze jit ven

                p_shift.set(last);
                if((p_shift.A.size()) == refr) {

                    t_slcircbuf::write(p_shift); //zapisem novy jeden radek
                    t_slcircbuf::read(p_shift);
                    p_shift = t_rt_slice<double>(0.0, 1); //inicializace noveho spektra
                }

                t_rt_slice<double>::t_rt_ai empty = {0.0, 0.0};
                last = empty;
                p_shift.append(last); //pripravim novy
            }
        }
    }

    p_shift.set(last);
    t_slcircbuf::set(p_shift);  //vratime aktualni
}

//------------------------------------------------------------------------
void t_rt_shift::change(){

    decif = par["Bands"].get().toDouble();  //pocet pasem / decimacni faktor
    numb = par["Multibuffer"].get().toDouble();  //pocet bodu v radku
    refr = par["Refresh"].get().toDouble();

    QJsonArray selected; /*! vybrana pasma */
    QJsonValue ts = par["Select"].get();
    if(ts.isArray()) selected = ts.toArray();  //vyber multiband
    else if(ts.isDouble()) selected.append(ts.toDouble());  //one selection

    for(int d=0; d<decif; d++) if(selected.contains(d)) mask |= (1 << d);

    /*! \todo - jak nastavit jen limit a ponechat vyber? */
    //set["Select"]->set("max", D); - nenastavuje limit ale i hodnotu

    pause();

    sta.fs_out = *sta.fs_in / decif;  //vystupni frekvence spektralnich rezu (prevracena hodnota casoveho rozliseni)
    t_slcircbuf::resize(refr); //novy vnitrni multibuffer

    t_rt_slice<double> dfs(0.0, 1);  //priprava radku (musi mit jeden prvek!)
    t_slcircbuf::set(dfs);

    for(int i=0; i<RT_MAX_BANDS_PER_OCTAVE; bank[i++] = 0)
        if(bank[i]) delete bank[i];

    double coe[1024], sh_coe[1024];
    char scoe[16]; snprintf(scoe, sizeof(scoe), "num%d", decif);
    int rn = par["__bandpass_directfir"].db(scoe, coe, numb);  //baseband

    //bandpass bank
    for(int i=0; i<decif; i++){

        for(int j= 0; j<rn; j++) sh_coe[j] = coe[j] * cos((2*PI*j) * (1.0*i/decif));  //freq. shift
        bank[i] = (t_pFilter<double> *) new t_DirectFilter<double>(sh_coe, rn, decif);
        for(int j=0; j<(decif-i); j++) bank[i]->Process(0.0, NULL);  //finta - inicizizace vnitrniho buferu nulami
                //kazdy filtr je inicializaovan jinak takze kazdy se v ramci decimace pocita
                //s ruznym vzorkem - zajistuje harmonizaci rychlosti vypoctu
    }

    emit on_change(); //poslem dal
    resume();
}

//------------------------------------------------------------------------
t_rt_shift::~t_rt_shift(){

    for(int i=0; i<RT_MAX_BANDS_PER_OCTAVE; bank[i++] = 0)
        if(bank[i]) delete bank[i];
}

//---------------------------------------------------------------------------
