#include <Grappa.hpp>
#include <GlobalVector.hpp>
#include <GlobalHashMap.hpp>
#include <GlobalHashSet.hpp>
#include <limits>
#include <cassert>

#include "KMeans.hpp"


const int N = 3;
const int ITERS = 15;

const long NPOINTS = 100;

typedef GlobalAddress<GlobalVector<Point>> GPointVector;
GlobalAddress<GlobalHashMap<size_t, int>> gmap;
GPointVector gvectors[N];

GlobalAddress<Point> gcentroids;


using namespace Grappa;

GlobalCompletionEvent avg_gce;

void average(PointList xs, GlobalAddress<Point> dest) {
    forall<async>(xs.base, xs.size, [dest, sz = xs.size](Point& p){
        delegate::increment<async>(dest, p/(double) sz);
    });
}

// Point& average(PointList xs) {
//
//     Point avg(0,0);
//     for (int i = 0; i < xs.size; i++) {
//         Point p = delegate::read(xs.base+i);
//         avg = avg + p;
//
//     }
//     Point result = avg / (double)xs.size;
//
//     DVLOG(0)<<avg << "/" << xs.size << " = " << result;
//     return result;
// }

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


// Point _closest(Point p, PointList xs) {

//     GlobalAddress<MinPoint> gmin = global_alloc<MinPoint>(1);
//     Point firstp = delegate::read(xs.base);
//     MinPoint min(p,firstp);

//     delegate::write(gmin, min);

//     forall<&closest_gce>(xs.base, xs.size, [gmin, min](Point& p){
//         MinPoint new_min(min.ref_point,p);
//         delegate::compare_and_swap<async>(gmin, new_min, new_min);
//     });

//     return delegate::read(gmin).point;
// }


Point closest(Point p, PointList xs) {
    Point min = delegate::read(xs.base);
    double min_dist = dist(p, min);
    for (size_t i=1;i<xs.size;i++) {
        Point candidate = delegate::read(xs.base+i);
        double d = dist(p, candidate);
        if (d < min_dist) {
            min = candidate;
            min_dist = d;
        }
    }
    return min;
}

void dump_array(GlobalAddress<Point> ps, size_t sz) {
    for(int j = 0; j < sz; j++) {
        Point p = delegate::read(ps+j);
        std::cout << p << ", ";
    };
    std::cout << std::endl;
}

void dump_vector(GPointVector gpv) {
  dump_array(gpv->base, gpv->size());
}

GlobalCompletionEvent closest_gce;


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
        delegate::write(gcentroids+i, p);

        DVLOG(0) << p.hash()<<", " << i;
        gmap->insert(p.hash(), i);

    }

    vectors_init();

}

void gcentroids_init(PointList xs) {
    GlobalAddress<Point> gpv = global_alloc<Point>(N);
    on_all_cores([gpv]{
        gcentroids = gpv;
    });
    for (int i = 0; i < N; i++) {
        Point p = delegate::read(xs.base+i);
        DVLOG(0) << "centroid " << p;
        delegate::write(gcentroids+i, p);
    };
}

void map_insert(Point& key, Point& value) {
    int vect_idx;

    gmap->lookup(key.hash(), &vect_idx);

    DVLOG(3) << "vect_idx " << vect_idx << std::endl;
    assert(vect_idx < N);

    gvectors[vect_idx]->push(value);
}

void map_reset() {
    gmap->clear();

    for(int64_t i = 0; i < N; i++){
        Point c = delegate::read(gcentroids+i);
        int j = i;
        DVLOG(0) << i << "::" << c.hash();
        gmap->insert(c.hash(), j);
        DVLOG(0) << i << "::" << c;

        gvectors[i]->clear();
    };
}

void clear_centroids() {
  for (int i = 0; i < N; i++) delegate::write(gcentroids+i, Point(0,0));
}

void calc_centroids(PointList xs) {

    map_reset();

    DVLOG(0) << "map was reset";

    PointList centroids(gcentroids, N);

    for(int i = 0; i < xs.size; i++) {
        Point x = delegate::read(xs.base+i);
        Point closest_p = closest(x, centroids);

        DVLOG(0) << x << " is closest to " << closest_p ;

        map_insert(closest_p, x);
    };

    DVLOG(4) << "reset centroids";

    clear_centroids();

    gmap->forall_entries([](size_t hash, int vect_idx){
        PointList ps(gvectors[vect_idx]->base, gvectors[vect_idx]->size());
        DVLOG(0) << "call average with a size of " << gvectors[vect_idx]->size();
        average(ps, gcentroids+vect_idx);
        DVLOG(0) << "avg is " << delegate::read(gcentroids+vect_idx);
    });

    std::cout<<"CENTROIDS ARE NOW:: ";
    dump_array(gcentroids, N);


}


void dump_clusters() {

    for(int i = 0; i < N; i++) {
        Point c = delegate::read(gcentroids+i);
        std::cout << "centroid " << c << "\n    ";
        int idx;
        gmap->lookup(c.hash(), &idx);
        dump_vector(gvectors[idx]);
        std::cout << std::endl;
    };
}




namespace test {
    void average() {
        PointList xs(10);
        for (int i = 0; i < 10; i++) {
            delegate::write(xs.base+i, Point(i,i));
        }
        Point* total = new Point(0,0);
        GlobalAddress<Point> avg = make_global(total);
        finish([xs,avg]{
          average(xs, avg);
        });
        Point p = *total;
        std::cout << p;
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
        test::centroids();
        dump_clusters();
    });
    Grappa::finalize();
    return 0;
}
