#include "rt_analysis.h"
#include "rt_basictypes.h"


//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//do budoucna lepsi nacitat to z ini_file

const double def_allps_num[] = {
                                        1.0
                                };
const double def_allps_den[] = {
                                        0.0
                                };


const double def_01cpb_num[1][6*3] =    {
                                            {
                                                #include "include/1_1elip_num.h"
                                            }
                                        };
const double def_01cpb_den[1][6*3] =    {
                                            {
                                                #include "include/1_1elip_den.h"
                                            }
                                        };
const double def_03cpb_num[3][3*3] = {  {
                                        //#include "include/3_3inv_num.h"
                                        #include "include/3_3cheb_num.h"
                                        }, {
                                        //#include "include/2_3inv_num.h"
                                        #include "include/2_3cheb_num.h"
                                        }, {
                                        //#include "include/1_3inv_num.h"
                                        #include "include/1_3cheb_num.h"
                                     }  };
const double def_03cpb_den[3][3*3] = {  {
                                        //#include "include/3_3inv_den.h"
                                        #include "include/3_3cheb_den.h"
                                        }, {
                                        //#include "include/2_3inv_den.h"
                                        #include "include/2_3cheb_den.h"
                                        }, {
                                        //#include "include/1_3inv_den.h"
                                        #include "include/1_3cheb_den.h"
                                     }  };
const double def_12cpb_num[12][3*2] = {
                                        {0} //#include "01cpb_num.h"
                                    };
const double def_12cpb_den[12][3*2] = {
                                        {0} //#include "01cpb_num.h"
                                    };
const double def_24cpb_num[24][3*1] = {
                                        {0} //#include "03cpb_num.h"
                                    };
const double def_24cpb_den[24][3*1] = {
                                        {0} //#include "03cpb_num.h"
                                    };

//---------------------------------------------------------------------------
t_rt_analysis::t_rt_analysis(QObject *parent):
    t_rt_base(parent)
{
}

//---------------------------------------------------------------------------
void t_rt_analysis::start(){
    t_rt_base::start();
}

//------------------------------------------------------------------------
void t_rt_analysis::pause(){
    t_rt_base::pause();
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
t_rt_cpb::t_rt_cpb(QObject *parent):
    t_rt_analysis(parent)
{
    set.initdef("Multibuffer", t_setup_entry(set.vlist() << 10 << 20 << 30 << 40 << 50 << 70 << 100, "", 4));
    set.initdef("Octaves", t_setup_entry(set.vlist() << 8 << 10 << 12 << 14, ""));
    set.initdef("Bands", t_setup_entry(set.vlist() << 1 << 3 << 12 << 24, "", 1, true));

    set.initdef("Time", t_setup_entry(
                    set.vlist() << 1.0 << 1.0/2 << 1.0/4 << 1.0/8 << 1.0/16 << 1.0/32 << 1.0/64,
                    set.slist() << "slow [1s]" << "slow [1/2s]" << "normal [1/4s]" << "normal [1/8s]"
                                << "fast [1/16s]" << "fast [1/32s]" << "fast [1/64s]", 3));


    double tdef_decim[] = {
       #include "include/a_3fir.h"
    };

    QVariantList list_def_decim =
        QVariantList::fromStdList(std::list<double>(&tdef_decim[0], &tdef_decim[sizeof(tdef_decim) / sizeof(double)]));

    set.initdef("FirDeciCoe", t_setup_entry(list_def_decim));


    t_DirectFilter dnum((double *)def_allps_num, (double *)0, sizeof(def_allps_num)/sizeof(double));
    t_DirectFilter dden((double *)def_decim_num, (double *)0, sizeof(def_decim_num)/sizeof(double), 2);
    bank = new t_DiadicFilterBank(dnum, dden, RT_MAX_OCTAVES_NUMBER);  //naddimenzujem pro nejvyssi pocet oktav

    for(int i=0; i<RT_MAX_OCTAVES_NUMBER;                            i++) dline[i] = 0; //vynulujem
    for(int i=0; i<RT_MAX_OCTAVES_NUMBER * RT_MAX_BANDS_PER_OCTAVE;  i++) aline[i] = 0;
    for(int i=0; i<RT_MAX_OCTAVES_NUMBER * RT_MAX_BANDS_PER_OCTAVE;  i++) aver[i] = 0;

    change(); //doinicializuje analyticke pasmove filtry a delay line
}

//---------------------------------------------------------------------------
void t_rt_cpb::process(){

    int N = set["Octaves"].get().toInt();  //aktualni pocet oktav
    int M = set["Bands"].get().toInt();  //pocet pasem na oktavu

    t_rt_base *pre = dynamic_cast<t_rt_base *>(parent());
    if(!pre) return; //navazujem na zdroj dat?

    if(sta.state != t_rt_status::ActiveState){

        int n_dummy = pre->t_slcircbuf::readSpace(rd_i); //zahodime data
        pre->t_slcircbuf::readShift(n_dummy, rd_i);
        return;  //bezime?
    }

    t_rt_slice p_cpb;   //radek soucasneho spektra
    t_slcircbuf::get(&p_cpb, 1);  //vyctem aktualni cpb spektrum

    t_rt_slice t_amp;  //radek caovych dat
    while(pre->t_slcircbuf::read(&t_amp, rd_i)){ //vyctem radek

        for(int i=0; i<t_amp.i; i++){

            double lp_st[RT_MAX_OCTAVES_NUMBER]; //
            int n = bank->Process(t_amp.A[i], lp_st);  //decimacni banka; updatuje vzorek [0] a [n]

            if(n >= N) //dal pocitame jen ty oktavy ktere jsme si porucily - decimace probiha v max rozliseni (na vykonu to neubira)
                continue;

            double d_st_0 = dline[0]->Process(lp_st[0], 0);  //zpozdovaci linka
            double d_st_n = dline[n]->Process(lp_st[n], 0);

            for(int j=0; j<M; j++){

                double a_st_0 = aline[j]->Process(d_st_0, 0); //cpb analyticke filtru
                double a_st_n = aline[n*M + j]->Process(d_st_n, 0);

                p_cpb.A[j]       = aver[j]->Process(a_st_0, 0); //vypocet uprade hodnot v bufferu
                p_cpb.A[n*M + j] = aver[n*M + j]->Process(a_st_n, 0);
            }

            //prepis vypocetnych na nove pokud uplynula doba prumerovani
            if((1 + nn_tot) / sta.fs_out <= t_amp.f[i]){ //je cas vzorku vetsi nez doba od dalsiho spektra

                t_slcircbuf::write(p_cpb); //zapisem novy jeden radek
                t_slcircbuf::readShift(1); //a novy pracovni si hned vyctem
                t_slcircbuf::get(&p_cpb, 1);

                p_cpb.i = 0; //jdem od zacatku
                p_cpb.t = nn_tot / sta.fs_out; //predpoklad konstantnich t inkrementu; cas 1. ho vzorku

                nn_tot += 1;  //novy rez
            }
        }
    }

    t_slcircbuf::set(&p_cpb, 1);  //vratime aktualni cpb spektrum
}

//------------------------------------------------------------------------
void t_rt_cpb::change(){

    int N = set["Octaves"].get().toInt();  //aktualni pocet oktav
    int M = set["Bands"].get().toInt();  //pocet pasem na oktavu
    t_rt_status::t_rt_a_sta pre_sta = sta.state;

    pause();

    sta.fs_out = 1 / set["Time"].get().toDouble();  //vystupni frekvence spektralnich rezu (prevracena hodnota casoveho rozliseni)
    t_slcircbuf::resize(set["Slices"].get().toInt()); //novy vnitrni multibuffer

    //inicializace prvku na defaultni hodnoty
    t_rt_slice dfs;
    QVector<double> cpb_f(N*M, 0); for(int fn=0; fn < M*N; fn++) cpb_f[fn] = fn; //mozno ale pocitat centralni frekvence filtru
    dfs.A = QVector<double>(N*M, 0);
    dfs.f = cpb_f;
    dfs.i = 0;
    dfs.t = 0.0; //vse na 0

    t_slcircbuf::init(dfs); //nastavime vse na stejno
    t_slcircbuf::clear(); //vynulujem ridici promenne - zacnem jako po startu na inx 0

    for(int i=0; i<RT_MAX_OCTAVES_NUMBER; dline[i++] = 0)
        if(dline[i]) delete dline[i];

    for(int i=0; i<RT_MAX_OCTAVES_NUMBER * RT_MAX_BANDS_PER_OCTAVE; aline[i++] = 0)
        if(aline[i]) delete aline[i];

    for(int i=0; i<RT_MAX_OCTAVES_NUMBER * RT_MAX_BANDS_PER_OCTAVE; aver[i++] = 0)
        if(aver[i]) delete aver[i];

    //napocitame jen takove zpozdeni ktere je nezbytne
    double delay[N], gd = sizeof(def_decim_num)/sizeof(double)/2.0 + 1;
    for(int i=0; i<N; i++){

        delay[i] = gd * ((1 << (N-i)) - 1);
        dline[i] = new t_DelayLine((double *)0, (double *)0, (int)delay[i]); //inicializace delay line

        //podporujem jen IIR prumerovani; lin je nerealne pro pocet koef;
        //s vyssi oktavou staci rychlejsi filtr; pod casovou konstantu 1 se ale dostat nechceme, rychlejsi uz proste nebudem
        double avr_k = 1 /* *sta.fs_in/ (2 << i) */;  if(avr_k < 1.0) avr_k = 1.0;
        double avr_num[2] = {1.0 / avr_k, 0.0}, avr_den[2] = {1.0, 1.0 - 1/avr_k};
        for(unsigned int j=0; j<M; j++)
            aver[i*M + j] = (t_pFilter *) new t_CanonFilter(avr_num, avr_den, 2);

        switch(M){

            case(1):
                aline[i] = (t_pFilter *) new t_BiQuadFilter((double *)def_01cpb_num, (double *)def_01cpb_den, 6);
            break;
            case(3):
                for(unsigned int j=0; j<M; j++)
                    aline[i*M + j] = (t_pFilter *) new t_BiQuadFilter((double *)&def_03cpb_num[j][0], (double *)&def_03cpb_den[j][0], 3);
            break;
            case(12):
                for(unsigned int j=0; j<M; j++)
                    aline[i*M + j] = (t_pFilter *) new t_BiQuadFilter((double *)&def_12cpb_num[j][0], (double *)&def_12cpb_den[j][0], 2);
            break;
            case(24):
                for(unsigned int j=0; j<M; j++)
                    aline[i*M + j] = (t_pFilter *) new t_BiQuadFilter((double *)&def_24cpb_num[j][0], (double *)&def_24cpb_den[j][0], 1);
            break;
        }
    }

    emit on_change(); //poslem dal

    switch(pre_sta){

        case t_rt_status::SuspendedState:
        case t_rt_status::StoppedState:
            break;

        case t_rt_status::ActiveState:

            start();
            break;
    }
}


//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
t_rt_shift::t_rt_shift(QObject *parent):
    t_rt_analysis(parent)
{
    set["Multibuffer"] = t_setup_entry(set.vlist() << 10 << 20 << 30 << 40 << 50 << 70 << 100, "", 4, true);
    set["Bands"] = t_setup_entry(set.vlist() << 4 << 8 << 16 << 32, "equi_fir", 1, true);
    set["Select"] = t_setup_entry(set.vlist() << 4 << 32, "", 0, true);    //prostredni pasmo zvolim jako default

    /*! \todo - realne se musi "select" nabidla menit se zmenou pasem
    * navic by mela umoznovat multiband select - coz pres sel() fci umoznuje
    */

    t_DirectFilter dnum((double *)def_allps_num, (double *)0, sizeof(def_allps_num)/sizeof(double));
    t_DirectFilter dden((double *)def_decim_num, (double *)0, sizeof(def_decim_num)/sizeof(double), 2);
    bank = new t_DiadicFilterBank(dnum, dden, RT_MAX_OCTAVES_NUMBER);  //naddimenzujem pro nejvyssi pocet oktav

    for(int i=0; i<RT_MAX_BANDS_PER_OCTAVE;  i++) bank[i] = 0;

    change(); //doinicializuje analyticke pasmove filtry a delay line
}

//---------------------------------------------------------------------------
void t_rt_shift::process(){

    int D = set["Bands"].get().toInt();  //pocet pasem na oktavu
    QList<QVariant> selected = set["Select"].getm();  //vyber co jde ven
    uint mask = 0; for(int d=0; d<D; d++) if(selected.contains(d)) mask |= (1 << d);

    t_rt_base *pre = dynamic_cast<t_rt_base *>(parent());
    if(!pre) return; //navazujem na zdroj dat?

    if(sta.state != t_rt_status::ActiveState){

        int n_dummy = pre->t_slcircbuf::readSpace(rd_i); //zahodime data
        pre->t_slcircbuf::readShift(n_dummy, rd_i);
        return;  //bezime?
    }

    t_rt_slice p_shift;   //radek soucasneho spektra
    t_slcircbuf::get(&p_shift, 1);  //vyctem aktualni rez

    t_rt_slice t_amp;  //radek caovych dat
    while(pre->t_slcircbuf::read(&t_amp, rd_i)){ //vyctem radek

        for(int i=0; i<t_amp.i; i++){

            int dd, m  = (nn_tot++ % D);
            double t_filt = bank[m]->Process(t_amp.A[i], &dd);  //band pass & decimace

            if((dd == 0) && (mask & (1 << m))){ //z filtru vypadl decimovany vzorek ktery chcem

                if(m & 0x1) //mirroring u lichych pasem
                    t_filt *= -1;

                p_shift.A[p_shift.i] += t_filt;  //scitame s prispevky od jinych filtru (pokud jsou vybrany)
            }

            if(m == (D-1)){  //mame hotovo (decimace D) vzorek muze jit ven

                p_shift.f[p_shift.i] = nn_tot / *sta.fs_in; //vkladame cas kazdeho vzorku
                if((p_shift.i += 1) == p_shift.A.count()) {

                    t_slcircbuf::write(p_shift); //zapisem novy jeden radek
                    t_slcircbuf::readShift(1); //a novy pracovni si hned vyctem
                    t_slcircbuf::get(&p_shift, 1);
                    p_shift.i = 0; //jdem od zacatku
                    p_shift.t = nn_tot / *sta.fs_in; //predpoklad konstantnich t inkrementu; cas 1. ho vzorku
                }

                p_shift.A[p_shift.i] = 0.0;  //pripravime novy
            }
        }
    }

    t_slcircbuf::set(&p_cpb, 1);  //vratime aktualni cpb spektrum
}

//------------------------------------------------------------------------
void t_rt_shift::change(){

    int D = set.get["Bands"].get().toInt();  //pocet pasem / decimacni faktor
    int N = set.get["Multibuffer"].get().toInt();  //pocet bodu v radku
    t_rt_status::t_rt_a_sta pre_sta = sta.state;

    /*! \todo - jak nastavit jen limit a ponechat vyber? */
    //set["Select"].set("max", D); - nenastavuje limit ale i hodnotu

    pause();

    sta.fs_out = sta.fs_in / D;  //vystupni frekvence spektralnich rezu (prevracena hodnota casoveho rozliseni)
    t_slcircbuf::resize(setget("Slices").toInt()); //novy vnitrni multibuffer

    //inicializace prvku na defaultni hodnoty
    /*! \todo - vymyslet ja vyuzit frekvenci osu */
    t_rt_slice dfs;
    dfs.A = QVector<double>(N, 0);
    dfs.f = QVector<double>(N, 0);
    dfs.i = 0;
    dfs.t = 0.0; //vse na 0

    t_slcircbuf::init(dfs); //nastavime vse na stejno
    t_slcircbuf::clear(); //vynulujem ridici promenne - zacnem jako po startu na inx 0

    for(int i=0; i<RT_MAX_BANDS_PER_OCTAVE; bank[i++] = 0)
        if(bank[i]) delete bank[i];

    //napocitame jen takove zpozdeni ktere je nezbytne
    for(int i=0; i<D; i++){

        bank[i] = (t_pFilter *) new t_DirectFilter(coef_bp[N], NULL, D);
        for(int j=0; j<(D-i); j++) bank[i]->Process(0.0, NULL);  //finta - inicizizace vnitrniho buferu nulami
                //kazdy filtr je inicializaovan jinak takze kazdy se v ramci decimace pocita
                //s ruznym vzorkem - zajistuje harmonizaci rychlosti vypoctu
    }

    emit on_change(); //poslem dal

    switch(pre_sta){

        case t_rt_status::SuspendedState:
        case t_rt_status::StoppedState:
            break;

        case t_rt_status::ActiveState:

            start();
            break;
    }
}
//---------------------------------------------------------------------------
