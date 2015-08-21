#pragma once


#include <jansson.h>
#include <Grappa.hpp>


using namespace Grappa;

PointList from_json() {
    int NPOINTS = 100000;
    int i=0;
    json_t *json;
    json_error_t error;
    size_t index;
    long int temp = 0;
    json_t *value;

    std::cout << "reading points... " ;

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

    GlobalAddress<Point> points = global_alloc<Point>(NPOINTS);
    
    std::cout << "writing to GlobalMem... " ;


    NPOINTS = 100;

    for(int i=0; i < NPOINTS; i++){
        Point p(xs[i],ys[i]);
        delegate::write(points+i,p);
    };

    std::cout << "done" << std::endl;

    return PointList(points, NPOINTS);
}

