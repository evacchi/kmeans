#pragma once


#include <jansson.h>
#include <Grappa.hpp>


using namespace Grappa;

void read_points(GPoint _points, GPoint _centroids, size_t num_points, size_t num_clusters) {
    int i=0;
    json_t *json;
    json_error_t error;
    size_t index;
    long int temp = 0;
    json_t *value;

    std::cout << "reading points... " ;

    double* xs = new double[num_points];
    double* ys = new double[num_points];

    json = json_load_file("../points.json", 0, &error);
    if(!json) {
        throw std::runtime_error("Error parsing Json file");
    }

    json_array_foreach(json, index, value) {
        xs[index] = json_number_value(json_array_get(value,0));
        ys[index] = json_number_value(json_array_get(value,1));
    }

    std::cout << "done " << std::endl;

    
    std::cout << "writing to GlobalMem... " ;


    for(int i = 0; i < num_points; i++){
        Point p(xs[i],ys[i]);
        delegate::write(_points+i,p);
        if (i < num_clusters) {
            delegate::write(_centroids+i, p);
        }
    };

    std::cout << "done" << std::endl;

}



GlobalAddress<Point> from_json(size_t NPOINTS) {
    int i=0;
    json_t *json;
    json_error_t error;
    size_t index;
    long int temp = 0;
    json_t *value;

    std::cout << "reading points... " ;


    GlobalAddress<Point> points = global_alloc<Point>(NPOINTS);

    double* xs = new double[NPOINTS];
    double* ys = new double[NPOINTS];



    json = json_load_file("../points.json", 0, &error);
    if(!json) {
        throw std::runtime_error("Error parsing Json file");
    }

    json_array_foreach(json, index, value) {
        xs[index] = json_number_value(json_array_get(value,0));
        ys[index] = json_number_value(json_array_get(value,1));
    }

    std::cout << "done " << std::endl;

    
    std::cout << "writing to GlobalMem... " ;


    for(int i=0; i < NPOINTS; i++){
        Point p(xs[i],ys[i]);
        delegate::write(points+i,p);
    };

    std::cout << "done" << std::endl;

    return points;
}

