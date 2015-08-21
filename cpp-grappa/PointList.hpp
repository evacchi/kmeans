#pragma once
#include "Grappa.hpp"
#include "Point.hpp"


using namespace Grappa;

template<typename T>
struct MyVector {
    GlobalAddress<T> base;
    size_t size;

    MyVector(size_t size_) : base(global_alloc<T>(size_)), size(size_) {}
    MyVector() {}
    MyVector(GlobalAddress<T> base_, size_t size_) : base(base_), size(size_) {}
};
template<typename T>
bool operator==(const MyVector<T>& p1, const MyVector<T>& p2) {
    return p1.size == p2.size && p1.base.raw_bits() == p2.base.raw_bits();
}


typedef MyVector<Point> PointList;
typedef GlobalAddress<GlobalVector<GlobalAddress<Point>>> GPointVector;

