#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

//#include "aproximation.h"
//#include "sort_operation.h"
//#include "wav_operation.h"
#include "freq_analysis.h" //pozor odkud to taha

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/* a(0)*y(n) = b(0)*x(n) + b(1)*x(n-1) + ... + b(nb-1)*x(n-nb)
                         - a(1)*y(n-1) - ... - a(na-1)*y(n-na) */

/* koeficient a[0] vrele doporucuji nastavovat na 1 - todle nechodi ani v matlabu */

/* input_x = x(n-N+1), x(n-N+2), ..., x(n-1), x(n) => !v poli od nejstarsiho! */
/* input_x[0] = x(n-N-1); input_x[N-1] = x(n) */

void my_filter( FILTER_DATA *f_specify, double input_x[], int x_N, double init_state[], double *output_y ){

        switch( f_specify->structure ){
                case( IIR_DIRECT2_STRUCT ): {

                        /* v[n] = x[n] - (a[1]*v[n-1]+a[2]*v[n-2]+...+a[den_N-1]*v[n-den_N+1]
                        a[0]*y[n] = b[0]*v[n]+b[1]*v[n-1]+...+b[num_N-1]*v[n-num_N+1]
                        init_state = { v[n-1], v[n-2], ... v[n-max(den_N,num_N)+1]}         */

                        int int_N = ( f_specify->den_N > f_specify->num_N ) ?
                                        (x_N + f_specify->den_N - 1) : (x_N + f_specify->num_N - 1);
                        double *int_buf = new double[ int_N ]; //zvetseny o inicializacni historii

                        if( init_state != NULL ){

                                for( int i = 0; i < (int_N-x_N); int_buf[i] = init_state[i++] ){;}     //inicializace
                        } else  for( int i = 0; i < (int_N-x_N); int_buf[i++] = 0.0 ){;}               //default inicializace

                        for( int x_pos = 0, n = (int_N - x_N); x_pos < x_N; x_pos++, n++ ){

                                //vypocet v[n]
                                int_buf[n] = input_x[x_pos];
                                for( int int_pos = 1; int_pos < f_specify->den_N; int_pos++ )
                                        int_buf[n] = int_buf[n] - int_buf[n - int_pos]*f_specify->den_coeff[int_pos];

                                //vypocet vystupu y[n]
                                output_y[x_pos] = 0.0;
                                for( int int_pos = 0; int_pos < f_specify->num_N; int_pos++ )
                                        output_y[x_pos] += int_buf[n - int_pos]*f_specify->num_coeff[int_pos];
                                output_y[x_pos] /= f_specify->den_coeff[0];
                        }

                        delete int_buf;
                }
                break;
                case( FIR_DIRECT1_STRUCT ):{

                        int int_N = x_N + f_specify->den_N - 1;
                        double *int_buf = new double[ int_N ]; //zvetseny o inicializacni historii

                        if( init_state != NULL ){

                                for( int i = 0; i < (int_N-x_N); int_buf[i] = init_state[i++] ){;}     //inicializace
                        } else  for( int i = 0; i < (int_N-x_N); int_buf[i++] = 0.0 ){;}               //default inicializace

                        output_y = new double[ x_N ];

                        for( int x_pos = 0, n = (int_N - x_N); x_pos < x_N; x_pos++, n++ ){

                                //vypocet vystupu y[n]
                                output_y[x_pos] = 0.0; int_buf[n] = input_x[x_pos];
                                for( int int_pos = 0; int_pos < f_specify->num_N; int_pos++ )
                                        output_y[x_pos] += int_buf[n - int_pos]*f_specify->num_coeff[int_pos];
                        }

                        delete int_buf;
                }
                break;
                case( FFT_FILTER_STRUCT ):{
                        /*v tool budou pro tento pripad ulozeny vahy pro realnou cast (numerator)
                        a penalizacni vahy pro imaginarni cas (denumerator)*/

                        unsigned int int_n = x_N; t_Compl *spect;

                        spect = my_fft( input_x, NULL, &int_n );  //FFT

                        if(int_n == (unsigned)x_N){  //pokud byl pocet prvku jiny nec 2^M nema cenu pokracovat

                                double *input_i = (double *)calloc( int_n, sizeof(double) );
                                double *input_r = (double *)calloc( int_n, sizeof(double) );

                                //kopie vahovani plus normalizace
                                for( int i=0; i<f_specify->num_N; i++ ) input_r[i] = spect[i].real * (f_specify->num_coeff[i]/int_n);
                                for( int i=f_specify->num_N; i<int_n; i++ ) input_r[i]= spect[i].real / int_n; //dokonceni normalizace
                                for( int i=0; i<f_specify->den_N; i++ ) input_i[i] = spect[i].imag * (f_specify->den_coeff[i]/int_n);
                                for( int i=f_specify->den_N; i<int_n; i++ ) input_i[i]= spect[i].imag / int_n; //dokonceni normalizace

                                free(spect); spect = my_fft( input_r, input_i, &int_n ); //IFFT

                                //vysledek je kruhove posunuty o 1 a zrcadlovy
                                output_y[0] = spect[0].real; for( int i=1; i<x_N; i++ ) output_y[i] = spect[int_n-i].real;

                                free( input_i ); free( input_r );
                         }
                         free(spect);
                }
                break;
                case( IIR_DIRECT1_STRUCT ): break;   //tyto implementace popripade dodelat pouzitim real-time filtru
                case( IIR_LATTICE_STRUCT ): break;
                case( IIR_BIQUADR_STRUCT ): break;
                case( FIR_LATTICE_STRUCT ): break;
        }
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
/*
//Obecny predek pro real-time cislicove filtry
class t_pFilter{

    protected:
        TBuffer<double>   *shift_reg;//bude se ale vytvaret hluboka kopie
        double            *shift_dat;
        double            *coeff_num;
        double            *coeff_den;
        int               N; //pocet koeficientu

    public:
        void    Reset(double *_shift_reg);//inicializuje posuvny_reg, pokud NULL pak ho vynuluje
        virtual double Process(double new_smpl) = 0;

        t_pFilter(double *_coeff_num, double *_coeff_den, int _N);
        ~t_pFilter(); //uvolni doeficienty a destroyne Buffer
};
*/

//------------------------------------------------------------------------------
t_pFilter::t_pFilter(double *_coeff_num, double *_coeff_den, int _N)
          :coeff_num(_coeff_num), coeff_den(_coeff_den), dec_fact(1){

    shift_reg = NULL;
    act_stat  = dec_fact;
    this->typ_filt = -1;

    if( (N = _N) > 1 ){

      shift_dat = (double *)calloc( N-1, sizeof(double) );
      shift_reg = new TBuffer<double>( shift_dat, N-1, BM_CIRC );
    }
}

//------------------------------------------------------------------------------
t_pFilter::t_pFilter(double *_coeff_num, double *_coeff_den, int _N, int _dec_fact)
          :coeff_num(_coeff_num), coeff_den(_coeff_den), dec_fact(_dec_fact){

    shift_reg = NULL;
    act_stat  = dec_fact;
    typ_filt  = -1;

    if( (N = _N) > 1 ){

      shift_dat = (double *)calloc( N-1, sizeof(double) );
      shift_reg = new TBuffer<double>( shift_dat, N-1, BM_CIRC );
    }
}

//------------------------------------------------------------------------------
t_pFilter::t_pFilter(t_pFilter &src_coeff_cpy){

    shift_reg = NULL;
    act_stat  = src_coeff_cpy.dec_fact;
    typ_filt  = src_coeff_cpy.GetTyp();

    coeff_num = src_coeff_cpy.coeff_num;
    coeff_den = src_coeff_cpy.coeff_den;
    dec_fact  = src_coeff_cpy.dec_fact;

    if( (N = src_coeff_cpy.N) > 1 ){

      shift_dat = (double *)calloc( N-1, sizeof(double) );
      shift_reg = new TBuffer<double>( shift_dat, N-1, BM_CIRC );
    }
}

//------------------------------------------------------------------------------
t_pFilter::~t_pFilter(){

    if(shift_reg != NULL){

        delete(shift_reg);
        free(shift_dat);
    }
}

//------------------------------------------------------------------------------
void t_pFilter::Reset(double *_shift_reg){

    if(shift_reg == NULL) return;

    unsigned int wN = N - 1;   //velikost alokace shift_reg!!! - pocatecni podminky se nastavuji jen do v[-1]

    if( _shift_reg != NULL ){

        (void )shift_reg->AddSample(_shift_reg, &wN);
    } else {                   //vynuluje a buffer nastavi na bgn

        for( int i=0; i<(N-1); i++ ) shift_dat[i] = 0.0;
        //(void )shift_reg->SetActual(shift_dat);      //zbytecne
    }
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
double t_CanonFilter::Process(double new_smpl, int *n_to_proc){
//den_coeff[0] se predpoklada byt 1.0 - jedina moznost
   double v_i, v_n = new_smpl, y = 0.0;

    for( int i=1; i<N; i++ ){

       (void )shift_reg->GetSample( &v_i );
       v_n -= coeff_den[N-i] * v_i;
       y   += coeff_num[N-i] * v_i;
    }

    y += coeff_num[0] * v_n;

    if( shift_reg != NULL ) (void )shift_reg->AddSample( v_n );
    if( n_to_proc != NULL ) if( (*n_to_proc = --act_stat) <= 0 ) act_stat = dec_fact;
    return y;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
double t_DelayLine::Process(double new_smpl, int *n_to_proc){

   double *y = NULL;

    if( shift_reg != NULL ){

        shift_reg->AddSample(new_smpl); //zapise a posune ukazatel na nejstarsi polozku
        shift_reg->GetActual(&y); //TBuffer umi vracet jen jeji pointer
    }
    //decimaci muzem nechat
    if( n_to_proc != NULL ) if( (*n_to_proc = --act_stat) <= 0 ) act_stat = dec_fact;
    return((y != NULL) ? *y : 0.0); //jen pro sichr
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
double t_BiQuadFilter::Process(double new_smpl, int *n_to_proc){
//den_coeff[3*n] se predpoklada byt 1.0 - jedina moznost
   double v_i, v_n = new_smpl, y = 0.0;
   //vshift_dat po dvojicich od nejnovejsiho - jinak nez u Canon!!
   for( int n=0; n<N/3; n++ ){

        v_n -= (shift_dat[n*3]*coeff_den[n*3+1] + shift_dat[n*3+1]*coeff_den[n*3+2]);
        y = v_n*coeff_num[n*3] + shift_dat[n*3]*coeff_num[n*3+1] + shift_dat[n*3+1]*coeff_num[n*3+2];

        shift_dat[n*3+1] = shift_dat[n*3];
        shift_dat[n*3] = v_n;
        v_n = y;
   }

   if( n_to_proc != NULL ) if( (*n_to_proc = --act_stat) <= 0 ) act_stat = dec_fact;
   return y;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
double t_DirectFilter::Process(double new_smpl, int *n_to_proc){

    double v_i, y = 0.0;

    if( act_stat == dec_fact ){     //prvni vzorek musi byt spocitan, vzhledem k principu FilterBank

        y = new_smpl * coeff_num[0];
        for( int i=1; i<N; i++ ){

            (void )shift_reg->GetSample( &v_i );
            y += coeff_num[N-i]*v_i;
        }
    }

    if( shift_reg != NULL ) (void )shift_reg->AddSample( new_smpl );
    if( n_to_proc != NULL ) if( (*n_to_proc = --act_stat) <= 0 ) act_stat = dec_fact;

    return y;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//k[0] se neuplatnuje - teoreticky by melo by byt 1.0
double t_LatticeFilter::Process(double new_smpl, int *n_to_proc){

    double f, f_i, g, g_i;

    g = f = new_smpl;
    for( int i=1; i<N; i++ ){

        f_i = f + coeff_num[i]*shift_dat[i-1];
        g_i = f*coeff_num[i] + shift_dat[i-1];

        shift_dat[i-1] = g;
        g = g_i; f = f_i;
    }

   //podle n_to_proc bud decimaci provadej nebo ne
   if( n_to_proc != NULL ) if( (*n_to_proc = --act_stat) <= 0 ) act_stat = dec_fact;
   return g; //nebo f - kontrola nutna
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
/*
class t_FilterBank{

    private:
        HP_filter **t_pFilter;
        LP_filter **t_pFilter;

        double *prep_smpl; //vstupni data pro vyssi stages
        int    *prep_indx; //indexy nalezitosti dat ke stages

        int    M;          //pocet stupnu(stages)
        int    N;          //velikost bufferu

        int    time_indx;
    public:
        int    Process( double new_smpl, double *sum_rslt );

        template<class HP_TYP, class LP_TYP> t_FilterBank( HP_TYP &_HP_filter, LP_TYP &_LP_filter, int _M );
        ~t_FilterBank();
};
*/

t_DiadicFilterBank::~t_DiadicFilterBank(){

     for( int i=0; i<M; i++ ){ delete( HP_filter[i] ); delete( LP_filter[i] ); }
     delete HP_filter; delete LP_filter;
     delete prep_indx; delete prep_smpl;
}
//------------------------------------------------------------------------------

void t_DiadicFilterBank::Reset(){

    time_indx = 0;

    for( int i=0; i<M; i++ ){ HP_filter[i]->Reset(NULL); LP_filter[i]->Reset(NULL); }
    for( int i=0; i<N; i++ ){ prep_smpl[i] = 0.0; prep_indx[i] = 0; }
}
//------------------------------------------------------------------------------

int t_DiadicFilterBank::Process( double new_smpl, double *sum_rslt ){

    double t_wrt_val;
    int t_wrt_ind, to_process;

    //nejvyssi oktava
    sum_rslt[0] = HP_filter[0]->Process( new_smpl, NULL );
    t_wrt_val   = LP_filter[0]->Process( new_smpl, &to_process );
    if( to_process == 1 ){  //musime brat prvni vzorek jako sudy. liche vzorky by se nedali umistovat v cyklu

        prep_smpl[time_indx+1] = t_wrt_val;
        prep_indx[time_indx+1] = 1;
    }

    int m = prep_indx[time_indx];

    if(time_indx > 0){

        sum_rslt[m] = HP_filter[m]->Process( prep_smpl[time_indx], NULL );
        t_wrt_val   = LP_filter[m]->Process( prep_smpl[time_indx], &to_process );
        if(m < (M-1)){

            if(to_process == 1){      //aby posledni platny stupen na (poz. t = N/2) uz dalsi pozici nezapisoval

                t_wrt_ind = time_indx + (0x1 << (m-1));
                prep_smpl[t_wrt_ind] = t_wrt_val;
                prep_indx[t_wrt_ind] = m+1;
            }
        } else prep_smpl[m+1] = t_wrt_val; //backup, vysledek posledni LP filtrace (u CPB zahazujem)
    } else sum_rslt[m] = prep_smpl[m+1]; //vratime jako vysledek filtrace v poseldnim stupni (tak aby sme vzdy meli 2 vysledky, muzem nemusime zpracovavat)

    time_indx = (time_indx+1)%N;
    return m;       //pri vypoctu prvniho stupne se vraci 0, protoze je updatovan jen jeden stupen, stupen 0!!! (+ posledni LP filtrace)
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//naprosto nezbytne je aby mel zadany lp i hp filtr nastavenou decimaci na 2!!!!
t_PacketTree::t_PacketTree(t_DirectFilter &_HP_filter,t_DirectFilter &_LP_filter, int _M ){

    for(int i=0; i<PACKETTREE_MAX_STAGES; i++){

        HP_filter[i] = NULL; LP_filter[i] = NULL;
    }

    if( (M = _M) > 0 ){

       for(int i=0; i<M; i++){ //vytvorime trojuhelnikovou matici filtru

            HP_filter[i] = new t_DirectFilter*[(1<<i)];
            LP_filter[i] = new t_DirectFilter*[(1<<i)];
            for(int j=0; j<(1<<i); j++){

                HP_filter[i][j] = new t_DirectFilter(_HP_filter);
                LP_filter[i][j] = new t_DirectFilter(_LP_filter);
            }
        }
    }
}
//------------------------------------------------------------------------------
int t_PacketTree::Process(double new_smpl, double *sum_rslt){

    int i, j, to_process = 1;
    int rev_lo, rev_hi;
    double part_rslt[1<<PACKETTREE_MAX_STAGES];
    double back_rslt[1<<PACKETTREE_MAX_STAGES];  //zde budem drzet mezivysledky, nastaveno na max pocet koef.

    back_rslt[0] = new_smpl;
    for(i=0; (i<M) && (to_process == 1); i++){  //dokud pocitame jen liche tak postupujem po stages, maximalne do M

        rev_hi = 1; rev_lo = 0;
        for(j=0; j<(1<<i); j++){

            part_rslt[2*j+rev_lo] = LP_filter[i][j]->Process(back_rslt[j], &to_process);
            part_rslt[2*j+rev_hi] = HP_filter[i][j]->Process(back_rslt[j], &to_process);  //&to_process by melo byt u obou stejne

            if(rev_hi){

                rev_hi = 0; rev_lo = 1;
            } else {

                rev_hi = 1; rev_lo = 0;
            }
        }
        memcpy(back_rslt, part_rslt, sizeof(double)*j*2);
    }

    if(to_process)  //i u posledniho stupne musime hledet na decimaci - ma ji nastavenou natvrdo (kazdy 2. vz. vraci 0)
        memcpy(sum_rslt, back_rslt, sizeof(double)*(1<<M));   //posledni kopyrovani do vystuponiho pole

    return to_process;  //1 dostali sme se az na konec
}
//------------------------------------------------------------------------------
t_PacketTree::~t_PacketTree(){

    for( int i=0; i<M; i++ ){ //znicime trojuhelnikovou matici filtru

        for(int j=0; j<(1<<i); j++){

            if(HP_filter[i][j]) delete(HP_filter[i][j]);
            if(LP_filter[i][j]) delete(LP_filter[i][j]);
        }

        if(HP_filter[i]) delete(HP_filter[i]);
        if(LP_filter[i]) delete(LP_filter[i]);
    }
}
//------------------------------------------------------------------------------

void t_PacketTree::Reset(){

    for( int i=0; i<M; i++ ) for(int j=0; j<(1<<i); j++){

        HP_filter[i][j]->Reset(NULL);
        LP_filter[i][j]->Reset(NULL);
    }
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
unsigned int BitReverz( unsigned int cislo, int pow ){

    unsigned long pom=0;

    for(unsigned int i=0; i<pow; i++){ pom |= ((cislo>>i) & 0x1); pom <<= 1; }

    return ((unsigned int)(pom>>1));
}
//------------------------------------------------------------------------------
//KLASICKA FFT
t_Compl *my_fft( double *data_r, double *data_i, unsigned int *Nd ) {

        int i_fft=0, i_TwF=0, L=0;
        int M = (int)ceil(log(*Nd)/log(2.00));
        int N = 1<<M;
        t_Compl Left, Rigth;

        t_Compl* fft = (t_Compl *)malloc( N*sizeof(t_Compl) );
        t_Compl* TwF = (t_Compl *)malloc( N/2*sizeof(t_Compl) );

        if( data_i != NULL ){

                for(int inx, n=0; n<N ; n++)
                        if( (inx = BitReverz(n,M)) < *Nd ){ fft[n] = t_Compl( data_r[inx], data_i[inx] );}
                        else fft[n] = t_Compl( 0.00, 0.00 );

        } else {

                for(int inx, n=0; n<N ; n++)
                        if( (inx = BitReverz(n,M)) < *Nd ){ fft[n] = t_Compl( data_r[inx], 0 );}
                        else fft[n] = t_Compl( 0.00, 0.00 );
        }

        for(int n=0; n<N/2; n++) TwF[n] = t_Compl( cos(-2*PI/N*n), sin(-2*PI/N*n) );

        for(int k=0; k<M; k++){

                L=1<<k; i_TwF=0;
                for(int m=0; m<(N/2/L); m++ ){

                        for(int l=0; l<L; l++ ){

                                i_fft = 2*m*L+l;
                                Left = TwF[i_TwF]*fft[i_fft+L]; Rigth = fft[i_fft];
                                fft[i_fft] = Rigth+Left; fft[i_fft+L] = Rigth-Left;
                                i_TwF = (i_TwF + (1<<(M-k-1)) ) % (N/2);
                        }
                }
        }

        *Nd = N;
        delete(TwF);
        return(fft);
}
//------------------------------------------------------------------------------
//FFT DOPLNENA O OKYNKA
t_Compl *my_fft( double *data_r, double *data_i, unsigned int *Nd, int w_typ ){

        int i_fft=0, i_TwF=0, L=0;
        int M = (int)ceil(log(*Nd)/log(2.00));
        int N = 1<<M;
        t_Compl Left, Rigth;

        t_Compl* fft = (t_Compl *)malloc( N*sizeof(t_Compl) );
        t_Compl* TwF = (t_Compl *)malloc( N/2*sizeof(t_Compl) );
        double *win = new double[N]; gen_window( win, N, w_typ );

        if( data_i != NULL ){

                for(int inx, n=0; n<N ; n++)
                        if( (inx = BitReverz(n,M)) < *Nd ){ fft[n] = t_Compl( win[inx]*data_r[inx], win[inx]*data_i[inx] );}
                        else fft[n] = t_Compl( 0.00, 0.00 );

        } else {

                for(int inx, n=0; n<N ; n++)
                        if( (inx = BitReverz(n,M)) < *Nd ){ fft[n] = t_Compl( win[inx]*data_r[inx], 0 );}
                        else fft[n] = t_Compl( 0.00, 0.00 );
        }

        for(int n=0; n<N/2; n++) TwF[n] = t_Compl( cos(-2*PI/N*n), sin(-2*PI/N*n) );

        for(int k=0; k<M; k++){

                L=1<<k; i_TwF=0;
                for(int m=0; m<(N/2/L); m++ ){

                        for(int l=0; l<L; l++ ){

                                i_fft = 2*m*L+l;
                                Left = TwF[i_TwF]*fft[i_fft+L]; Rigth = fft[i_fft];
                                fft[i_fft] = Rigth+Left; fft[i_fft+L] = Rigth-Left;
                                i_TwF = (i_TwF + (1<<(M-k-1)) ) % (N/2);
                          }
                }
        }

        *Nd = N;
        delete(TwF); delete(win);
        return(fft);
}

//------------------------------------------------------------------------------
//FFT pro opakovane pouziti
t_Compl *my_fft(double *r_data, double *i_data, FFT_CONTEXT *con){

        int i_fft, i_TwF, L;
        t_Compl Left, Rigth;

        if(con == NULL)
            return NULL;

        t_Compl *fft = (t_Compl *)malloc(con->N*sizeof(t_Compl));
        for(int i, n = 0; n<con->N; n++){

            i = con->ExI[n];
            fft[n] = t_Compl((r_data != NULL) ? con->Win[i]*r_data[i] : 0, (i_data != NULL) ? con->Win[i]*i_data[i] : 0);
        }
        for(int k = 0; k<con->M; k++){

                L=1<<k; i_TwF=0;
                for(int m = 0; m<(con->N/2/L); m++){

                        for(int l=0; l<L; l++ ){

                                i_fft = 2*m*L+l;
                                Left = con->TwF[i_TwF]*fft[i_fft+L]; Rigth = fft[i_fft];
                                fft[i_fft] = Rigth+Left; fft[i_fft+L] = Rigth-Left;
                                i_TwF = (i_TwF + (1<<(con->M-k-1))) % (con->N/2);
                          }
                }
        }

        return(fft);
}
//------------------------------------------------------------------------------
void gen_window( double *rslt, int N, unsigned int type ){

  //koduje treba y-zoom nebo sigmu u gausse
  double  temp, param = (((unsigned int)(type & FFT_MODE_OPT_PAR_MASC)) >> 16) / 65536.0;  //<0,65536>;

   switch ( type & FFT_MODE_OPT_WIN_MASC ){

      case WINDOW_HANN:        //w = 1 - cos(2*pi*(0:m-1)'/(n-1)));
                for( int i=0; i<N; i++ ) rslt[i] = 2*(0.5 - 0.5*cos( 2*PI*i/N ));
      break;
      case WINDOW_HAMM:        //w = (54 - 46*cos(2*pi*(0:m-1)'/(n-1)))/100;
                for( int i=0; i<N; i++ ) rslt[i] =2*(0.54 - 0.46*cos( 2*PI*i/N ));
      break;
      case WINDOW_BLCK:        //w = (42 - 50*cos(2*pi*(0:m-1)/(n-1)) + 8*cos(4*pi*(0:m-1)/(n-1)))'/100;
                for( int i=0; i<N; i++ ) rslt[i] = 2*(0.42 - 0.50*cos( 2*PI*i/N ) - 0.08*cos( 4*PI*i/N ));
      break;
      case WINDOW_RECT:
                for( int i=0; i<N; i++ ) rslt[i] = 1;
      break;
      case WINDOW_FLAT:        //w = 1 - 1.98*cos(2*Pi*(0:m-1)/(n-1)) + 1.29*cos(4*Pi*(0:m-1)/(n-1)) - 0.388*cos(6*Pi*(0:m-1)/(n-1)) + 0.0322*cos(8*Pi*t/T);
                for( int i=0; i<N; i++ ) rslt[i] = (1 - 1.98*cos( 2*PI*i/N ) + 1.29*cos( 4*PI*i/N ) - 0.388*cos( 6*PI*i/N ) + 0.0322*cos( 8*PI*i/N ));
      break;
      case WINDOW_BRTL:
                for( int i=0; i<N; i++ ) rslt[i] = (2 - fabs(4*(i-N/2)/N));
      break;
      case WINDOW_GAUS:         //w = exp(-((-Nw/2:(Nw/2-1))/(Sigma*Nw)).^2);
                for( int i=0; i<N; i++ ){
                    temp = (i-N/2.0)/(param*N);    //zpocitame si predem exponent
                    rslt[i] = exp(-(temp*temp));
                }
      break;
   }
}
/*
    int     N;   //pocet bodu FFT (v mocnine 2)
    int     M;   //mocnina 2

    t_Compl *TwF; //tabelace twidle factoru 1..N/2
    double *Win; //tabelace okynka
    int *ExI; //tabelace idexu prohazeni vzorku
*/

/*
//------------------------------------------------------------------------------
int init_fftcontext(FFT_CONTEXT *con, int _N_FFT, int _WIN, int WinComp, int OvrComp){

    t_Compl *_TwF;
    double *_Win;
    int  *_ExI;

    con->Norm = (WinComp > 0) ? 1.0 : -1.0;    //pokud se maji pocitat pak se inicializuji - jinak -1
    con->Over = (OvrComp > 0) ? 1.0 : -1.0;

    if(con->N != _N_FFT){

        con->N = 1; con->M = 0;
        while((_N_FFT >>= 1) > 0){ con->N <<= 1; con->M += 1; }// takto ziskame mocninu 2

        _TwF = (t_Compl *)malloc(con->N/2*sizeof(t_Compl));
        _Win = (double *)malloc(con->N*sizeof(double));
        _ExI = (int *)malloc(con->N*sizeof(int));

        if((_TwF == NULL) || (_Win == NULL) || (_ExI == NULL))
            return 0;

        for(int n=0; n<con->N/2; n++) _TwF[n] = t_Compl(cos(-2*PI/con->N*n), sin(-2*PI/con->N*n));
        for(int n=0; n<con->N;   n++) _ExI[n] = BitReverz(n, con->M);

        if(con->TwF) free(con->TwF); con->TwF = _TwF;
        if(con->Win) free(con->Win); con->Win = _Win;
        if(con->ExI) free(con->ExI); con->ExI = _ExI;

    }

    rebuild_fftcontext(con, _WIN);     //okynko + normalizace  + prekryti
    return 1;
}

//------------------------------------------------------------------------------
void rebuild_fftcontext(FFT_CONTEXT *con, int _WIN){

    gen_window(con->Win, con->N, _WIN);
    double Max;   

        if(con->Norm > 0){     //kompenzace je pozadovana - byla uz pocitana? jo - pocitame zas

            con->Norm = 0;
            for(int n=0; n<con->N; n++) con->Norm += con->Win[n];
            for(int n=0; n<con->N; n++) con->Win[n] /= con->Norm;
        }

        Max = con->Win[con->N/2];  //melo by byt - okynka jsou symetricka
        if(con->Over > 0){   //pozadovan jeste i vypocet prekryti?

            con->Over = 0;
            for(int i = 0; (con->Win[i]/Max < 0.5) && (i < (con->N/2-1)); i++) con->Over += 1.0;
            con->Over = double(2*con->Over) / con->N;  //(0, 1) - jde o skutecne prekryti, ne posuv
        }
}

//------------------------------------------------------------------------------
void destroy_fftcontext(FFT_CONTEXT *con){

        if(con->TwF) free(con->TwF); con->TwF = NULL;
        if(con->Win) free(con->Win); con->Win = NULL;
        if(con->ExI) free(con->ExI); con->ExI = NULL;

        con->N = 0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#define AGAUSS_REF_M    1024     //na kolik poctu bodu jsou napocirtany tabulky? (limituje presnost mereni B)
void init_agausscontext(AGAUSS_CONTEXT *con,  char *path){

    int ret = 1;
    t_DatFile *imp_file;
    t_DatHeader imp_size;

    char part_path[512];
    memset(&imp_size, 0, sizeof(t_DatHeader));

    //natahnem tabelovane sirky pasem (0, 1)
    sprintf(part_path, "%s\\sigma_meas_F.txt", path);
    imp_file = (t_DatFile *) new t_DatFile(part_path, "rb", false);
    imp_file->getInfo(&imp_size);
    con->nF = (imp_size.n_clm > imp_size.n_row) ? imp_size.n_clm : imp_size.n_row;
    con->Ftab = (double *) new double[con->nF];
    if(con->nF != imp_file->Read(con->Ftab, con->nF)) ret = 0;
    delete(imp_file); imp_file = NULL;

    //natahnem tabelovane sirky oken
    sprintf(part_path, "%s\\sigma_meas_S.txt", path);
    imp_file = (t_DatFile *) new t_DatFile(part_path, "rb", false);
    imp_file->getInfo(&imp_size);
    con->nS = (imp_size.n_clm > imp_size.n_row) ? imp_size.n_clm : imp_size.n_row;
    con->Stab = (double *) new double[con->nS];
    if(con->nS != imp_file->Read(con->Stab, con->nS)) ret = 0;
    delete(imp_file); imp_file = NULL;

    //prevodni tabulka 1. Sopt = f(F)
    sprintf(part_path, "%s\\sigma_opt_vs_B.txt", path);
    imp_file = (t_DatFile *) new t_DatFile(part_path, "rb", false);
    imp_file->getInfo(&imp_size);
    con->nB = (imp_size.n_clm > imp_size.n_row) ? imp_size.n_clm : imp_size.n_row;
    con->Sopt_vs_F[0] = (double *) new double[con->nB];
    con->Sopt_vs_F[1] = (double *) new double[con->nB];
    if(con->nB != imp_file->ReadLine(con->Sopt_vs_F[0], con->nB)) ret = 0;
    if(con->nB != imp_file->ReadLine(con->Sopt_vs_F[1], con->nB)) ret = 0;
    delete(imp_file); imp_file = NULL;

    //prevodni tabulka 2. F = f(Smeas, S)
    sprintf(part_path, "%s\\sigma_meas_vs_B.txt", path);
    imp_file = (t_DatFile *) new t_DatFile(part_path, "rb", false);
    imp_file->getInfo(&imp_size);
    if((imp_size.n_clm*imp_size.n_row) == (con->nF*con->nS)){

        con->Smeas_vs_F = (double **) new double*[con->nS];
        for(int i=0; i<con->nS; i++) con->Smeas_vs_F[i] = (double *) new double[con->nF];
        if((con->nS*con->nF) != imp_file->Read(con->Smeas_vs_F, con->nS*con->nF)) ret = 0;
    } else ret = 0;
    delete(imp_file); imp_file = NULL;

    if(ret == 0){
        //nemame nic
        destroy_agausscontext(con);
    } else {
        //interni formatovani navic - vime ze ref. pocet bodu FFT byl 1024 - musime normovat
        for(int i=0; i<con->nB; i++) con->Sopt_vs_F[0][i] *= AGAUSS_REF_M;
        for(int i=0; i<con->nF; i++) con->Ftab[i] *= AGAUSS_REF_M;
        for(int i=0; i<con->nF; i++) for(int j=0; j<con->nS; j++) con->Smeas_vs_F[j][i] *= AGAUSS_REF_M;
    }

    con->I = con->nS - 1;  //prave pouzivany index - zacimane s nejsirsim okenkem
}

//------------------------------------------------------------------------------
void destroy_agausscontext(AGAUSS_CONTEXT *con){

    if(con->Sopt_vs_F){

        for(int i=0; i<con->nS; i++) delete(con->Smeas_vs_F[i]);
        delete(con->Smeas_vs_F); con->Smeas_vs_F = NULL;
    }

    if(con->Sopt_vs_F[0]) delete(con->Sopt_vs_F[0]); con->Sopt_vs_F[0] = NULL;
    if(con->Sopt_vs_F[1]) delete(con->Sopt_vs_F[1]); con->Sopt_vs_F[1] = NULL;

    if(con->Stab) delete(con->Stab); con->Stab = NULL;
    if(con->Ftab) delete(con->Ftab); con->Ftab = NULL;

    con->nF = 0;
    con->nS = 0;
    con->nB = 1;
    con->I = 0;
}

//--------------------------------------------------------------------------
//kontext, i/o - parametry okenka, zmerene fft, pocet bodu
double process_agauss(AGAUSS_CONTEXT *con, double *fft, int M){

  double S_n, S_m = 0, S_lo = 0, S_hi = 0;  //<0,65536>;

  int n = 0, j, MaxI = FindMax<double>(fft, M);  //max k vypoctu optima

  double B;
  double wfft[AGAUSS_REF_M*2];     //nema cenu delsi protoze ref. tabulku mame jen do 1024 bodu
  double waxs[AGAUSS_REF_M*2];

    if(M > AGAUSS_REF_M*2) M = AGAUSS_REF_M*2;
    for(int i=0; i<M; i++) waxs[i] = i;  //vypocet normy a indexova osa

    if(MaxI < (M-3)){ //podminka min poctu bodu (kdyz bude peak uplne vlevo)

        for(int i=MaxI; i<M; i++) wfft[i-MaxI] = fft[i]/fft[MaxI];
        S_lo = LinEval2(0.3679, M-MaxI, waxs, wfft);  //pravostranny odhad
        S_hi = LinEval2(0.7788, M-MaxI, waxs, wfft);
        n += 1;
    }

    if(MaxI > 3){ //podminka min poctu bodu (kdyz bude peak uplne vpravo)

        for(int i=MaxI; i>=0; i--) wfft[MaxI-i] = fft[i]/fft[MaxI];
        S_lo += LinEval2(0.3679, MaxI, waxs, wfft);   //levostranny odhad
        S_hi += LinEval2(0.7788, MaxI, waxs, wfft);
        n += 1;
    }

    S_m = (S_lo + S_hi*2) / (2*n);  //nejake Ska se vypocitali - zprumerujem

    for(j=con->nF-1; j>=0; j--) if(con->Smeas_vs_F[con->I][j] < S_m) break;  //budem prohledavat odzhora (na zacatku mohou byt pozkazene zaznamy)
    if(j >= 1){     //muze se stat ze hodnota S_m je mensi nez predpokladana teor. (mnesi nez mame - v pripade ze je tam sum muze byt)

        B = LinEval2(S_m, con->nF-j, &con->Ftab[j], &con->Smeas_vs_F[con->I][j]);    //najdeme skutecnou sirku pasma signalu (bez okenka)
        S_n = LinEval2(B, con->nB, con->Sopt_vs_F[1], con->Sopt_vs_F[0]);   //otimalni S k siri pasma signalu
        con->I = FindNearest<double>(S_n, con->Stab, con->nS);  //nejblizsi tabelovana hodnota (zalohujem si ji v kontextu)
    }   //jinak nechavame jakou mame

    return con->Stab[con->I];
}
*/
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
