/** 
 *  These examples demonstrate how to use an R*Tree virtual table for (geo)spatial searching.
 *  
 *  For information about the SQLite R*Tree Module see https://www.sqlite.org/rtree.html
 */

#include <sqlite_orm/sqlite_orm.h>

// note: clang currently has problems to use constexpr variables in lambdas
#if defined(SQLITE_ENABLE_RTREE) && SQLITE_VERSION_NUMBER >= 3024000 && defined(SQLITE_ORM_CPP20_RANGES_SUPPORTED) &&  \
    defined(SQLITE_ORM_WITH_CPP20_ALIASES) && !defined(__clang__)
#define ENABLE_THIS_EXAMPLE
#endif

#ifdef ENABLE_THIS_EXAMPLE
#include <iostream>
#include <string>
#include <numbers>
#include <cmath>

using namespace sqlite_orm;
using std::cout, std::endl;

void sqlite_office() {
    struct DemoIndex {
        int64 id;
        float minX, maxX;
        float minY, maxY;
    };
    constexpr orm_table_reference auto demo_index = c<DemoIndex>();

    auto storage = make_storage("",
                                make_virtual_table("demo_index",
                                                   using_rtree(make_column("id", &DemoIndex::id, primary_key()),
                                                               make_column("minX", &DemoIndex::minX),
                                                               make_column("maxX", &DemoIndex::maxX),
                                                               make_column("minY", &DemoIndex::minY),
                                                               make_column("maxY", &DemoIndex::maxY))));

    storage.sync_schema();

    storage.insert(into<demo_index>(),
                   columns(&DemoIndex::id, &DemoIndex::minX, &DemoIndex::maxX, &DemoIndex::minY, &DemoIndex::maxY),
                   values(std::tuple(28215, -80.781227, -80.604706, 35.208813, 35.297367),
                          std::tuple(28216, -80.957283, -80.840599, 35.235920, 35.367825),
                          std::tuple(28217, -80.960869, -80.869431, 35.133682, 35.208233),
                          std::tuple(28226, -80.878983, -80.778275, 35.060287, 35.154446),
                          std::tuple(28227, -80.745544, -80.555382, 35.130215, 35.236916),
                          std::tuple(28244, -80.844208, -80.841988, 35.223728, 35.225471),
                          std::tuple(28262, -80.809074, -80.682938, 35.276207, 35.377747),
                          std::tuple(28269, -80.851471, -80.735718, 35.272560, 35.407925),
                          std::tuple(28270, -80.794983, -80.728966, 35.059872, 35.161823),
                          std::tuple(28273, -80.994766, -80.875259, 35.074734, 35.172836),
                          std::tuple(28277, -80.876793, -80.767586, 35.001709, 35.101063),
                          std::tuple(28278, -81.058029, -80.956375, 35.044701, 35.223812),
                          std::tuple(28280, -80.844208, -80.841972, 35.225468, 35.227203),
                          std::tuple(28282, -80.846382, -80.844193, 35.223972, 35.225655)));

    auto zipCodesOfSQLiteProjectLocation = storage.select(
        demo_index->*&DemoIndex::id,
        where(demo_index->*&DemoIndex::minX <= -80.77470 and demo_index->*&DemoIndex::maxX >= -80.77470 and
              demo_index->*&DemoIndex::minY <= 35.37785 and demo_index->*&DemoIndex::maxY >= 35.37785));
    cout << "Zipcodes that might service the main office of the SQLite project:" << endl;
    for (int64 zipCode: zipCodesOfSQLiteProjectLocation) {
        cout << '\t' << zipCode << endl;
    }

    constexpr orm_table_alias auto a = "a"_alias.for_<demo_index>();
    constexpr orm_table_alias auto b = "b"_alias.for_<demo_index>();
    auto zipCode28269BoundingBox = storage.select(
        a->*&DemoIndex::id,
        where(a->*&DemoIndex::maxX >= b->*&DemoIndex::minX and a->*&DemoIndex::minX <= b->*&DemoIndex::maxX and
              a->*&DemoIndex::maxY >= b->*&DemoIndex::minY and a->*&DemoIndex::minY <= b->*&DemoIndex::maxY and
              b->*&DemoIndex::id == 28269));

    cout << "Zipcode bounding boxes that overlap with the 28269 zipcode:" << endl;
    for (int64 zipCode: zipCode28269BoundingBox) {
        cout << '\t' << zipCode << endl;
    }

    auto zipCodesOverlappingWith35thParallel =
        storage.select(&DemoIndex::id,
                       where(demo_index->*&DemoIndex::maxY >= 35.1 and demo_index->*&DemoIndex::minY <= 35.1));
    cout << "Zipcodes that overlap with the 35.1th parallel:" << endl;
    for (int64 zipCode: zipCodesOverlappingWith35thParallel) {
        cout << '\t' << zipCode << endl;
    }
}

static float degrees_to_radians(float d) {
    return (d / 180.0f) * (std::numbers::pi_v<float>);
}

// Calculate distance in km between two coordinates
static float haversine_distance(float lat1, float lon1, float lat2, float lon2) {
    using std::pow, std::sqrt, std::sin, std::cos, std::atan2;
    constexpr float R = 6371.f;  // Earth radius in km
    const float dlat = degrees_to_radians(lat2 - lat1);
    const float dlon = degrees_to_radians(lon2 - lon1);
    const float a = pow(sin(dlat / 2.f), 2) +
                    cos(degrees_to_radians(lat1)) * cos(degrees_to_radians(lat2)) * pow(sin(dlon / 2.f), 2);
    const float c = 2.f * atan2(sqrt(a), sqrt(1.f - a));
    return R * c;
}

void nearby_restaurants() {
    struct Restaurant {
        int64 id = 0;
        float minLat = 0;
        float maxLat = 0;
        float minLon = 0;
        float maxLon = 0;

        // auxiliary columns
        // --

        std::string name;
        std::string cuisine;
    };
    static constexpr orm_table_reference auto restaurant = c<Restaurant>();

    struct NearbyRestaurant {
        int64 id = 0;
        std::string name;
        std::string cuisine;
        float distance_km = 0;
    };

    auto storage =
        make_storage("",
                     make_virtual_table("restaurant",
                                        using_rtree(make_column("id", &Restaurant::id, primary_key()),
                                                    make_column("minLat", &Restaurant::minLat),
                                                    make_column("maxLat", &Restaurant::maxLat),
                                                    make_column("minLon", &Restaurant::minLon),
                                                    make_column("maxLon", &Restaurant::maxLon),
                                                    make_column("name", &Restaurant::name, auxiliary()),
                                                    make_column("cuisine", &Restaurant::cuisine, auxiliary()))));

    storage.sync_schema();

    storage.insert(into<restaurant>(),
                   columns(&Restaurant::id,
                           &Restaurant::minLat,
                           &Restaurant::maxLat,
                           &Restaurant::minLon,
                           &Restaurant::maxLon,
                           &Restaurant::name,
                           &Restaurant::cuisine),
                   values(std::tuple(1, 40.7301, 40.7301, -73.9352, -73.9352, "Joe's Pizza", "Italian"),
                          std::tuple(2, 40.7223, 40.7223, -73.9874, -73.9874, "Katz's Delicatessen", "Deli"),
                          std::tuple(3, 40.7411, 40.7411, -73.9897, -73.9897, "Shake Shack", "Burgers"),
                          std::tuple(4, 40.7624, 40.7624, -73.9809, -73.9809, "Le Bernardin", "French"),
                          std::tuple(5, 40.7296, 40.7296, -73.9854, -73.9854, "Momofuku Noodle Bar", "Asian"),
                          std::tuple(6, 40.7097, 40.7097, -73.9622, -73.9622, "Peter Luger", "Steakhouse"),
                          std::tuple(7, 40.6251, 40.6251, -73.9613, -73.9613, "Di Fara Pizza", "Italian"),
                          std::tuple(8, 40.7226, 40.7226, -73.9887, -73.9887, "Russ & Daughters", "Deli"),
                          std::tuple(9, 40.7419, 40.7419, -73.9873, -73.9873, "Eleven Madison Park", "Fine Dining"),
                          std::tuple(10, 40.7184, 40.7184, -73.9972, -73.9972, "Xi'an Famous Foods", "Chinese")));

    // Find restaurants within radius_km of given coordinates
    const auto find_nearby_restaurants =
        [&storage](float lat, float lon, float radius_km) -> std::vector<NearbyRestaurant> {
        // Convert radius to approximate degrees (rough approximation)
        // 1 degree ~ 111 km
        const float delta = radius_km / 111.f;

        std::vector<NearbyRestaurant> results;
        // Query R-tree directly with auxiliary columns
        for (auto [rid, rlat, rlon, name, cuisine]:
             storage.iterate(select(columns(restaurant->*&Restaurant::id,
                                            restaurant->*&Restaurant::minLat,
                                            restaurant->*&Restaurant::minLon,
                                            restaurant->*&Restaurant::name,
                                            restaurant->*&Restaurant::cuisine),
                                    where(restaurant->*&Restaurant::minLat >= lat - delta and
                                          restaurant->*&Restaurant::maxLat <= lat + delta and
                                          restaurant->*&Restaurant::minLon >= lon - delta and
                                          restaurant->*&Restaurant::maxLon <= lon + delta)))) {
            const float dist = haversine_distance(lat, lon, rlat, rlon);
            if (dist <= radius_km) {
                results.emplace_back(rid, name, cuisine, std::round(dist * 100.f) / 100.f);
            }
        }

        // Sort by distance
        std::ranges::sort(results, std::ranges::less{}, &NearbyRestaurant::distance_km);
        return results;
    };

    cout << "Restaurants near Times Square (40.758, -73.985)\n" << "  Within 2 km:" << endl;
    for (const NearbyRestaurant& nearby: find_nearby_restaurants(40.758f, -73.985f, 2.f)) {
        cout << '\t' << nearby.name << " (" << nearby.cuisine << ") - " << nearby.distance_km << " km away" << endl;
    }

    cout << "Restaurants near East Village (40.726, -73.982)\n" << "  Within 1 km:" << endl;
    for (const NearbyRestaurant& nearby: find_nearby_restaurants(40.726f, -73.982f, 1.f)) {
        cout << '\t' << nearby.name << " (" << nearby.cuisine << ") - " << nearby.distance_km << " km away" << endl;
    }

    cout << "All Italian Restaurants" << endl;
    for (auto [name, minLat, minLon]: storage.iterate(select(columns(restaurant->*&Restaurant::name,
                                                                     restaurant->*&Restaurant::minLat,
                                                                     restaurant->*&Restaurant::minLon),
                                                             where(restaurant->*&Restaurant::cuisine == "Italian")))) {
        cout << '\t' << name << " - Location : (" << minLat << ", " << minLon << endl;
    }
}
#endif

int main() {
#ifdef ENABLE_THIS_EXAMPLE
    try {
        sqlite_office();
        nearby_restaurants();
    } catch (const std::system_error& e) {
        cout << "[" << e.code() << "] " << e.what();
    }
#endif
}
