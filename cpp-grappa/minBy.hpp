#include <Grappa.hpp>

using namespace Grappa;

template<typename T, typename P = std::pair<T,double>>
P collective_min_by_second(const P& p1, const P& p2) {
    return p1.second < p2.second ? p1 : p2;
}


template<typename T, typename F, typename GT=GlobalAddress<T>,  typename U = std::pair<GT,double> >
GT minBy( GT seq, size_t nelems,  F distance ) {
    int elems_per_core = nelems/Grappa::cores();
    GT min;
    on_all_cores([&] {

        int mycore = Grappa::mycore();
        int begin = mycore*elems_per_core;
        int end = begin+elems_per_core;

        // if last core, sum also the remainder
        if (mycore == Grappa::cores()-1) end+= nelems % Grappa::cores();

        GT min_val( seq+begin );
        double min_dist = delegate::call(min_val, [&](T& t){ return distance(t); });

        if (begin+1<=end) {
            for (int i = begin+1; i < end; i++) {
                GT v = seq+i;
            
                double d = delegate::call(v, [&](T& t){ return distance(t); });
                if (d < min_dist) {
                    min_val  = v;
                    min_dist = d;
                }
            }
        }


        U min_pair = std::make_pair(min_val, min_dist);

        // DVLOG(0) << "prepare to reduce";
        min = Grappa::allreduce<U,collective_min_by_second<T>>(min_pair).first;
    });
    // DVLOG(0) << "reduce = " << min;
    return min;

}
