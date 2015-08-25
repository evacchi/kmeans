#pragma once
#include <Grappa.hpp>


double sq(double x) { return x*x; }


struct Point {
    double x;
    double y;

    Point(double x_, double y_): x(x_), y(y_) {}
    Point(): Point(0,0) {}
    Point(const Point& p): Point(p.x, p.y) {}

    Point operator/(double d) { return Point(x/d, y/d); }
    Point& operator/=(const double& d) { x/=d; y/=d; return *this; }
    Point& operator+=(const Point& other) { x+=other.x; y+=other.y; return *this; }

    double modulus() { return sqrt(sq(x)+sq(y)); }

    size_t hash() ;

};

typedef GlobalAddress<Point> GPoint;


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

      return ((hash<double>()(k.x)
               ^ (hash<double>()(k.y) << 1)) >> 1);
    }
  };
}

size_t Point::hash() { return std::hash<Point>()(*this); }

std::ostream& operator<<(std::ostream& stream, const Point& p) {
    return stream << "Point(" << ( (double) p.x ) << ", " << ( (double) p.y) << ")" ;
}



double dist(Point p1, Point p2) {
    return (p1 - p2).modulus();
}
