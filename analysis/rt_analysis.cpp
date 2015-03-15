

//---------------------------------------------------------------------------
template <typename T> void t_rt_shift::analyse(T dt, int size){

    t_rt_slice<T> p_shift = t_slcircbuf::get(0);  //vyctem aktualni rez
    t_rt_slice<T>::t_rt_ai last = p_shift.last();  //posledni zapsany

    for(int i=0; i<size; i++){

        for(int m=0; m<decif; m++){

            int dd;
            T t_filt = bank[m]->Process(dt[i], &dd);  //band pass & decimace

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
