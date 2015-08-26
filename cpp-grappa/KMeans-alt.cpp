#include <Grappa.hpp>
#include <GlobalVector.hpp>
#include <GlobalHashMap.hpp>
#include <GlobalHashSet.hpp>
#include <limits>
#include <cassert>
#include <chrono>

#include "KMeans.hpp"

typedef GlobalAddress<int> GInt;
using namespace Grappa;

void reset_centroids(GPoint _points, GPoint _centroids, size_t num_points, size_t num_centroids) {
    for(int i = 0; i < num_centroids; i++){
        Point p = delegate::read(_points+i);
        delegate::write(_centroids+i, p);
    }
}

void reset_clusters(GInt _clusters, size_t num_points) {
    for(int i = 0; i < num_points; i++){
        delegate::write(_clusters+i, 0);
    }
}



int closest(Point point, Point* centroids, int num_centroids)
{
    int _closest=0;
    double min_d=dist(point,centroids[0]);
    
    for(int i=1;i<num_centroids;i++)
    {   
        int d = dist(point,centroids[i]);
        if(min_d >= d)
        {
            _closest=i;
            min_d=d;
        }
    }
    return _closest;
}

void update_centroids(GPoint points, GInt clusters, GPoint centroids, int num_clusters, int num_points)
{

    DVLOG(3) << "update_centroids";
    Point new_centroids[num_clusters];
    int   population[num_clusters];

    // for all cluster assignments
    // count the points in that cluster 
    // forall_here(0, num_clusters, [=](int64_t i) {
    //     int k = delegate::read(clusters + i);
    //     population[k]++;

    //     Point p = delegate::read(points+i);
    //     new_centroids[k] += p;

    // });
    // forall_here(0, num_clusters, [=](int64_t i) {
    //     if(population[i] != 0)
    //         new_centroids[i] /= population[i];
    // });
    // forall_here(0, num_clusters, [=](int64_t i) {
    //     delegate::write(centroids+i, new_centroids[i]);
    // });

    for(int64_t i = 0; i < num_clusters; i++) {
        int k = delegate::read(clusters + i);
        population[k]++;

        Point p = delegate::read(points + i);
        new_centroids[k] += p;

    };

    DVLOG(3) << "counters updated";

    for(int64_t i = 0; i < num_clusters; i++) {
        if(population[i] != 0)
            new_centroids[i] /= population[i];
    };
    for(int64_t i = 0; i < num_clusters; i++) {
        delegate::write(centroids+i, new_centroids[i]);
    };


    DVLOG(3) << "centroids updated";


}

void main_body() {


    int repetitions = 2;
    
    int num_clusters = 10;
    int num_points   = 10000;
    int niters = 15;


    int job_size = num_points / Grappa::cores();
    
    std::chrono::time_point<std::chrono::system_clock> start, end;
    std::chrono::duration<double> elapsed_seconds;
        
    Grappa::run([=] {  

        GPoint gcentroids = global_alloc<Point>(num_clusters);
        GPoint gpoints    = global_alloc<Point>(num_points);
        GInt   gclusters  = global_alloc<int>(num_points);
        
        read_points(gpoints, num_points);


        for (int t = 0; t < repetitions; t++) {
            reset_centroids(gpoints, gcentroids, num_points, num_clusters);
            reset_clusters(gclusters, num_points);

            start = std::chrono::system_clock::now();


            DVLOG(3) << "points read. begin.";
            
            //Main job of master processor is done here     
            for(int iter = 0; iter < niters; iter++)
            {   
                DVLOG(3) << "iter " << iter;

                on_all_cores([=]{

                    int64_t my_offset = job_size * Grappa::mycore();
                    int64_t my_job_size = job_size;

                    if (Grappa::mycore() == Grappa::cores() - 1)
                        my_job_size += num_points % Grappa::cores();

                    // double decl to work-around C++ capture behavior w/ array refs

                    Point _points[my_job_size];
                    Point* points = _points;
                    
                    Point _centroids[num_clusters];
                    Point* centroids = _centroids;

                    DVLOG(3) << "fetch centroids " << my_job_size;

                
                    // fetch points from gmemory
                    forall_here(0, num_clusters, [=](int64_t i) {
                        centroids[i] = delegate::read(gcentroids+i);
                    });

                    DVLOG(3) << "fetch points " << my_job_size;

                
                    forall_here(0, my_job_size, [=](int64_t i) {
                        points[i] = delegate::read(gpoints + my_offset + i);
                    });

                    DVLOG(3) << "points were fetched. start";

                    // compute & write back gclusters to gmemory
                    forall_here(my_offset, my_job_size, [=](int64_t j) {
                        delegate::write(gclusters + j, my_offset + closest(points[j], centroids, num_clusters));
                    });
                    DVLOG(3) << "finished work.";


                });
                
                update_centroids(gpoints, gclusters, gcentroids, num_clusters, num_points);

            }

            end = std::chrono::system_clock::now();
            elapsed_seconds += ( end-start );

        }


        std::time_t end_time = std::chrono::system_clock::to_time_t(end);
     
        std::cout << "finished computation at " << std::ctime(&end_time)
                  << "elapsed time: " << elapsed_seconds.count() / (double) repetitions << "s\n";


        
    });
}

int main(int argc, char* argv[])
{
    Grappa::init(&argc, &argv);
    main_body();
    Grappa::finalize();
    return 0;
}
