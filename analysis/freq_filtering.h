#ifndef FREQ_RT_FILTERING_H
#define FREQ_RT_FILTERING_H

#include <stdint.h>
#include <stdarg.h>
#include <vector>

#define FREQ_RT_FILT_TRACE(NO, FORMAT, ...)\
    fprintf (stderr, "%d>  "FORMAT"\n", NO, __VA_ARGS__);


static int32_t _n_to_proc = 0;   //null dev in app doesn't need this value

/*! \class template of t_pFilter
 * \brief - pure virtual ancestor of SISO digital filter
 * deep copy of filter coeficient is made
 * filter impementation depends on successor as well as order of coeficinets
 */
template <class T> class t_pFilter {

    public:

        enum e_filter_struct {

            IIR_DIRECT1 = 1,
            IIR_DIRECT2 = 2,
            IIR_LATTICE = 3,
            IIR_BIQUADR = 4,

            FIR_DIRECT1 = 5,
            FIR_DELAYLN = 6,
            FIR_LATTICE = 7,

            FFT_FILTER = 8,
            AVER_RECURSIVE = 9
        };

    protected:
        std::vector<T> shift_dat;
        std::vector<T> coeff_num;
        std::vector<T> coeff_den;

        int32_t N;            //pocet koeficientu

        unsigned decimationf;     //mozny je jen faktor > 0, 0 funguje jako pauza (vystup se neupdatuje)
        e_filter_struct struction;  //typ pro zpetnou identifikaci

        uint64_t proc;     //index zracovaneho vzorku (inkrement)
        T prev;    //zaloha posledniho vysledeku

    public:

        /*! \brief - defines behaviour of filter,
            n_2_proc == 0 defines valid output in decimation mode
            \todo - may be used to signal valid output after group delay, after filter run-up time/sample
        */
        virtual T process(const T new_smpl, int32_t *n_to_proc = &_n_to_proc) = 0;

        /*! \brief - backward identification */
        e_filter_struct getTyp(){

            return struction;
        }

        /*! \brief - return last filtering result (cache for deciamtion purpose) */
        T getLast(){

            return prev;
        }

        /*! \brief - inicialize shift register
         */
        void reset(const T def = T(0)){

            shift_dat.assign(shift_dat.size(), def);
            prev = T(0);
            proc = 0;
        }

        /*! \brief - set initial value (to minimize gd for example)
         * preload sample inxed as well (to realize harmonic bank filter)
         */
        void reset(std::vector<T> &def, T iniv = T(0)){

            int i=0;
            for(; i<shift_dat.size(); i++)
                if(i<def.size()) shift_dat[i] = def[i];
                    else shift_dat[i] = T(0.0);

            prev = iniv;
            proc = i;
        }        

        /*! \brief - copy of existing, except delay line */
        t_pFilter(const t_pFilter<T> &src_coeff_cpy):
            shift_dat(src_coeff_cpy.shift_dat),
            coeff_num(src_coeff_cpy.coeff_num),
            coeff_den(src_coeff_cpy.coeff_den),
            decimationf(src_coeff_cpy.decimationf),
            struction(src_coeff_cpy.struction)
        {
            proc  = 0;
        }

        /*! \brief - new flter definition (including decimation factor) */
        t_pFilter(const T *_coeff_num, const T *_coeff_den, int32_t _N, int32_t _decimationf = 1):
            shift_dat(0),
            coeff_num(0),
            coeff_den(0),
            decimationf(_decimationf)
        {
            if((N = _N) > 1) shift_dat = std::vector<T>(N - 1);            
            if(_coeff_num) coeff_num = std::vector<T>(_coeff_num, &_coeff_num[N]);
            if(_coeff_den) coeff_den = std::vector<T>(_coeff_den, &_coeff_den[N]);
           
            prev = T(0);
            proc  = 0;
        }

        /*! \brief - free previsous allocation */
        virtual ~t_pFilter(){

        }
};

/*! \class template of t_AveragingFilter
 * \brief - recursive moving averaging filter starting as liner an after aquire RC x samples
 * switch to IIR 1-pole filter with respoect to T_A ~ 2*T_RC
 */
template <class T> class t_AveragingFilter : public t_pFilter<T> {

    public:
        /*! \brief - simple delay buffer, ceficients doesn't play any role */
        T process(const T new_smpl, int32_t *n_to_proc = &_n_to_proc){

            T y = this->prev;
            this->proc += 1;

            if(this->proc < this->coeff_num[0])
                y = y * (this->proc - 1)/this->proc + new_smpl / this->proc; //linear
            else
                y += (new_smpl - y) / (this->coeff_num[0] / 2); //exponential moving

            //decimation - on every dec.period refresch prev cache value
            if(0 == (*n_to_proc = (this->proc % this->decimationf)))
                this->prev = y;

            return this->prev;
        }

        /*! \brief - runtime modification of averating time constant */
        void modify(T TA){

           this->coeff_num[0] = TA;
        }

        /*! \brief - copy constructor */
        t_AveragingFilter(const t_pFilter<T> &src_coeff_cpy)
                      :t_pFilter<T>(src_coeff_cpy)
        {
            this->struction = t_pFilter<T>::AVER_RECURSIVE;
        }

        /*! \brief - new delay line; rc konstant as parameter - in number of samples
         * averaging time TA ~ 2*RC
        */
        t_AveragingFilter(T TA, int32_t _decimationf = 1)
                      :t_pFilter<T>(NULL, NULL, 1, _decimationf)
        {
            this->shift_dat[0] = (T)0;
            this->coeff_num[0] = TA;
            this->N = 1;

            this->struction = t_pFilter<T>::AVER_RECURSIVE;
        }

        ~t_t_AveragingFilter(){;}
};


/*! \class template of t_DelayLine
 * \brief - simple delay buffer of length N, ceficients doesn't play any role
 * supports decimation
 */
template <class T> class t_DelayLine : public t_pFilter<T> {

    public:
        /*! \brief - simple delay buffer, ceficients doesn't play any role */
        T process(const T new_smpl, int32_t *n_to_proc = &_n_to_proc){

            this->shift_dat[this->proc++ % (this->N - 1)] = new_smpl;

            //decimation - on every dec.period refresh prev cache value
            if(0 == (*n_to_proc = (this->proc % this->decimationf)))
                this->prev = this->shift_dat[this->proc % (this->N - 1)];

            return this->prev;
        }

        /*! \brief - copy constructor */
        t_DelayLine(const t_pFilter<T> &src_coeff_cpy)
                      :t_pFilter<T>(src_coeff_cpy)
        {
            this->struction = t_pFilter<T>::FIR_DELAYLN;
        }

        /*! \brief - new delay line */
        t_DelayLine(int32_t _N, int32_t _decimationf = 1)
                      :t_pFilter<T>(NULL, NULL, _N+1, _decimationf)
        {
            this->struction = t_pFilter<T>::FIR_DELAYLN;
        }

        ~t_DelayLine(){;}
};

/*! \class template of t_CanonFilter IIR form
 * \brief - canonnical filter structure - direct form of 2nd class, N-1 is order of filter => N coeff is needed
 * \warning - den_coeff[0] must be 1.0 (!)
 */
template <class T> class t_CanonFilter : public t_pFilter<T> {

    public:
        /*! \brief - not for efective decimation */
        T process(const T new_smpl, int32_t *n_to_proc = &_n_to_proc){

            T v_i, v_n = new_smpl, y = T(0);

            for(int32_t i=1; i<this->N; i++){

                v_i = this->shift_dat[(this->proc + i - 1) % (this->N - 1)];
                v_n -= this->coeff_den[this->N-i] * v_i;
                y   += this->coeff_num[this->N-i] * v_i;
            }

            y += this->coeff_num[0] * v_n;
            this->shift_dat[this->proc % (this->N - 1)] = v_n;

            //decimation - on every dec.period refresch prev cache value
            if(0 == (*n_to_proc = (this->proc++ % this->decimationf)))
                this->prev = y;

            return this->prev;
        }

        t_CanonFilter(const t_pFilter<T> &src_coeff_cpy)
                      :t_pFilter<T>(src_coeff_cpy)
        {
            this->struction = t_pFilter<T>::IIR_DIRECT1;
        }

        t_CanonFilter(const T *_coeff_num, const T *_coeff_den, int32_t _N, int32_t _decimationf = 1)
                      :t_pFilter<T>(_coeff_num, _coeff_den, _N, _decimationf)
        {
            this->struction = t_pFilter<T>::IIR_DIRECT1;
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

        T process(const T new_smpl, int32_t *n_to_proc = &_n_to_proc){

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

            //decimation - on every dec.period refresch prev cache value
            if(0 == (*n_to_proc = (this->proc++ % this->decimationf)))
                this->prev = y;

            return this->prev;
        }

        t_BiQuadFilter(const t_pFilter<T> &src_coeff_cpy)
                       :t_pFilter<T>(src_coeff_cpy){

            this->struction = t_pFilter<T>::IIR_BIQUADR;
        }

        t_BiQuadFilter(const T *_coeff_num, const T *_coeff_den, int32_t _N, int32_t _decimationf = 1)
                       :t_pFilter<T>(_coeff_num, _coeff_den, 3*_N, _decimationf){

            this->struction = t_pFilter<T>::IIR_BIQUADR;
        }

        ~t_BiQuadFilter(){;}
};

/*! \class - template of t_DirectFilter FIR
 * \brief - direct FIR form with efficient decimator implementation
 * N is number of taps
 */
template <class T> class t_DirectFilter : public t_pFilter<T>{

    public:
        T process(const T new_smpl, int32_t *n_to_proc = &_n_to_proc){

            if(0 == (*n_to_proc = (this->proc++ % this->decimationf))){

                this->prev = new_smpl * this->coeff_num[0];
                for(int32_t i=1; i<this->N; i++ ){

                    T v_i = this->shift_dat[(this->proc + i - 1) % (this->N - 1)];
                    this->prev += this->coeff_num[this->N-i]*v_i;  //reverse order of coefs assumed or symetry...hm
                }

                FREQ_RT_FILT_TRACE(1, "fir-proc-tick on %llu, res %f", this->proc, this->prev);
            }

            this->shift_dat[this->proc % (this->N - 1)] = new_smpl;
            return this->prev;  //pokud vypocet neprobihal (kvuli decimaci, vracime posledni vysledek)
        }

        t_DirectFilter(const t_pFilter<T> &src_coeff_cpy)
                       :t_pFilter<T>(src_coeff_cpy){

            this->struction = t_pFilter<T>::FIR_DIRECT1;
        }

        t_DirectFilter(const T *_coeff_num, int32_t _N, int32_t _decimationf = 1)
                       :t_pFilter<T>(_coeff_num, NULL, _N, _decimationf){

            this->struction = t_pFilter<T>::FIR_DIRECT1;
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
        T process(const T new_smpl, int32_t *n_to_proc = &_n_to_proc){

            T f, f_i, g, g_i;

            g = f = new_smpl;
            for(int32_t i=1; i<this->N; i++ ){

                f_i = f + this->coeff_num[i]*this->shift_dat[i-1];
                g_i = f*this->coeff_num[i] + this->shift_dat[i-1];

                this->shift_dat[i-1] = g;
                g = g_i; f = f_i;
            }

            //decimation - on every dec.period refresch prev cache value
            if(0 == (*n_to_proc = (this->proc++ % this->decimationf)))
                this->prev = g; //or f (?!)

           return this->prev;
        }


        t_LatticeFilter(t_pFilter<T> &src_coeff_cpy)
                       :t_pFilter<T>(src_coeff_cpy){

            this->struction = t_pFilter<T>::FIR_LATTICE;
        }

        t_LatticeFilter(const T *_coeff_num, int32_t _N, int32_t _decimationf = 1)
                       :t_pFilter<T>(_coeff_num, NULL, _N, _decimationf){

            this->struction = t_pFilter<T>::FIR_LATTICE;
        }

        ~t_LatticeFilter(){;}
};

/*! \class - template of t_DiadicFilterBank SIMO system
 * \brief - realize decimation tree with diadic step while LP_filter decimate with factor 2 (and it should)
 * impemented algorithm is produce one 2 samples per sample - duration of every process is constant and limited.
 * for resamplink line only the HP filter should be identity function
 * M - number of stages
 */
template <class T, int32_t DIADTREE_MAX_STAGES> class t_DiadicFilterBank {

    private:
        t_pFilter<T> *HP_filter[DIADTREE_MAX_STAGES];
        t_pFilter<T> *LP_filter[DIADTREE_MAX_STAGES];

        uint64_t    in;  //sample index
    public:
        /*! \brief - computes highest freq and one of lower stage
         * \param - sum_rslr is external buffer where output of all stages can be stored (M size is assumed)
         * \returns - number of additional stage; fist call reurns 0, because only highest band is updated (stage0!!!)
         */
        int32_t process(const T new_smpl, T *sum_rslt){

            //nejvyssi oktava
            sum_rslt[0] = HP_filter[0]->process(new_smpl);
            LP_filter[0]->process(new_smpl);

            //count next stage index 
            //every next stage runs at half speed - that ensures modulo 
            //plus we need a shift each stage process to hamrmonize analysis speed
            int32_t m = 1;
            for(; m<DIADTREE_MAX_STAGES; m++)
                if(0 == (((2^(m-1))+1 + in) % (2^m))){  //offset + step % period

                    //pocitame dalsi z nizsich stupnu
                    sum_rslt[m] = HP_filter[m]->process(LP_filter[m-1]->getLast());
                    LP_filter[m]->process(LP_filter[m-1]->getLast());

                    FREQ_RT_FILT_TRACE(2, "%d-bank-proc on %llu", m, in);
                    break;              
                }

            in++;
            return m;
        }

        /*! \brief - Reset history (internal and filters)
         */
        void reset(){

            in = 0;
            for(int i=0; i<DIADTREE_MAX_STAGES; i++){

                HP_filter[i]->reset();
                LP_filter[i]->reset();
            }
        }

        /*! \brief - allocate resampling LP line and analytic HP filter according to given teplates
         */
        template<template<class> class LP, template<class> class HP>
            t_DiadicFilterBank(const LP<T> &_HP_filter, const HP<T> &_LP_filter){

            for(int i=0; i<DIADTREE_MAX_STAGES; i++){

                HP_filter[i] = new HP<T>(_HP_filter);
                LP_filter[i] = new LP<T>(_LP_filter);
            }

            reset();
        }

        /*! \brief - destroy filters and imed buffers
         */
        ~t_DiadicFilterBank(){

            for(int i=0; i<DIADTREE_MAX_STAGES; i++){

                delete(HP_filter[i]);
                delete(LP_filter[i]);
            }
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

        /*! \brief - process one sample
         *  \return - 1 if we reach the last stage, results in sum_rslt are valid now
         *  size of output buffer rum_rslt must be at least 2^M (!)
         */
        int32_t process(const T new_smpl, T *sum_rslt){

            int32_t i, j, to_process = 1;
            int32_t rev_lo, rev_hi;
            T part_rslt[1<<PACKETTREE_MAX_STAGES];
            T back_rslt[1<<PACKETTREE_MAX_STAGES];  //zde budem drzet mezivysledky, nastaveno na max pocet koef.

            back_rslt[0] = new_smpl;
            for(i=0; (i<M) && (to_process == 1); i++){  //dokud pocitame jen liche tak postupujem po stages, maximalne do M

                rev_hi = 1; rev_lo = 0;
                for(j=0; j<(1<<i); j++){

                    part_rslt[2*j+rev_lo] = LP_filter[i][j]->process(back_rslt[j], &to_process);
                    part_rslt[2*j+rev_hi] = HP_filter[i][j]->process(back_rslt[j], &to_process);  //&to_process by melo byt u obou stejne

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
        void reset(){

            for(int32_t i=0; i<M; i++ )
                for(int32_t j=0; j<(1<<i); j++){

                    HP_filter[i][j]->reset();
                    LP_filter[i][j]->reset();
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


/*! \class - template of t_DiadicFilterBank SIMO system
 * \brief - realize decimation tree with diadic step while LP_filter decimate with factor 2
 * impemented algorithm is produce one 2 samples per sample - duration of every process is constant and limited.
 * M - number of stages
 */
template <class T, int32_t DIADTREE_MAX_STAGES> class t_DiadicFilterBankH {

    private:
        t_DirectFilter<T> LP_filter[DIADTREE_MAX_STAGES];
        uint64_t    in;  //sample index

        unsigned M;
        unsigned N;

    public:
        /*! \brief - computes highest freq and one of lower stage
         * \param - sum_rslr is external buffer where output of all stages can be stored
         */
        int32_t process(const T new_smpl, T (*sum_rslt)[DIADTREE_MAX_STAGES]){

            //highest octave
            sum_rslt[0] = LP_filter[0].process(new_smpl);

            //count next stage index 
            //every next stage runs at half speed - that ensures modulo 
            //plus we need a shift each stage process to hamrmonize analysis speed
            int32_t m = 1;
            for(; m<M; m++)
                if(0 == ((2^(m-1)+1 + in) % (2^m))){  //offset + step % period

                    //another lower stage
                    sum_rslt[m] = LP_filter[m].process(LP_filter[m-1]->GetLast());
                    FREQ_RT_FILT_TRACE(2, "%d-bank-proc on %llu", m, in);
                    break;              
                }

            in++;
            return m;
        }

        /*! \brief - Reset history (internal and filters)
         */
        void reset(){

            in = 0;
            for(int i=0; i<M; i++)
                LP_filter[i].reset();
        }

        /*! \brief - allocate resampling LP line and analytic HP filter according to given teplates
         */
        t_DiadicFilterBankH(const T halfband_coef[], unsigned  _N, unsigned _M):
            M(_M),
            N(_N)
        {

            if(!M*N)
                return;

            for(int i=0; i<M; i++)
                LP_filter[i] = t_DirectFilter<T>(halfband_coef, 0, 2);

            reset();
        }

        /*! \brief - destroy filters and imed buffers
         */
        ~t_DiadicFilterBankH(){;}

};

/*! \class Packet (Wavelet) Tree Harmonized
 * heavy use of recursion (in runtime & constructor)
 * procceed exactlu M stages every sample
 * each band has slightly different group delay - check the order of update on return from proccess
 */
template <class T, int32_t PACKETTREE_MAX_STAGES> class t_PacketTreeH{

    private:

        struct t_packet_node {

            t_DirectFilter<T> HP_filter;    //working hp 2x decimation fir filter - ticks on odd samples - see reset method()
            t_DirectFilter<T> LP_filter;    //working hp 2x decimation fir filter - ticks on event samples (unchanged)

            int HP_succ_inx;    //follow node index
            int LP_succ_inx;

            unsigned code;          //binary code of node == frequency band (after decimation-aliasing compesation using mirroring in HP)
            unsigned stage;         //node stage  
        } node[(0x1 << PACKETTREE_MAX_STAGES) - 1]; //2^M - 1; pro upresneni M=3 -> 4 nody na vystupu (8 pasem) 


        unsigned M; //actual number of stages
        unsigned N; //filter length

        uint64_t in;    //sample index

        /*! recursively generate packet tree network
        */
        int __interconnect(unsigned m = 0, unsigned c = 0){

            static unsigned n = 0;
            unsigned i = n;     //index of this node

            node[i].stage = m;
            node[i].code = c;
            
            if(++n >= sizeof(node)){

                FREQ_RT_FILT_TRACE(1, "!!node index %d overflow!!", n); 
                return -1;
            }  

            if(m < M){

                node[i].LP_succ = __interconnect(m+1, (c << 1) + 1);
                node[i].HP_succ = __interconnect(m+1, (c << 1) + 0);
            }

            return i;
        }


        /*! recursion core half-band complementary filters
         */
        unsigned run(const T inp, unsigned n = 0){

            FREQ_RT_FILT_TRACE(2, "node %d on sample %llu", n, in);

            unsigned l_tick, h_tick;
            node[n].LP_filter.process(inp, &l_tick);
            node[n].HP_filter.process(inp, &h_tick);

            if(l_tick == 0 && h_tick == 0)
                FREQ_RT_FILT_TRACE(3, "!!concurency on node %d!!", n);              

            if((n >= 0) && (node[n].stage < M)){

                if(l_tick == 0) {

                    inp = node[n].LP_filter.getLast();
                    return run(inp, node[n].LP_succ_inx);
                } 

                if(h_tick == 0) {

                    inp = node[n].HP_filter.getLast();
                    inp *= (0x1 & (in << node[n].stage)) ? T(-1.0) : T(1.0);  //fs/2 shift (act as mirroring here) to prevent reorder freq after decimation
                    return run(inp, node[n].HP_succ_inx);
                }

                FREQ_RT_FILT_TRACE(4, "!!no tick on node %d!!", n)
                return -1;
            } 

            return n;
        }        

    public:

        /*! \brief - recursively process one sample
         *  size of output buffer rum_rslt must be at least 2^M (!)
         */
        int32_t process(const T new_smpl, T (*sum_rslt)[0x1 << PACKETTREE_MAX_STAGES]){

            unsigned i = run(new_smpl);

            sum_rslt[node[i].code + 0] = node[i].LP_filter.getLast(); //not sure which - update both
            sum_rslt[node[i].code + 1] = node[i].HP_filter.getLast();

            FREQ_RT_FILT_TRACE(5, "band %d(+1) updated on sample %llu", node[i].code, in);
            in += 1;

            return in;
        }

        /*! \brief - resets all filters
         * hp is initiated to decimate avery odd sample, lp will decimate to even
         */
        void reset(){

            std::vector<T> pre(1);  //preloads HP to ensure shift when it tick
            pre[0] = T(0.0);

            for(int32_t n = 0; n < sizoef(node); n++){

                node[n].HP_filter.reset(pre);
                node[n].LP_filter.reset();
            }
        }

        /*! \brief - creates all M^2 packet tree filters
         */
        t_PacketTreeH(const T halband_coef[], unsigned  _N, unsigned _M):
            M(_M),
            N(_N)
        {

            if(!M*N)
                return;

            const T lp_coeff[N];  //temp
            const T hp_coeff[N];

            for(int32_t i = 0; i< N ; i++){

               hp_coeff[i] = lp_coeff[i] = halband_coef[i];
               hp_coeff[i] *= (i & 1) ? T(-1.0) : T(1.0);  //shift fs/2 to create high pass
            }


           for(int32_t n = 0; n < sizoef(node); n++){ //iterrate trought all filters

                node[n].HP_filter = t_DirectFilter<T>(hp_coeff, 0, 2);
                node[n].LP_filter = t_DirectFilter<T>(lp_coeff, 0, 2);

                node[n].HP_succ_inx = -1;
                node[n].LP_succ_inx = -1;
            }

            reset(); //reset + delay output HP filters for 1 sample 

            __interconnect(); //connection between nodes & nodes role assesment
        }

        /*! \brief - destroy all filters
         */
        ~t_PacketTreeH(){;}

};

#endif // FREQ_RT_FILTERING_H
