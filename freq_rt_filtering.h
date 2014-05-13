#ifndef FREQ_RT_FILTERING_H
#define FREQ_RT_FILTERING_H

#include <stdint.h>
#include <vector>

/*! \class template of t_pFilter
 * \brief - pure virtual ancestor of SISO digital filter
 * deep copy of filter coeficient is made
 * filter impementation depends on successor as well as order of coeficinets
 */
template <class T> class t_pFilter {

    protected:
        TBuffer<T>   *shift_reg;   //vytvari se instance
        T            *shift_dat;

        std::vector<T> coeff_num;
        std::vector<T> coeff_den;

        int32_t N;            //pocet koeficientu

        int32_t dec_fact;     //mozny je jen faktor > 0, < 0 nemaji uplne koser vyznam
        int32_t act_stat;
        int32_t typ_filt;

    public:
        /*! \brief - defines behaviour of filter */
        virtual T Process(const T new_smpl, int32_t *n_to_proc) = 0;

        /*! \brief - backward identification */
        int32_t GetTyp(){

            return(typ_filt);
        }

        /*! \brief - inicialize shift register / or delete it if NULL
         * \warning - size of _shift_reg is expected to be N-1 (!)
         */
        void Reset(const T *_shift_reg){

            int32_t wN = N - 1;
            if((wN > 0) && (shift_reg == NULL))
                return;

            if(_shift_reg != NULL ){

                shift_reg->AddSample(_shift_reg, (uint32_t *)&wN);
            } else { //vynuluje a buffer nastavi na bgn

                for(int32_t i=0; i<wN; i++)
                    shift_dat[i] = T(0);
            }
        }

        /*! \brief - copy of existing, except delay line */
        t_pFilter(const t_pFilter<T> &src_coeff_cpy):
            coeff_num(src_coeff_cpy.coeff_num),
            coeff_den(src_coeff_cpy.coeff_den),
            dec_fact(src_coeff_cpy.dec_fact)
        {

            shift_reg = NULL;
            act_stat  = src_coeff_cpy.dec_fact;
            typ_filt  = src_coeff_cpy.typ_filt;

            if((N = src_coeff_cpy.N) > 1){

              shift_dat = (T *)calloc(N-1, sizeof(T));
              shift_reg = new TBuffer<T>(shift_dat, N-1, BM_CIRC);
            }
        }

        /*! \brief - new flter definition (including decimation factor) */
        t_pFilter(const T *_coeff_num, const T *_coeff_den, int32_t _N, int32_t _dec_fact = 1)
        {
            if(_coeff_num) coeff_num = std::vector<T>(_coeff_num, &_coeff_num[_N]);
            if(_coeff_den) coeff_den = std::vector<T>(_coeff_den, &_coeff_den[_N]);

            dec_fact = _dec_fact;

            shift_reg = NULL;
            act_stat  = dec_fact;
            this->typ_filt = -1;

            if( (N = _N) > 1 ){

              shift_dat = (T *)calloc(N-1, sizeof(T));
              shift_reg = new TBuffer<T>(shift_dat, N-1, BM_CIRC);
            }
        }

        /*! \brief - free previsous allocation */
        virtual ~t_pFilter(){

            if(shift_reg != NULL){

                delete(shift_reg);
                free(shift_dat);
            }
        }
};


/*! \class template of t_DelayLine
 * \brief - simple delay buffer of length N, ceficients doesn't play any role
 * supports decimation
 */
template <class T> class t_DelayLine : public t_pFilter<T> {

    public:
        /*! \brief - simple delay buffer, ceficients doesn't play any role */
        T Process(const T new_smpl, int32_t *n_to_proc){

            T *y = NULL;
            if(this->shift_reg != NULL){

                this->shift_reg->AddSample(new_smpl);
                this->shift_reg->GetActual(&y);
            }

            if(n_to_proc != NULL )
                if((*n_to_proc = --this->act_stat) <= 0)
                    this->act_stat = this->dec_fact;

            return((y != NULL) ? *y : T());
        }

        /*! \brief - copy constructor */
        t_DelayLine(const t_pFilter<T> &src_coeff_cpy)
                      :t_pFilter<T>(src_coeff_cpy)
        {
            this->typ_filt = FIR_DELAYLN_STRUCT;
        }

        /*! \brief - new delay line */
        t_DelayLine(int32_t _N, int32_t _dec_fact = 1)
                      :t_pFilter<T>(NULL, NULL, _N+1, _dec_fact)
        {
            this->typ_filt = FIR_DELAYLN_STRUCT;
        }

        ~t_DelayLine(){;}
};

/*! \class template of t_CanonFilter IIR form
 * \brief - canonnical filter structure - direct form of 2nd class, N-1 is order of filter => N coeff is needed
 * \warning - den_coeff[0] must be 1.0 (!)
 */
template <class T> class t_CanonFilter : public t_pFilter<T> {

    public:
        /*! \brief - decimation throw up */
        T Process(const T new_smpl, int32_t *n_to_proc){

            T v_i, v_n = new_smpl, y = T(0);

            for(int32_t i=1; i<this->N; i++){

                this->shift_reg->GetSample(&v_i);
                v_n -= this->coeff_den[this->N-i] * v_i;
                y   += this->coeff_num[this->N-i] * v_i;
            }

            y += this->coeff_num[0] * v_n;
            this->shift_reg->AddSample(v_n);

            if(n_to_proc != NULL)
                if((*n_to_proc = --this->act_stat) <= 0)
                    this->act_stat = this->dec_fact;
            return y;
        }

        t_CanonFilter(const t_pFilter<T> &src_coeff_cpy)
                      :t_pFilter<T>(src_coeff_cpy)
        {
            this->typ_filt = IIR_DIRECT1_STRUCT;
        }

        t_CanonFilter(const T *_coeff_num, const T *_coeff_den, int32_t _N, int32_t _dec_fact = 1)
                      :t_pFilter<T>(_coeff_num, _coeff_den, _N, _dec_fact)
        {
            this->typ_filt = IIR_DIRECT1_STRUCT;
        }

        ~t_CanonFilter(){;}
};

/*! \class - template of t_BiQuadFilter IIR form
 * \brief - biquadratic form consisted of 2nd order stages
 * N id number of stages!
 * coeficinet has to be order in triplets
 * \warning - den_coeff[3*n] coeficient suppos to be 1.0 (!)
 */
template <class T> class t_BiQuadFilter : public t_pFilter<T>{

    public:

        T Process(const T new_smpl, int32_t *n_to_proc){

           T v_n = new_smpl, y = T(0);
           //vshift_dat po dvojicich od nejnovejsiho - jinak nez u Canon!!
           for(int32_t n=0; n<this->N/3; n++ ){

                v_n -= (this->shift_dat[n*3]*this->coeff_den[n*3+1] +
                        this->shift_dat[n*3+1]*this->coeff_den[n*3+2]);

                y = v_n*this->coeff_num[n*3] +
                        this->shift_dat[n*3]*this->coeff_num[n*3+1] +
                        this->shift_dat[n*3+1]*this->coeff_num[n*3+2];

                this->shift_dat[n*3+1] = this->shift_dat[n*3];
                this->shift_dat[n*3] = v_n;
                v_n = y;
           }

           if(n_to_proc != NULL)
               if((*n_to_proc = --this->act_stat) <= 0)
                   this->act_stat = this->dec_fact;

           return y;
        }

        t_BiQuadFilter(const t_pFilter<T> &src_coeff_cpy)
                       :t_pFilter<T>(src_coeff_cpy){
            this->typ_filt = IIR_BIQUADR_STRUCT;
        }

        t_BiQuadFilter(const T *_coeff_num, const T *_coeff_den, int32_t _N, int32_t _dec_fact = 1)
                       :t_pFilter<T>(_coeff_num, _coeff_den, 3*_N, _dec_fact){
            this->typ_filt = IIR_BIQUADR_STRUCT;
        }

        ~t_BiQuadFilter(){;}
};

/*! \class - template of t_DirectFilter FIR
 * \brief - direct FIR form with efficient decimator implementation
 * N is number of taps
 */
template <class T> class t_DirectFilter : public t_pFilter<T>{

    public:
        T Process(const T new_smpl, int32_t *n_to_proc){

            T v_i, y = T(0);

            if( --this->act_stat <= 0 ){

                y = new_smpl * this->coeff_num[0];
                this->act_stat = this->dec_fact;

                for(int32_t i=1; i<this->N; i++ ){

                        this->shift_reg->GetSample(&v_i);
                        y += this->coeff_num[this->N-i]*v_i;
                }
            }

            this->shift_reg->AddSample(new_smpl);

            if(n_to_proc != NULL)
                *n_to_proc = this->act_stat;

            return y;
        }

        t_DirectFilter(const t_pFilter<T> &src_coeff_cpy)
                       :t_pFilter<T>(src_coeff_cpy){
            this->typ_filt = FIR_DIRECT1_STRUCT;
        }

        t_DirectFilter(const T *_coeff_num, int32_t _N, int32_t _dec_fact = 1)
                       :t_pFilter<T>(_coeff_num, NULL, _N, _dec_fact){
            this->typ_filt = FIR_DIRECT1_STRUCT;
        }

        ~t_DirectFilter(){;}
};

/*! \class - template of t_LatticeFilter FIR
 * \brief - lattice structure FIR form
 * not tested yet!
 * \warning - k[0] is ommited (suppose to be 1!)
 */
template <class T> class t_LatticeFilter : public t_pFilter<T>{
    public:
        T Process(const T new_smpl, int32_t *n_to_proc){

            T f, f_i, g, g_i;

            g = f = new_smpl;
            for(int32_t i=1; i<this->N; i++ ){

                f_i = f + this->coeff_num[i]*this->shift_dat[i-1];
                g_i = f*this->coeff_num[i] + this->shift_dat[i-1];

                this->shift_dat[i-1] = g;
                g = g_i; f = f_i;
            }

           if(n_to_proc != NULL)
               if((*n_to_proc = --this->act_stat) <= 0)
                   this->act_stat = this->dec_fact;

           return g; //or f (?!)
        }


        t_LatticeFilter(t_pFilter<T> &src_coeff_cpy)
                       :t_pFilter<T>(src_coeff_cpy){
            this->typ_filt = FIR_LATTICE_STRUCT;
        }

        t_LatticeFilter(const T *_coeff_num, int32_t _N, int32_t _dec_fact = 1)
                       :t_pFilter<T>(_coeff_num, NULL, _N, _dec_fact){
            this->typ_filt = FIR_LATTICE_STRUCT;
        }

        ~t_LatticeFilter(){;}
};

/*! \class - template of t_DiadicFilterBank SIMO system
 * \brief - realize decimation tree with diadic step while LP_filter decimate with factor 2 (and it should)
 * impemented algorithm is produce one 2 samples per sample - duration of every Process is constant and limited.
 * for resamplink line only the HP filter should be identity function
 * M - number of stages
 */
template <class T> class t_DiadicFilterBank {

    private:
        t_pFilter<T> **HP_filter;
        t_pFilter<T> **LP_filter;

        T          *prep_smpl; //output data for higher stage
        int32_t    *prep_indx; //indexes

        int32_t    M;          //number of stages
        int32_t    N;          //internal buffer size

        int32_t    time_indx;
    public:
        /*! \brief - computes highest and one of lower stage
         * \param - sum_rslr is external buffer where output of all stages can be stored (M size is assumed)
         * \returns - number of additional stage; fist call reurns 0, because only highest band is updated (stage0!!!)
         */
        int32_t Process(const T new_smpl, T *sum_rslt){

            T t_wrt_val;
            int32_t t_wrt_ind, to_process;

            //nejvyssi oktava
            sum_rslt[0] = HP_filter[0]->Process( new_smpl, NULL );
            t_wrt_val   = LP_filter[0]->Process( new_smpl, &to_process );
            if( to_process == 1 ){  //musime brat prvni vzorek jako sudy. liche vzorky by se nedali umistovat v cyklu

                prep_smpl[time_indx+1] = t_wrt_val;
                prep_indx[time_indx+1] = 1;
            }

            int32_t m = prep_indx[time_indx];

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

        /*! \brief - Reset history (internal and filters)
         */
        void Reset(){

            time_indx = 0;
            for(uint32_t i=0; i<M; i++){

                HP_filter[i]->Reset(NULL);
                LP_filter[i]->Reset(NULL);
            }

            for(uint32_t i=0; i<N; i++){

                prep_smpl[i] = T(0);
                prep_indx[i] = 0;
            }
        }

        /*! \brief - allocate resampling LP line and analytic HP filter according to given teplates
         */
        template<template<class> class LP, template<class> class HP>
            t_DiadicFilterBank(const LP<T> &_HP_filter, const HP<T> &_LP_filter, int32_t _M ){

            time_indx = 0;
            if((M = _M) > 0){

                N = 0x1 << (M-1);
                HP_filter = new t_pFilter<T>*[M];
                LP_filter = new t_pFilter<T>*[M];

                prep_smpl = new T[N+1]; //+1 for last LP filtration (unused for CPB for example)
                prep_indx = new int32_t[N];

                for(int32_t i=0; i<M; i++){

                    HP_filter[i] = new HP<T>(_HP_filter);
                    LP_filter[i] = new LP<T>(_LP_filter);
                }

                Reset();
            }
        }

        /*! \brief - destroy filters and imed buffers
         */
        ~t_DiadicFilterBank(){

            for( int i=0; i<N; i++ ){

                delete(HP_filter[i]);
                delete(LP_filter[i]);
            }

            delete HP_filter; delete LP_filter;
            delete prep_indx; delete prep_smpl;
        }

};

/*! \class Packet (Wavelet) Tree
 * \warning - it has not harmonic proccess as diadic tree (every Proceess() call takes different time)
 * it is constricted only to FIR filters
 * decimation factor for given filter HP and LP filters has to be in 2 (!)
 */
template <class T, int32_t PACKETTREE_MAX_STAGES> class t_PacketTree{

    private:
        t_DirectFilter<T> **HP_filter[PACKETTREE_MAX_STAGES];
        t_DirectFilter<T> **LP_filter[PACKETTREE_MAX_STAGES];
        int32_t  M; //actual number of stages

    public:

        /*! \brief - Process one sample
         *  \return - 1 if we reach the last stage, results in sum_rslt are valid now
         *  size of output buffer rum_rslt must be at least 2^M (!)
         */
        int32_t Process(const T new_smpl, T *sum_rslt){

            int32_t i, j, to_process = 1;
            int32_t rev_lo, rev_hi;
            T part_rslt[1<<PACKETTREE_MAX_STAGES];
            T back_rslt[1<<PACKETTREE_MAX_STAGES];  //zde budem drzet mezivysledky, nastaveno na max pocet koef.

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
                memcpy(back_rslt, part_rslt, sizeof(T)*j*2);
            }

            if(to_process)  //i u posledniho stupne musime hledet na decimaci - ma ji nastavenou natvrdo (kazdy 2. vz. vraci 0)
                memcpy(sum_rslt, back_rslt, sizeof(T)*(1<<M));   //posledni kopyrovani do vystuponiho pole

            return to_process;  //1 dostali sme se az na konec
        }

        /*! \brief - Resets all filters
         */
        void Reset(){

            for(int32_t i=0; i<M; i++ )
                for(int32_t j=0; j<(1<<i); j++){

                    HP_filter[i][j]->Reset(NULL);
                    LP_filter[i][j]->Reset(NULL);
                }
        }

        /*! \brief - creates all M^2 packet tree filters
         */
        t_PacketTree(t_DirectFilter<T> &_HP_filter, t_DirectFilter<T> &_LP_filter, int32_t _M){

            for(int32_t i=0; i<PACKETTREE_MAX_STAGES; i++){

                HP_filter[i] = NULL; LP_filter[i] = NULL;
            }

            if((M = _M) > 0){

               for(int32_t i=0; i<M; i++){ //vytvorime trojuhelnikovou matici filtru

                    HP_filter[i] = new t_DirectFilter<T>*[(1<<i)];
                    LP_filter[i] = new t_DirectFilter<T>*[(1<<i)];
                    for(int32_t j=0; j<(1<<i); j++){

                        HP_filter[i][j] = new t_DirectFilter<T>(_HP_filter);
                        LP_filter[i][j] = new t_DirectFilter<T>(_LP_filter);
                    }
                }
            }
        }

        /*! \brief - destroy all filters
         */
        ~t_PacketTree(){

            for(int32_t i=0; i<M; i++ ){ //znicime trojuhelnikovou matici filtru

                for(int32_t j=0; j<(1<<i); j++){

                    if(HP_filter[i][j]) delete(HP_filter[i][j]);
                    if(LP_filter[i][j]) delete(LP_filter[i][j]);
                }

                if(HP_filter[i]) delete(HP_filter[i]);
                if(LP_filter[i]) delete(LP_filter[i]);
            }
        }
};

#endif // FREQ_RT_FILTERING_H
