/* Даны два выпуклых многоугольника на плоскости. В первом n точек, во втором m.
 * Определите, пересекаются ли они за O(n + m).
 */
#include <cmath>
#include <iostream>
#include <utility>
#include <vector>

using std::cin;
using std::cout;
using std::endl;
using std::vector;

const static double PI = 3.14159265358;
const static double EPS = 0.00000000001;


class Point {
public:
    Point() : x_(0), y_(0) {}
    Point(const Point& other) : x_(other.x_), y_(other.y_) {}
    Point(double x, double y) : x_(x), y_(y) {}
    Point operator + (const Point& other) const {
        return Point(x_ + other.x_, y_ + other.y_);
    }
    Point operator - (const Point& other) const {
        return Point(x_ - other.x_, y_ - other.y_);
    }
    Point& operator = (const Point& other) {
        (*this).y_ = other.y_;
        (*this).x_ = other.x_;
        return *this;
    }
    bool operator == (const Point& other) {
        return x_ == other.x_ && y_ == other.y_;
    }
    double Angle() const;

    double GetX()const {
        return x_;
    }

    double GetY()const {
        return y_;
    }

    double Length() {
        return sqrt(x_ * x_ + y_ * y_);
    }

    bool IsToTheLeft(const Point& first, const Point& last) const;

private:
    double x_;
    double y_;
};


double Point::Angle() const {
    if (y_ == 0) {
        if (x_ >= 0){
            return 0.0;
        } else {
            return PI;
        }
    }
    if (x_ == 0) {
        if (y_ > 0) {
            return PI / 2;
        } else {
            return 3 * (PI / 2);
        }
    }
    // y_ != 0 && x_ != 0
    double arctan = atan(y_ / x_);
    if (x_ > 0) {
        if (arctan < 0){
            return arctan + 2 * PI;
        }
        return arctan;
    } else {
        return arctan + PI;
    }
}

bool Point::IsToTheLeft(const Point& first, const Point& last) const {
    /* Проверяет, что данная точка лежит либо на отрезке [a,b],
     * либо в левой полуплоскости относительно луча ab (в направлении от a до b)
     */
    Point middle = *this;
    Point a = last - first;

    Point b = middle - first;

    double ax = a.GetX(), bx = b.GetX();
    double ay = a.GetY(), by = b.GetY();
    double product = ax * by - bx * ay;

    if (product > EPS) { //strictly to the left
        return true;
    }
    if (product < -EPS) { //strictly to the right
        return false;
    }
    if (middle == last || middle == first) { //endpoint
        return true;
    }
    if (ax * bx < -EPS && ay * by < -EPS) {//behind
        return false;
    }
    return a.Length() >= b.Length() + EPS;
}

vector<Point> MinkowskiSum(const vector<Point>& first, const vector<Point>& second) {
    int i = 0;
    int j = 0;
    int vert_num_1 = first.size();
    int vert_num_2 = second.size();
    vector<Point> first_pol = first;
    vector<Point> second_pol = second;
    first_pol.push_back(first[0]);
    first_pol.push_back(first[1]);
    second_pol.push_back(second[0]);
    second_pol.push_back(second[1]);
    vector<Point> answer;
    while (i < vert_num_1 || j < vert_num_2) {
        answer.push_back(first_pol[i] + second_pol[j]);
        double angle_i = (first_pol[i + 1] - first_pol[i]).Angle();
        if (i >= vert_num_1) {
            angle_i += 2 * PI;
        }
        double angle_j = (second_pol[j + 1] - second_pol[j]).Angle();
        if (j >= vert_num_2) {
            angle_j += 2 * PI;
        }
        if (angle_i + EPS < angle_j) {
            ++i;
        } else if (angle_i > angle_j + EPS) {
            ++j;
        } else {
            ++i;
            ++j;
        }
    }
    return answer;
}

vector<Point> SortVertices(const vector<Point>& vertices_) {
    vector<Point> result;
    int min_index = 0;
    double min_x = vertices_[0].GetX();
    double min_y = vertices_[0].GetY();
    for (int i = 1; i < vertices_.size(); ++i) {
        double curr_x = vertices_[i].GetX();
        double curr_y = vertices_[i].GetY();
        if ((EPS < min_y - curr_y) || ((EPS >= min_y - curr_y && min_y - curr_y > 0.0)
                                       && (EPS < min_x - curr_x))) {
            min_y = curr_y;
            min_x = curr_x;
            min_index = i;
        }
    }
    result.emplace_back(min_x, min_y);
    for (int i = min_index - 1; i >= 0; --i) {
        result.emplace_back(vertices_[i].GetX(), vertices_[i].GetY());
    }
    for (int i = vertices_.size() - 1; i > min_index; --i) {
        result.emplace_back(vertices_[i].GetX(), vertices_[i].GetY());
    }
    return result;
}

bool PointBelongsToPolygon(const Point &point, const vector<Point> &polygon) {
    for (int i = 0; i < polygon.size() - 1; ++i) {
        if (!point.IsToTheLeft(polygon[i], polygon[i + 1])) {
            return false;
        }
    }
    return point.IsToTheLeft(polygon[polygon.size() - 1], polygon[0]);
}

int main() {
    int n;
    cin >> n;
    vector<Point> first_polygon;
    for (int i = 0; i < n; ++i) {
        double x, y;
        cin >> x >> y;
        first_polygon.emplace_back(x, y);
    }

    int m;
    cin >> m;
    vector<Point> second_polygon;
    for (int i = 0; i < m; ++i) {
        double x, y;
        cin >> x >> y;
        second_polygon.emplace_back(-x, -y);
    }
    first_polygon = SortVertices(first_polygon);
    second_polygon = SortVertices(second_polygon);
    vector<Point> answer = MinkowskiSum(first_polygon, second_polygon);
    Point zero;
    if (PointBelongsToPolygon(zero, answer)) {
        cout << "YES";
    } else {
        cout << "NO";
    }
}
