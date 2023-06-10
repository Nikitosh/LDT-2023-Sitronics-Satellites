#include <algorithm>
#include <chrono>
#include <climits>
#include <cmath>
#include <fstream>
#include <iostream>
#include <regex>
#include <string>
#include <vector>
#include <map>

#include "lib/json.hpp"

namespace fs = std::filesystem;
using json = nlohmann::json;

#include "Utils.h"
#include "Time.h"
#include "Segment.h"
#include "Reader.h"
#include "ResultsReader.h"
#include "SatelliteType.h"

int main() {
    // Reads config.
    json config = Reader::ReadConfig("config.json");
    std::vector<SatelliteType> satellites_config;
    for (auto& satellite : config["satellites"]) {
        satellites_config.push_back(SatelliteType((int) satellites_config.size(), 
            satellite["name"], satellite["name_regex"], satellite["filling_speed"],
            satellite["freeing_speed"], satellite["space"]));
    }

    // Reads and creates all information about satellites.
    // Note that all names are stored separately, 
    // we operate with indexed entities to make all operations faster.
    std::map<std::string, std::vector<Segment>> satellite_visibility_map 
        = Reader::ReadSatelliteVisibility(config["satellite_path"]);
    int satellites = 0;
    std::vector<std::string> satellite_names;
    std::map<std::string, int> satellite_names_map;
    std::vector<std::vector<Segment>> satellite_visibility;
    std::vector<SatelliteType> satellite_types;
    for (const auto& [name, segments] : satellite_visibility_map) {
        satellite_visibility.push_back(segments);
        satellite_names.push_back(name);
        satellite_names_map[name] = satellites++;
        for (const auto& satellite_type : satellites_config) {
            if (std::regex_match(name, std::regex(satellite_type.name_regex))) {
                satellite_types.push_back(satellite_type);
            }
        }
    }
    
    // Reads and creates all information about facilities and facility-satellite visibility segments.
    // Note that all names are stored separately, 
    // we operate with indexed entities to make all operations faster.
    std::map<std::string, std::map<std::string, std::vector<Segment>>> facility_visibility_map 
        = Reader::ReadFacilityVisibility(config["facility_path"]);
    int facilities = 0;
    std::vector<std::string> facility_names;
    std::map<std::string, int> facility_names_map;
    std::vector<std::vector<std::vector<Segment>>> facility_visibility;
    for (const auto& [name, satellites_segments] : facility_visibility_map) {
        std::vector<std::vector<Segment>> segments(satellites);
        for (const auto& [satellite, satellite_segments] : satellites_segments) {
            segments[satellite_names_map[satellite]] = satellite_segments;
        }
        facility_visibility.push_back(segments);
        facility_names.push_back(name);
        facility_names_map[name] = facilities++;
    }

    // Reads all outputted results about transmitted data segments.
    std::map<std::string, std::map<std::string, std::vector<Segment>>> transmission_segments 
        = ResultsReader::ReadDropFiles(std::string(config["schedule_path"]) + "Drop/");
    // Reads all outputted results about photoshotting segments.
    std::map<std::string, std::vector<Segment>> shooting_segments 
        = ResultsReader::ReadCameraFiles(std::string(config["schedule_path"]) + "Camera/");

    // Stores all segments for given entity (facility or satellite).
    // Stores 1 as second element of pair if the segment corresponds to data transmission.
    // Stores 0 if the segment corresponds to photoshooting.
    std::map<std::string, std::vector<std::pair<Segment, int>>> action_segments;
    for (const auto& [facility, satellite_segments] : transmission_segments) {
        for (const auto& [satellite, segments] : satellite_segments) {
            const auto& visibility_segments = facility_visibility_map[facility][satellite];
            for (const auto& segment : segments) {
                action_segments[facility].push_back(std::make_pair(segment, 1));
                action_segments[satellite].push_back(std::make_pair(segment, 1));

                // Check that visibility segments contain outputted segment.
                auto it = lower_bound(visibility_segments.begin(), visibility_segments.end(), Segment(segment.l + 1, segment.l));
                assert(it != visibility_segments.begin());
                it--;
                assert(it->l <= segment.l && segment.r <= it->r);
            }
        }
    }
    for (const auto& [satellite, segments] : shooting_segments) {
        const auto& visibility_segments = satellite_visibility_map[satellite];
        for (const auto& segment : segments) {
            action_segments[satellite].push_back(std::make_pair(segment, 0));

            // Check that visibility segments contain outputted segment.
            auto it = lower_bound(visibility_segments.begin(), visibility_segments.end(), Segment(segment.l + 1, segment.l));
            assert(it != visibility_segments.begin());
            it--;
            assert(it->l <= segment.l && segment.r <= it->r);
        }
    }

    std::vector<long long> data(satellite_names.size());
    long long total_data = 0;
    for (const auto& [name, segments] : action_segments) {
        std::vector<std::pair<Segment, int>> sorted_segments = segments;
        sort(sorted_segments.begin(), sorted_segments.end());
        for (int i = 1; i < (int) sorted_segments.size(); i++) {
            // Check that no two segments with some action (data transmission or photoshooting) 
            // intersect with each other.
            if (sorted_segments[i].first.Intersects(sorted_segments[i + 1].first)) {
                std::cerr << name << " " << sorted_segments[i].first.l << " " << sorted_segments[i].first.r 
                << " " << sorted_segments[i + 1].first.l << sorted_segments[i + 1].first.r << "\n";
            }
            assert(!sorted_segments[i].first.Intersects(sorted_segments[i + 1].first));
        }
        if (!satellite_names_map.count(name)) {
            continue;
        }
        int ind = satellite_names_map[name];
        for (auto& [segment, transmit] : sorted_segments) {
            if (!transmit) {
                data[ind] += segment.Length() * satellite_types[ind].filling_speed;
                // Check that we never exceed disk space.
                assert(data[ind] <= satellite_types[ind].space * 1000);
            } else {
                long long current_data = segment.Length() * satellite_types[ind].freeing_speed;
                data[ind] -= current_data;
                total_data += current_data;
                // Check that we never try to transmit more than we have.
                assert(data[ind] >= 0);
            }
        }
    }
    std::cout << "Total transmitted data: " << total_data / 1000 << "." 
        << ToStringWithLength(total_data % 1000, 3) << " MiB\n";

    return 0;
}