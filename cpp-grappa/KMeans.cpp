#include <Grappa.hpp>
#include <GlobalVector.hpp>
#include <GlobalHashMap.hpp>
#include <GlobalHashSet.hpp>
#include <limits>
#include <cassert>

#include "KMeans.hpp"


const int N = 3;
const int ITERS = 15;

const long NPOINTS = 10;

typedef GlobalAddress<GlobalVector<Point>> GPointVector;
GlobalAddress<GlobalHashMap<size_t, int>> gmap;
GPointVector gvectors[N];

GPointVector gcentroids; 


using namespace Grappa;

GlobalCompletionEvent avg_gce;

Point& average(PointList xs)
{
    GPoint total = global_alloc<Point>(1);
 
    forall<&avg_gce>(xs.base, xs.size, [total](Point& p){
        delegate::increment<async,&avg_gce>(total, p);
    }); 

    std::cout<<"avg result";

    Point result( delegate::read(total) / xs.size );

    global_free(total);

    return result;
}

struct MinPoint {
    double d = 0;
    Point ref_point;
    Point point;
    MinPoint(){}
    MinPoint(const Point& ref_point_, const Point& p_) : point(p_), ref_point(ref_point_) {
        d = dist(point, ref_point);
        // std::cout << point << ref_point << d << "\n";

    }  
};

bool operator==(const MinPoint& a, const MinPoint& b) {
    // std::cout << (a.d<b.d) << std::endl;
    return a.d < b.d;
}

GlobalCompletionEvent closest_gce;

Point closest(Point p, PointList xs) {

    GlobalAddress<MinPoint> gmin = global_alloc<MinPoint>(1);
    Point firstp = delegate::read(xs.base);
    MinPoint min(p,firstp); 

    delegate::write(gmin, min);

    forall<&closest_gce>(xs.base, xs.size, [gmin, min](Point& p){
        MinPoint new_min(min.ref_point,p);
        delegate::compare_and_swap<async>(gmin, new_min, new_min);
    });

    return delegate::read(gmin).point;
}

void vectors_init() {
    for (int i = 0; i < N; i++) {
        auto gv = GlobalVector<Point>::create(NPOINTS);
        on_all_cores([i,gv] {
            gvectors[i] = gv;
        });
        DVLOG(0) << "gvectors[" << i << "] inited";  
        gvectors[i]->clear();
    }
}
void map_init(PointList xs) {
    DVLOG(0) << "map init";
    gmap = GlobalHashMap<size_t, int>::create(N);

    for (int i = 0; i < N; i++) {
        
        // init first N centroids
        Point p = delegate::read(xs.base+i);
        delegate::write(gcentroids->base+i, p);

        // DVLOG(0) << p.hash()<<", " << i;
        gmap->insert(p.hash(), i);

    }  

    vectors_init();

}

void gcentroids_init(PointList xs) {
    gcentroids = GlobalVector<Point>::create(N);
    for (int i = 0; i < N; i++) {
        Point p = delegate::read(xs.base+i);
        DVLOG(0) << p;
        gcentroids->push(p);
    };
}

void map_insert(Point& key, Point& value) {
    int vect_idx;

    if (gmap->lookup(key.hash(),&vect_idx)) {
       DVLOG(0) << "vect_idx " << vect_idx << std::endl;
       DVLOG(0) << gvectors[vect_idx];
    } else {
        DVLOG(0) << "DIE PLEASE\n";
    }

    gvectors[vect_idx]->push(value);

    DVLOG(0) << "PUSHED";
}

void map_reset() {
    gmap->clear();
    // forall(gcentroids, [](int64_t i, Point& c){
    //     int j = i;
    //     DVLOG(0) << i << "::" << c.hash();
    //     gmap->insert(c.hash(), j);
    //     DVLOG(0) << i << "::" << c;
        
    //     gvectors[i]->clear();
    // });

    for(int64_t i; i < gcentroids->size(); i++){
        Point c = delegate::read(gcentroids->base+i);
        int j = i;
        DVLOG(0) << i << "::" << c.hash();
        gmap->insert(c.hash(), j);
        DVLOG(0) << i << "::" << c;
        
        gvectors[i]->clear();
    };
}

void calc_centroids(PointList xs) {

    map_reset();

    DVLOG(0) << "map was reset";

    PointList centroids(gcentroids->base, gcentroids->size());

    for(int i = 0; i < xs.size; i++) {
        Point x = delegate::read(xs.base+i);   

        DVLOG(0) << x;

        Point closest_p = closest(x, centroids);

        DVLOG(0) << closest_p;

        map_insert(closest_p, x);
    };


    gmap->forall_entries([](size_t hash, int vect_idx){
        PointList ps(gvectors[vect_idx]->base, gvectors[vect_idx]->size());
        Point avg = average(ps);
        std::cout << avg << std::endl;
        gcentroids->push(avg);
    });


}

void dump_clusters() {
        std::cout << "dumping..."<<  gcentroids->size() <<"\n";

    forall(gcentroids, [] (Point& c) {
        std::cout << "centroid " << c;
        int idx;
        gmap->lookup(c.hash(), &idx);
        forall(gvectors[idx], [] (Point& p){
            std::cout << p << ", ";
        });
        std::cout << std::endl;
    });
}




namespace test {
    void average() {
        PointList xs(NPOINTS);
        for (int i = 0; i < NPOINTS; i++) {
            delegate::write(xs.base+i, Point(i,i));
        }
        Point p = average(xs);
        assert (p.x == 4.5 && p.y == 4.5);
    }
    void closest() {
        PointList xs(NPOINTS);
        for (int i = 0; i < NPOINTS; i++) {
            delegate::write(xs.base+i, Point(i,i));
        }
        Point p = closest(Point(4,5), xs);
        // std::cout << p <<"\n";
        assert (p.x == 4 && p.y == 4);
    }
    void centroids() {
        PointList xs(NPOINTS);
        for (int i = 0; i < NPOINTS; i++) {
            delegate::write(xs.base+i, Point(i,i));
        }

        gcentroids_init(xs);
        map_init(xs);

        for (int k=0;k<ITERS;k++) {
            calc_centroids(xs);
        }
        calc_centroids(xs);
    }

}



int main(int argc, char const *argv[])
{
    Grappa::init(&argc, &argv);
    Grappa::run([]{

        // test::closest();

        test::centroids();
        // std::cout<"::::::::::::";
        // dump_clusters();
    });
    Grappa::finalize();
    return 0;
}