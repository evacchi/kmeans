package com.example;

import java.util.*;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import static java.util.Arrays.stream;
import static java.util.stream.Collectors.*;
import static java.util.stream.Collectors.groupingBy;
import static java.util.stream.Collectors.minBy;

/**
 * Created by evacchi on 27/02/15.
 */
public class KMeans {

    int n = 10;
    int iters = 15;

    public void run(Point[] xs) {
        Stream<Point> centroids = stream(xs).limit(n);
        for (int i = 0; i < iters; i++) {
            centroids = clusters(xs, centroids.toArray(Point[]::new))
                        .stream().map(this::average);
        }
        clusters(xs, centroids.toArray(Point[]::new));
    }

    public Collection<List<Point>> clusters(Point[] xs, Point[] centroids) {
        return stream(xs).collect(groupingBy((Point x) -> closest(x, centroids))).values();
    }

    public Point closest(final Point x, Point[] choices) {
        return stream(choices)
            .collect(minBy((y1, y2) -> dist(x, y1) <= dist(x, y2) ? -1 : 1)).get();
    }
    
    public double dist(Point x, Point y) { return x.minus(y).getModulus(); }

    public Point average(List<Point> xs) {
        return xs.stream().reduce(Point::plus).get().div(xs.size());
    }

}