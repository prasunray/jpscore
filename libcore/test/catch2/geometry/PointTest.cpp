/*
 * This file is part of JuPedSim.
 *
 * JuPedSim is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * JuPedSim is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with JuPedSim. If not, see <http://www.gnu.org/licenses/>.
 *
 **/

#include "geometry/Point.hpp"

#include <catch2/catch.hpp>
#include <cmath>
#include <string>

const double PI = std::acos(-1);

TEST_CASE("geometry/Point", "[geometry][Point]")
{
    SECTION("ctor")
    {
        Point p;
        REQUIRE(p.x == 0);
        REQUIRE(p.y == 0);

        Point p2(1, 1);
        REQUIRE(p2.x == 1);
        REQUIRE(p2.y == 1);

        Point p3(p2);
        REQUIRE(p3.x == 1);
        REQUIRE(p3.y == 1);
    }

    SECTION("Setter")
    {
        Point p2(1, 1);
        p2.x = (0.5);
        p2.y = (-232.2);
        REQUIRE(p2.x == 0.5);
        REQUIRE(p2.y == -232.2);
    }

    SECTION("Norm")
    {
        Point p1(0, 0);
        REQUIRE(p1.Norm() == Approx(0));
        p1.x = (1);
        REQUIRE(p1.Norm() == Approx(1));
        p1.x = (-1);
        REQUIRE(p1.Norm() == Approx(1));
        p1.x = (3);
        REQUIRE(p1.Norm() == Approx(3));
        p1.x = (0);
        p1.y = (1);
        REQUIRE(p1.Norm() == Approx(1));
        p1.y = (-1);
        REQUIRE(p1.Norm() == Approx(1));
        p1.y = (3);
        REQUIRE(p1.Norm() == Approx(3));

        p1.x = (3);
        p1.y = (4);
        REQUIRE(p1.Norm() == Approx(5));
    }

    SECTION("to String")
    {
        std::string xpt[] = {"0.25", "1.25", "2.25", "3.25"};
        std::string ypt[] = {"10.25", "11.25", "12.25", "13.25"};
        Point p1;
        for(double i = 0.25; i < 4; ++i) {
            p1.x = (i);
            p1.y = (i + 10);
            REQUIRE(p1.toString() == "( " + xpt[int(i)] + " : " + ypt[int(i)] + " )");
        }

        Point p2(-2, -0.5);
        REQUIRE(p2.toString() == "( -2 : -0.5 )");
    }

    SECTION("Norm Molified")
    {
        Point p;
        for(double i = 0, j = 0.5; i < 10; ++i, ++j) {
            p.x = (i);
            p.y = (j);
            REQUIRE(p.NormMolified() == Approx(std::sqrt(std::pow(i, 2) + std::pow(j, 2) + 0.1)));
        }
    }

    SECTION("Norm Square")
    {
        Point p;
        for(double i = -5, j = 0.6; i < 5; ++i, ++j) {
            p.x = (i);
            p.y = (j);
            REQUIRE(p.NormSquare() == Approx(std::pow(i, 2) + std::pow(j, 2)));
        }
    }

    SECTION("Point Normalize")
    {
        Point p1(0.0001, 0.0001);
        Point p2(10, 10);
        p2 = p1.Normalized();
        REQUIRE(p2.x == Approx(0.0));
        REQUIRE(p2.y == Approx(0.0));

        for(double i = 0, j = -10; i < 5; ++i, ++j) {
            p1.x = (i);
            p1.y = (j);
            p2 = p1.Normalized();
            REQUIRE(p2.x == Approx(i / p1.Norm()));
            REQUIRE(p2.y == Approx(j / p1.Norm()));

            p2 = p1.NormalizedMolified();
            REQUIRE(p2.x == Approx(i / p1.NormMolified()));
            REQUIRE(p2.y == Approx(j / p1.NormMolified()));
        }
    }

    SECTION("Determinant")
    {
        Point p1(10, 5);
        Point p2;
        for(int i = 1; i < 5; ++i) {
            p2.x = (std::cos(PI / -i));
            p2.y = (std::sin(PI / i));
            REQUIRE(p1.Determinant(p2) == Approx(10 * std::sin(PI / i) - 5 * std::cos(PI / i)));
        }
    }

    SECTION("Scalar Product")
    {
        Point p1(10, 5);
        Point p2;
        for(int i = 1; i < 5; ++i) {
            p2.x = (std::cos(PI / i));
            p2.y = (std::sin(PI / -i));
            REQUIRE(p1.ScalarProduct(p2) == Approx(10 * p2.x + 5 * p2.y));
        }
    }

    SECTION("Operator overloading")
    {
        for(int i = 1; i < 10; ++i) {
            Point p1(i, -i * 10);
            Point p2(cos(PI / i), sin(PI / i));

            Point sum = p1 + p2;
            REQUIRE(sum.x == Approx(i + std::cos(PI / i)));
            REQUIRE(sum.y == Approx(-i * 10 + std::sin(PI / i)));
            Point sub = p1 - p2;
            REQUIRE(sub.x == Approx(i - std::cos(PI / i)));
            REQUIRE(sub.y == Approx(-i * 10 - std::sin(PI / i)));
            Point mul = p2 * i;
            REQUIRE(mul.x == Approx(i * std::cos(PI / i)));
            REQUIRE(mul.y == Approx(i * std::sin(PI / i)));
            Point pluseq(i, i);
            pluseq += p1;
            REQUIRE(pluseq.x == Approx(i + i));
            REQUIRE(pluseq.y == Approx(-i * 10 + i));
            Point div = p2 / 1E-7;
            REQUIRE(div.x == Approx(p2.x));
            REQUIRE(div.y == Approx(p2.y));
            div = p2 / 2;
            REQUIRE(div.x == Approx(p2.x / 2));
            REQUIRE(div.y == Approx(p2.y / 2));

            REQUIRE(p1 != p2);
            p1.x = (p2.x);
            p1.y = (p2.y);
            REQUIRE(p1 == p2);
        }
    }

    SECTION("Coord Transform to Ellipse")
    {
        for(int i = 1; i < 5; ++i) {
            Point p1(i * 10, i / 10);
            Point center(i, -i);
            Point check = (p1 - center).Rotate(std::cos(PI / i), -std::sin(PI / i));
            Point transform =
                p1.TransformToEllipseCoordinates(center, std::cos(PI / i), std::sin(PI / i));
            REQUIRE(transform == check);
        }
    }

    SECTION("Coord Transforma to Cartesian")
    {
        for(int i = 1; i < 5; ++i) {
            Point p1(i * 10, i / 10);
            Point center(-i, i);
            Point transform =
                p1.TransformToCartesianCoordinates(center, std::cos(PI / i), std::sin(PI / i));
            Point check = p1.Rotate(std::cos(PI / i), std::sin(PI / i));
            REQUIRE(transform.x == Approx(check.x - i));
            REQUIRE(transform.y == Approx(check.y + i));
        }
    }
}
