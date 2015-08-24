#pragma once
#include "Grappa.hpp"
#include "Point.hpp"


using namespace Grappa;

struct PointList {
    GlobalAddress<Point> base;
    size_t size;

    PointList(size_t size_) : base(global_alloc<Point>(size_)), size(size_) {}
    PointList() {}
    PointList(GlobalAddress<Point> base_, size_t size_) : base(base_), size(size_) {}
};
bool operator==(const PointList& p1, const PointList& p2) {
    return p1.size == p2.size && p1.base.raw_bits() == p2.base.raw_bits();
}

