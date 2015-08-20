#include <Grappa.hpp>
#include <GlobalVector.hpp>
#include <GlobalHashMap.hpp>
#include <limits>


using namespace Grappa;



const int n = 10 ;
const int iters = 15;
const int executions = 100;


GlobalCompletionEvent gce;      // tasks synchronization


double sq(double x) { return x*x; }

template<typename T, T (*collective)(const T&, const T&) >
T reduce(GlobalAddress<T> seq, size_t nelems, T zero) {
    int elems_per_core = nelems/Grappa::cores();
    T total = zero;
    on_all_cores([&] {

        int mycore = Grappa::mycore();
        int begin = mycore*elems_per_core;
        int end = begin+elems_per_core;

        // if last core, sum also the remainder
        if (mycore == Grappa::cores()-1) end+= nelems % Grappa::cores();

        T partial_total = zero;
        for (int i = begin; i < end; i++) {
            partial_total = collective(partial_total, Grappa::delegate::read(seq+i));
        }

        if (mycore == 0) total = Grappa::allreduce<T,collective>(partial_total);
    });
    return total;
}


//template<typename T, typename U, typename P = std::pair<T,U>, P (*LIFTED)(const P&,const P&)>
//LIFTED lift( U (*f)(const T&, const T&) ) {

//}

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

        double min_dist = std::numeric_limits<double>::max();
             GT min_val( seq+begin );

        if (begin+1<=end) {
            for (int i = begin+1; i < end; i++) {
                GT v = seq+i;
                double d = delegate::call(v, [&](T& t){ return distance(t); });
                if (d < min_dist) {
                    min_val = v;
                }
            }
        }

        U min_pair = std::make_pair(min_val, min_dist);
        if (mycore == 0) min = Grappa::allreduce<U,collective_min_by_second<T>>(min_pair).first;
    });
    return min;

}


//template<typename T, typename F>
//T reduce(GlobalAddress<T> seq, size_t nelems, T zero, F aggr) {
//    int elems_per_core = nelems/Grappa::cores();
//    T total = zero;
//    on_all_cores([&] {

//        int mycore = Grappa::mycore();
//        int begin = mycore*elems_per_core;
//        int end = begin+elems_per_core;

//        // if last core, sum also the remainder
//        if (mycore == Grappa::cores()-1) end+= nelems % Grappa::cores();

//        T partial_sum = zero;
//        for (int i = begin; i < end; i++) {
//            partial_sum = aggr(partial_sum, Grappa::delegate::read(seq+i));
//        }

//        total = Grappa::reduce()
//    });
//    return total;
//}


template<typename T>
struct MyVector {
    GlobalAddress<T> base;
    size_t size;

    MyVector(size_t size_) : base(global_alloc<T>(size_)), size(size_) {}
    MyVector(GlobalAddress<T> base_, size_t size_) : base(base_), size(size_) {}
};
template<typename T>
bool operator==(const MyVector<T>& p1, const MyVector<T>& p2) {
    return p1.size == p2.size && p1.base.raw_bits() == p2.base.raw_bits();
}



struct Point {
    double x;
    double y;

    Point(double x_, double y_): x(x_), y(y_) {}
    Point(): Point(0,0) {}
    Point(const Point& p): Point(p.x, p.y) {}

    Point operator/(double d) {
        return Point(x/d, y/d);
    }


    double modulus() {
        return sqrt(sq(x)+sq(y));
    }

};

typedef MyVector<Point> PointList;
typedef GlobalAddress<GlobalVector<Point>> GPointVector;


Point operator-(const Point& p1, const Point& p2) {
    return Point(p1.x - p2.x, p1.y - p2.y);
}
Point operator+(const Point& p1, const Point& p2) {
    return Point(p1.x + p2.x, p1.y + p2.y);
}
bool operator==(const Point& p1, const Point& p2) {
    p1.x == p2.x && p1.y == p2.y;
}

namespace std {

  template <>
  struct hash<Point>
  {
    std::size_t operator()(const Point& k) const
    {
      using std::size_t;
      using std::hash;
      using std::string;

      // Compute individual hash values for first,
      // second and third and combine them using XOR
      // and bit shifting:

      return ((hash<double>()(k.x)
               ^ (hash<double>()(k.y) << 1)) >> 1);
    }
  };

}



double dist(Point p1, Point p2) {
    return (p1 - p2).modulus();
}

Point average(GlobalAddress<Point> ps, size_t npoints) {
    return reduce<Point, collective_add>(ps, npoints, Point(0,0)) / npoints;
}



template <typename F>
GlobalHashMap<Point,GPointVector> groupBy(GlobalAddress<Point> seq, size_t nelems, F group) {

    GlobalAddress<GlobalHashMap<Point, GPointVector>> map = GlobalHashMap<Point, GPointVector>::create(nelems);
    forall(seq, nelems, [group, map, nelems](Point& p) {
        GPointVector *gpv = nullptr;
        if (map->lookup(p, gpv)) {
            (*gpv)->push(p);
        } else {
            GPointVector gpv = GlobalVector<Point>::create(nelems);
            map->insert(p, gpv);
            gpv->push(p);
        }
    });

}

// choices minBy { y => dist(x, y) }
GlobalAddress<Point> closest(Point x, PointList choices) {
    return minBy<Point>(choices.base, choices.size, [x] (Point y) { return dist(x,y); });
}


PointList clusters(PointList xs, PointList centroids) {
    auto map = groupBy(xs.base, xs.size, [centroids](Point p){ return closest(p, centroids); });
    GlobalAddress<GlobalVector<Point>> vec = GlobalVector<Point>::create(map.ncells());
    map.forall_entries([&vec](Point p, GlobalAddress<GlobalVector<Point>> v){
        vec->push(p);
    });
    return PointList(vec->base, vec->size());
}



void run(PointList xs) {
    PointList centroids(xs.base, n);
    for (size_t i = 0; i < iters; i++)  {
        auto cs = clusters(xs, centroids);
        PointList ps(cs.size);
        forall<unbound>(0, cs.size, [cs,ps](int64_t i){
            delegate::write(ps.base+i, average(cs.base+i, cs.size));
        });
    }
    clusters(xs, centroids);
}



Point mysum(const Point& p1, const Point& p2) {
    return p1+p2;
}


int main(int argc, char *argv[])
{

//    var centroids: [1..n] point;

//    var xs: [1..100000] point;

//    C.getJson();

//    for i in 1..100000 do
//            xs[i] = new point(C.getXAt(i), C.getYAt(i));

//    forall(0, executions, [] (int64_t k) {

//            forall(0, n, [=] (int64_t j) {
//                    delegate::write(centroids+j, Point(delegate::read(xs+j)));
//            });


//    });


    init(&argc, &argv);
    run([=] {
        PointList xs(n);
    });
    finalize();




    return 0;
}

