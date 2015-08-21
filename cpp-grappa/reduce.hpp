#include <Grappa.hpp>

using namespace Grappa;

template<typename T,     GlobalAddress<T> (*collective_op)(const GlobalAddress<T>&, const GlobalAddress<T>&) >
T my_reduce(GlobalAddress<GlobalAddress<T>> seq, size_t nelems, T zero) {
    int elems_per_core = nelems/Grappa::cores();
    T total = zero;

    on_all_cores([&] {

        int mycore = Grappa::mycore();
        int begin = mycore*elems_per_core;
        int end = begin+elems_per_core;

        // if last core, sum also the remainder
        if (mycore == Grappa::cores()-1) end+= nelems % Grappa::cores();


        GlobalAddress<T> partial_total = Grappa::delegate::read(seq);
        
        
        if (begin+1 <= end) {
            for (int i = begin+1; i < end; i++) {
                partial_total = collective_op(partial_total, Grappa::delegate::read(seq+i));
            }
        }
    

         DVLOG(3) << "prepare to reduce w/ p total = " << partial_total;

        partial_total = Grappa::allreduce<GlobalAddress<T>,collective_op>(partial_total);

        // DVLOG(3) << "reduced to total = " << partial_total;

        if (mycore == 0) total = delegate::read(partial_total); 
    });
    return total;
}

