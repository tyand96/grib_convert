#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <CoordinateSystem.hpp>
#include <stdexcept>

TEST_CASE("Point Equality", "[Point]") {
    SECTION("Equal Points") {
        Point p1{1.0f, 2.0f};
        Point p2{1.0f, 2.0f};
        REQUIRE(p1 == p2);
    }

    SECTION("Unequal Points") {
        Point p1{1.0f, 2.0f};
        Point p2{3.0f, 4.0f};
        REQUIRE_FALSE(p1 == p2);
    }
}

TEST_CASE("Grid Functionality", "[Grid]") {
    SECTION("Equality") {
        CoordinateSystem::Grid grid1;
        grid1.points.insert({1.0f, 2.0f});
        grid1.points.insert({3.0f, 4.0f});

        CoordinateSystem::Grid grid2;
        grid2.points.insert({1.0f, 2.0f});
        grid2.points.insert({3.0f, 4.0f});

        REQUIRE(grid1 == grid2);
    }

    SECTION("Inequality") {
        CoordinateSystem::Grid grid1;
        grid1.points.insert({1.0f, 2.0f});
        grid1.points.insert({3.0f, 4.0f});

        CoordinateSystem::Grid grid2;
        grid2.points.insert({1.0f, 2.0f});
        grid2.points.insert({5.0f, 6.0f});

        REQUIRE_FALSE(grid1 == grid2);
        REQUIRE(grid1 != grid2);
    }

    SECTION("Point Validity") {
        CoordinateSystem::Grid grid;
        grid.points.insert({1.0f, 2.0f});
        grid.points.insert({3.0f, 4.0f});

        REQUIRE(grid.isPointValid(1.0f, 2.0f));
        REQUIRE_FALSE(grid.isPointValid(5.0f, 6.0f));
    }

    SECTION("Contains") {
        CoordinateSystem::Grid grid1;
        grid1.points.insert({1.0f, 2.0f});
        grid1.points.insert({3.0f, 4.0f});

        CoordinateSystem::Grid grid2;
        grid2.points.insert({1.0f, 2.0f});

        REQUIRE(grid1.contains(grid2));
        REQUIRE_FALSE(grid2.contains(grid1));
    }

    SECTION("Overlaps") {
        CoordinateSystem::Grid grid1;
        grid1.points.insert({1.0f, 2.0f});
        grid1.points.insert({3.0f, 4.0f});

        CoordinateSystem::Grid grid2;
        grid2.points.insert({3.0f, 4.0f});
        grid2.points.insert({5.0f, 6.0f});

        REQUIRE(grid1.overlaps(grid2));
        REQUIRE(grid2.overlaps(grid1));

        CoordinateSystem::Grid grid3;
        grid3.points.insert({7.0f, 8.0f});

        REQUIRE_FALSE(grid1.overlaps(grid3));
    }

    SECTION("Clone") {
        CoordinateSystem::Grid grid;
        grid.points.insert({1.0f, 2.0f});
        grid.points.insert({3.0f, 4.0f});

        auto clonedGrid = grid.clone();
        REQUIRE(*clonedGrid == grid);

        // Change the second grid
        clonedGrid->points.insert({5.0f, 6.0f});
        REQUIRE_FALSE(*clonedGrid == grid); // They should not be equal anymore
    }

    SECTION("Create Regular Grid") {
        auto grid = CoordinateSystem::Grid::createRegularGrid(
            std::unordered_set<float>{1.0f, 2.0f, 3.0f},
            std::unordered_set<float>{4.0f, 5.0f, 6.0f}
        );

        REQUIRE(grid.points.size() == 9); // 3 latitudes * 3 longitudes
        REQUIRE(grid.isPointValid(1.0f, 4.0f));
        REQUIRE(grid.isPointValid(2.0f, 5.0f));
        REQUIRE_FALSE(grid.isPointValid(7.0f, 8.0f));
    }
}

TEST_CASE("Coordinate System Construction", "[CoordinateSystem]") {
    SECTION("Default") {
        CoordinateSystem coords;
        std::unordered_set<Point> points;

        REQUIRE(coords.getGridType() == CoordinateSystem::GridType::UNDEFINED);
        REQUIRE(coords.getGrid() == nullptr);
    }

    SECTION("Constructor with Grid") {
        CoordinateSystem::Grid grid = CoordinateSystem::Grid::createRegularGrid(
            std::unordered_set<float>{1, 2, 3},
            std::unordered_set<float>{4, 5, 6}
        );
        CoordinateSystem coords(
            grid.clone(),
            CoordinateSystem::GridType::REGULAR_LATLON
        );

        REQUIRE(coords.getGridType() == CoordinateSystem::GridType::REGULAR_LATLON);
        REQUIRE(*coords.getGrid() == grid);
    }

    SECTION("Constructor with Points") {
        std::unordered_set<Point> points = {
            {1.0f, 4.0f},
            {2.0f, 5.0f},
            {3.0f, 6.0f}
        };
        CoordinateSystem::Grid grid;
        grid.points = points;
        CoordinateSystem coords(
            std::make_unique<CoordinateSystem::Grid>(grid),
            CoordinateSystem::GridType::REGULAR_LATLON
        );

        REQUIRE(coords.getGridType() == CoordinateSystem::GridType::REGULAR_LATLON);
        REQUIRE(coords.getGrid()->points == points);

    }

    SECTION("Copy") {
        CoordinateSystem coords = CoordinateSystem::createRegularGrid(
            std::unordered_set<float>{1, 2, 3},
            std::unordered_set<float>{4, 5, 6}
        );

        auto coords2 = coords;

        REQUIRE(coords == coords2);
    }

    SECTION("Move") {
        CoordinateSystem coords = CoordinateSystem::createRegularGrid(
            std::unordered_set<float>{1, 2, 3},
            std::unordered_set<float>{4, 5, 6}
        );

        CoordinateSystem coords2 = std::move(coords);

        REQUIRE(coords2.getGridType() == CoordinateSystem::GridType::REGULAR_LATLON);
        REQUIRE(coords2.getGrid() != nullptr);
        REQUIRE(coords2.getGrid()->points.size() == 9); // 3 latitudes
    }
}

TEST_CASE("Comparisons") {
    SECTION("Equality") {
        CoordinateSystem coords1 = CoordinateSystem::createRegularGrid(
            std::unordered_set<float>{1, 2, 3},
            std::unordered_set<float>{4, 5, 6}
        );
        CoordinateSystem coords2 = CoordinateSystem::createRegularGrid(
            std::unordered_set<float>{1, 2, 3},
            std::unordered_set<float>{4, 5, 6}
        );

        REQUIRE(coords1 == coords2);
    }

    SECTION("Inequality") {
        CoordinateSystem coords1 = CoordinateSystem::createRegularGrid(
            std::unordered_set<float>{1, 2, 3},
            std::unordered_set<float>{4, 5, 6}
        );
        CoordinateSystem coords2 = CoordinateSystem::createRegularGrid(
            std::unordered_set<float>{1, 2, 7},
            std::unordered_set<float>{4, 5, 6}
        );

        REQUIRE(coords1 != coords2);
    }
}