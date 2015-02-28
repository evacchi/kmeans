package com.example;

import lombok.AllArgsConstructor;
import lombok.Data;

/**
 * Created by evacchi on 28/02/15.
 */
@Data
@AllArgsConstructor
class Point {
    double x,y;

    public Point plus(Point p2) {
        return new Point(x + p2.getX(), y + p2.getY());
    }
    public Point minus(Point p2) {
        return new Point(x - p2.getX(), y - p2.getY());
    }
    public Point div(double d) {
        return new Point(x/d, y/d);
    }
    public Double getModulus() { return Math.sqrt(sq(x) + sq(y)); }
    private double sq(double x) { return x*x; }

}