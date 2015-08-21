#include <Grappa.hpp>
#include <GlobalVector.hpp>
#include <GlobalHashMap.hpp>
#include <GlobalHashSet.hpp>
#include <limits>

#include "KMeans.hpp"


using namespace Grappa;
typedef GlobalAddress<GlobalHashMap<size_t, GPointVector>> PointMap;


const int nelems = 6;
const int n = 3 ;
const int iters = 15;
const int executions = 100; 
PointMap hm;
PointList xs;
PointList centroids;

GlobalCompletionEvent gce;      // tasks synchronization



GlobalAddress<Point> my_add(const GlobalAddress<Point>& p1, const  GlobalAddress<Point>& p2) {
    delegate::call(p1, [p2] (Point& _p1) {
        _p1 = _p1 + delegate::read(p2);
    });
    return p1;
}


Point average(GlobalAddress<GlobalAddress<Point>> ps, size_t npoints) {
    return my_reduce<Point, my_add>(ps, npoints, Point(0,0)) / (double) npoints;
}

// choices minBy { y => dist(x, y) }
GlobalAddress<Point> closest(Point x, PointList choices) {
    return minBy<Point>(choices.base, choices.size, [x] (Point y) { return dist(x,y); });
}


typedef GlobalAddress<Point> GPoint;

// template <typename F>
// //GlobalAddress<GlobalHashMap<Point,GPointVector>> 
// GlobalAddress<GlobalHashSet<Point>>
// groupBy(GlobalAddress<Point> seq, size_t nelems, F group) {
//     DVLOG(0) << "groupBy: N = "  << nelems << "\n";

//     GlobalAddress< GlobalVector<std::pair<Point,Point>> > gpv = GlobalVector<std::pair<Point,Point>>::create(nelems);

//     DVLOG(0) << "created gvector <Point,Point> "  << "\n";

//     forall(seq, nelems, [group, gpv](Point& p) {
        
//         GlobalAddress<Point> gk = group(p);
//         DVLOG(0) << "delegate call"  << "\n";

//         Point k = delegate::read(gk);

//         DVLOG(0) << "groupBy: "  << k << " -> " << p << "\n";

//         gpv->push(std::make_pair(k,p));
//     });

//     DVLOG(0) << "NOW:: insert all keys; size = " << gpv->size();



//     GlobalAddress<GlobalHashSet<Point>> gps = GlobalHashSet<Point>::create(gpv->size());


//     forall(gpv, [gps](std::pair<Point,Point>& p) {
//         gps->insert(p.first);
//     });

//     DVLOG(0) << "groupBy: END";

//     return gps;

// }
void groupBy() {
    forall(xs.base, xs.size, [](int64_t i, Point& p){
        GlobalAddress<Point> centroid_addr = closest(p, centroids);
        Point centroid = delegate::read(centroid_addr);
        size_t h = std::hash<Point>()(centroid);
        GPointVector gpv;
        hm->insert(h,&gpv);
        gpv->push(xs.base+i);
    });
}

void hm_clearAll() {
    hm->forall_entries([](size_t h, GPointVector gpv){
        gpv->clear();
    });
}

// void test_groupBy(PointList xs) {

//     PointList centroids(xs.base, 3);

//     DVLOG(0) << "test groupBy" << std::endl;



//     GlobalAddress<GlobalHashSet<Point>> set = 
//         groupBy(xs.base, xs.size, (Point p){ return closest(p, centroids); });




//     set->forall_keys([](Point& k){
//         std::cout << k << "\n";
//     });

//     std::cout << std::endl;
// }


void clusters() {
    DVLOG(0) << "clusters "  << "\n";


    groupBy();
    //centroids->clear();

    hm->forall_entries([](size_t h, GPointVector gpv){
        average(gpv->base, gpv->size());
    });
}




int main(int argc, char *argv[])
{





    init(&argc, &argv);
    run([=] {
        //PointList xs(from_json());

        int k = 0; 

        DVLOG(0) << "init";
        hm = GlobalHashMap<size_t, GPointVector>::create(n);
        xs = PointList(nelems);
        centroids = PointList(10);

        for (int i = 0; i < nelems; i++) {
            delegate::write(xs.base+k++, Point(i+1, i));
            delegate::write(xs.base+k++, Point(i, i+1));
        }


        PointList centroids(n);

        for (int i = 0; i < n; i++) {
            
            Point p = delegate::read(xs.base+i);
            delegate::write(centroids.base+i, p);
            size_t h =  std::hash<Point>()(p);

            GPointVector gpv = GlobalVector<GlobalAddress<Point>>::create(xs.size);
            gpv->clear();

            hm->insert(h, gpv);

            DVLOG(0) << p << " -> " << h;

        }  


        DVLOG(0) << "groupby";

        groupBy();

        hm->forall_entries([](size_t h, GPointVector gpv){
            forall(gpv, [h](GlobalAddress<Point>& p) {
                DVLOG(0) << h << " --> " << delegate::read(p);                 
            });

        });
        


        // auto xs = global_alloc<Point>(6);
        // for (int i = 0; i < 6; i++) {
        //      delegate::write(xs+i, Point(i,i));
        // }


        // test_groupBy(PointList(xs, 6));



            // for (size_t i = 0; i < iters; i++)  {
            //     DVLOG(0) << "iteration: " << i << "\n";
            //     auto cs = clusters(xs, centroids);
            //     PointList ps(cs.size);
            //     forall<unbound>(0, cs.size, [cs,ps](int64_t i){
            //         delegate::write(ps.base+i, average(cs.base+i, cs.size));
            //     });
            // }

            // clusters(xs, centroids);

    });
    finalize();




    return 0;
}
