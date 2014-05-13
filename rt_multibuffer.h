#ifndef RT_MULTIBUFFER_H
#define RT_MULTIBUFFER_H

#include <QReadWriteLock>
#include <QtDebug>


#define NO_AVAIL_CHECK

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//plna implementace cteciho/zapisovaciho kruh bufferu - misto stavu vraci hodnotu overflow
//overflow < 0 == RUNNING; overflow = 0 == FULL;
//overflow > 1 == OVERFLOW od posledniho cteni(coz ale neznamena ze jsme nezapsali!!)
//primo potomek zamku - zamykame jak cteni tak zapis (cteni je dovoleno az po tom co uvolnen zapis,
//vicero cteni nevadi, zapis take az co je docteno)
//umoznujeme cteni z vicero zdroju - nezavisle read pointery
template <typename T, int N> class t_multibuffer : public QReadWriteLock {
    private:
        int     size;
        int     wmark;
        int     rmark[N];
        int     overflow[N];
        T       *buf;

    public:
        int write(T smp);  //jednoduche pridani vzorku - aby se to nekomplikovalo memcpy
        int read(T *smp, int n = 0);  //jednoduche cteni vzorku
        int write(T *smp, int len);  //cyklicke volani write - zadna lezpsi jednoduch optimalizace neni mozna
        int read(T *smp, int *len, int n = 0);

        int get(T *smp, int len, int n = 0);  //funkce ktera precte bez toho aby posunula ctecim pntr, neupdatuje ani over
        int set(T *smp, int len);  //funkce ktera zapise bez toho aby posunula write pntr, neupdatuje ani over

        int readShift(int len, int n = 0);     //funkce posouvajici cteci pointer (dummy read); pracuje s overflow
        int writeShift(int len);    //funkce posouvajici cteci pointer (dummy write); pracuje s overflow

        int writeSpace(int n = 0);     //misto pro zapis
        int readSpace(int n = 0);      //kolik muzem vycist

        bool    isEmpty(int n = 0);
        bool    isOver(int n = 0);

        bool    resize(int _size);
        void    clear(void);  //nulujem rd a ar indexy; prvky nemenime
        void    init(T df);  //nastavi hodnoty bufferu na konstantni hodnoty

        t_multibuffer(int _size);
        ~t_multibuffer();
};

//------------------------------------------------------------------------------
template <typename T, int N> t_multibuffer<T, N>::t_multibuffer(int _size)
    :size(_size), QReadWriteLock(QReadWriteLock::NonRecursive){

    buf = (T *)0;
    if(N > 0) resize(_size);
}

//------------------------------------------------------------------------------
template <typename T, int N> bool t_multibuffer<T, N>::isEmpty(int n){

    return(((overflow[n % N] < 0) && (wmark == rmark[n % N])));
}

//------------------------------------------------------------------------------
template <typename T, int N> bool t_multibuffer<T, N>::isOver(int n){

    return(overflow[n % N] > 0) ? 1 : 0;
}

//------------------------------------------------------------------------------
//funkce nanstavuje owerwrite, proto pri nacitani hrozi ze pokud mame preteceni
//dovoli nam kruhove nasict libovolne velke mnozstvi dat
//varianta NO_AVAIL_CHECK nehlida kde je zapisovaci pointer
template <typename T, int N> int t_multibuffer<T, N>::get(T *smp, int len, int n){

    int L = len;
    int nn = n % N;
    int bmark = rmark[nn];     //zaloha cteciho pointeru

    lockForRead();

#ifndef NO_AVAIL_CHECK
    while(((wmark != bmark) || (overflow[nn] >= 0))&&(L > 0)){

         if((plen = (wmark > bmark) ? (wmark - bmark) : (size - bmark))) plen = L;
         for(int i=0; i<plen; i++) smp[i] = buf[bmark+i];
         if((bmark += plen) >= size) bmark -= size;
         L -= plen; smp += plen;
    }
#else
    for(int i=0; i<len; i++, L--) smp[i] = buf[(bmark+i) % size];
#endif //NO_AVAIL_CHECK

    unlock();
    return ((len -= L));      //kolik sme skutecne precetli
}

//------------------------------------------------------------------------------
//funkce nenastavuje wr pntr ani ower - slouzi jen pro zapis na slepo
template <typename T, int N> int t_multibuffer<T, N>::set(T *smp, int len){

    int L = 0;

    lockForWrite();
    while(L < len){

        smp[L] = buf[(wmark+L) % size];
        L += 1;
    }
    unlock();

    return L;
}


//------------------------------------------------------------------------------
template <typename T, int N> int t_multibuffer<T, N>::readShift(int len, int n){

    int nn = n % N;

    lockForRead();
    if((rmark[nn] += len) >= size) rmark[nn] -= size;
    overflow[nn] = -1;
    unlock();

    return overflow[nn];
}


//------------------------------------------------------------------------------
template <typename T, int N> int t_multibuffer<T, N>::writeShift(int len){

    lockForWrite();
    while(len--){

        if((wmark += 1) >= size) wmark -= 1;

        for(int nn=0; nn<N; nn++)  //testujem pro vsechny cteci pointery!
            if((overflow[nn] > 0) || (wmark == rmark[nn]))  //musime testovat znovu po updatu
                overflow[nn] = (overflow[nn]++) % size; //vetsi preteceni nez size neindikujem, po prvnim srovnani je overflow 0 == FULL
    }
    unlock();

    return 0;
}

//------------------------------------------------------------------------------
template <typename T, int N> bool t_multibuffer<T, N>::resize(int _size){

    lockForWrite();

    if(buf) delete[] buf;
    size = _size;

    wmark = 0;
    for(int i=0; i<N; i++){

        overflow[i] = -1;
        rmark[i] = 0;
    }

    if(size)    //protoze on jse chopne priradit o nulovenu poli pointer!
        try {

            buf = (T *) new T[_size];
        }
        catch (...) {

            qDebug() << "Multibuffer reallocation error";
        }

    unlock();

    return(buf != (T *)0);
}

//------------------------------------------------------------------------------
template <typename T, int N> void t_multibuffer<T, N>::clear(void){

    lockForWrite();
    //memset(buf, 0, size*sizeof(T));
    wmark = 0;

    for(int i=0; i<N; i++){

        overflow[i] = -1;
        rmark[i] = 0;
    }

    unlock();
}

//------------------------------------------------------------------------------
template <typename T, int N> void t_multibuffer<T, N>::init(T df){

    lockForWrite();

    for(int i=0; i<size; i++)
        buf[i] = df;

    unlock();
}

//------------------------------------------------------------------------------
template <typename T, int N> int t_multibuffer<T, N>::write(T smp){

    lockForWrite();

    buf[wmark++] = smp;
    if(wmark >= size) wmark = 0;

    for(int nn=0; nn < N; nn++)  //testujem pro vsechny cteci pointery!
        if((overflow[nn] > 0) || (wmark == rmark[nn]))  //musime testovat znovu po updatu
            overflow[nn] = (overflow[nn]+1) % size; //vetsi preteceni nez size neindikujem, po prvnim srovnani je overflow 0 == FULL

    unlock();

    return 0;
}

//------------------------------------------------------------------------------
template <typename T, int N> int t_multibuffer<T, N>::read(T *smp, int n){

    int nn = n % N;
    int ret = 0;

    lockForRead();
    if((wmark != rmark[nn]) || (overflow[nn] >= 0)){

        *smp = buf[rmark[nn]++];
        if(rmark[nn] >= size) rmark[nn] = 0;

        overflow[nn] = -1; //OK
        ret = 1;
    }
    unlock();

    return ret;
}

//------------------------------------------------------------------------------
template <typename T, int N> int t_multibuffer<T, N>::write(T *smp, int len){

    int L = 0;
    while(L < len)
        write(smp[L++]); //v pripade nekolika readeru jedina (elegantni) moznost

    return L;
}

//------------------------------------------------------------------------------
template <typename T, int N> int t_multibuffer<T, N>::read(T *smp, int *len, int n){

    int plen, L = *len;
    int nn = n % N;

    lockForRead();
    while(((wmark != rmark[nn]) || (overflow[nn] >= 0)) && (L > 0)){

        plen = (wmark > rmark[nn]) ? (wmark - rmark[nn]) : (size - rmark[nn]);
         if(plen > N) plen = N;
         //memcpy((void *)smp, (void *)&buf[rmark[nn]], plen*sizeof(T));//nelze pro objekty
         for(int i=0; i<plen; i++) smp[i] = buf[rmark[nn]+i];
         if((rmark[nn] += plen) >= size) rmark[nn] -= size;
         L -= plen; smp += plen;
         overflow[nn] = -1;                           //OK
    }
    unlock();

    *len -= N;      //kolik sme skutecne precetli
    return overflow[nn];
}

//------------------------------------------------------------------------------
//vycitani volneho mista z kruh bufferu ktery vyuziva cely prostor
template <typename T, int N> int t_multibuffer<T, N>::readSpace(int n){

    lockForWrite();

    int nn = n % N;
    int tmp = (int)(wmark - rmark[nn]);
    if( ((tmp == 0)&&(overflow[nn] >= 0)) || (tmp < 0) ) tmp += size;   //volny prostor je prelozeny, nebo doslo k preteceni
    unlock();

    return(tmp);   //prostor nebyl prelozeny (rd muze != wr i pri overflow)
}

//------------------------------------------------------------------------------
//vycitani volneho mista z kruh bufferu ktery vyuziva cely prostor
template <typename T, int N> int t_multibuffer<T, N>::writeSpace(int n){

    lockForWrite();

    int nn = n % N;
    int tmp = (int)(rmark[nn] - wmark);
    if(overflow[nn] < 0){

      if(tmp < 0) tmp += size;
    } else
      tmp = 0;

    unlock();
    return tmp; //mam tu preteceni - nelze zapsat nic; az dokud si app overflow neshodi
}

//------------------------------------------------------------------------------
template <typename T, int N> t_multibuffer<T, N>::~t_multibuffer(){

    resize(0);
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

#endif // RT_MULTIBUFFER_H
