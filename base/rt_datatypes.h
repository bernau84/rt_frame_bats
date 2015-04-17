#ifndef RT_DATATYPES_H
#define RT_DATATYPES_H

#include <vector>
using namespace std;

/*! \brief amplitude-frequency row/slice for rt buffer
 * 2D extension for simple rt_multibuffer
 */
template <class T> class t_rt_slice {

private:
    int irecent;  //last write index - -1 unitialized, 0 - first valid

public:

    T         t;  //time mark of this slice
    vector<T> A;  //amplitude
    vector<T> I;  //index / frequency

    //some useful operators on <A, I> pair
    typedef struct {

        T A;
        T I;
    } t_rt_ai;

    /*! \brief - test */
    bool isempty(){

       return (irecent < (signed)A.size()) ? true : false;
    }
    /*! \brief - test */
    bool isfull(){

       return (irecent == (signed)A.size()) ? true : false;
    }
    /*! \brief - reading from index */
    const t_rt_ai read(int i){

       i %= A.size(); //prevent overrange index
       t_rt_ai v = { A[i], I[i] };
       return v;
    }
    /*! \brief - read last written */
    const t_rt_ai last(){

       if(irecent < 0) irecent = 0;  //special case - not inited
       int i = irecent % A.size(); //prevent overrange index
       t_rt_ai v = { A[i], I[i] };
       return v;
    }
    /*! \brief - set last written */
    void set(const T v, const T f){

        if(irecent < 0) irecent = 0; //special case - not inited
        A[irecent] = v;
        I[irecent] = f;
    }
    /*! \brief - writing, returns number of remaining positions */
    int append(const T v, const T f){

       if(++irecent < (signed)A.size()){

           A[irecent] = v;
           I[irecent] = f;
       }

       return (A.size() - irecent);
    }

    t_rt_slice &operator= (const t_rt_slice &d){

        A = vector<T>(d.A);
        I = vector<T>(d.I);
        t = d.t;
        irecent = -1;
        return *this;
    }

    t_rt_slice(const t_rt_slice &d):
        irecent(-1), t(d.t), A(d.A), I(d.I){;}

    t_rt_slice(T time = T(), int N = 0, T def = T()):
        irecent(-1), t(time), A(N, def), I(N){;}
};

#endif // RT_DATATYPES_H
