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
#include "SatelliteType.h"
#include "Reader.h"
#include "Writer.h"
#include "TransmissionResult.h"
#include "Solver.h"
#include "TheoreticalMaxSolver.h"
#include "GreedyQuantizedTimeSolver.h"
#include "GreedyEventBasedSolver.h"

int main() {
    auto start_time = std::chrono::steady_clock::now();

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

    // Runs theoretical maximum calculator.
    TheoreticalMaxSolver max_solver;
    TransmissionResult max_result = max_solver.GetTransmissionSchedule(facility_visibility, 
        satellite_visibility, satellite_types, {});

    // Runs main greedy solution algorithm.
    /*
    GreedyQuantizedTimeSolver greedy_quantized_time_solver;
    TransmissionResult greedy_result = greedy_quantized_time_solver.GetTransmissionSchedule(
        facility_visibility, satellite_visibility, satellite_types, {});
    cerr << "Theoretical maximum: " << max_result.total_data << "\n";
    cerr << "Achieved maximum: " << greedy_result.total_data << "\n";
    */

    auto solution_start_time = std::chrono::steady_clock::now();

    GreedyEventBasedSolver greedy_event_based_solver;
    TransmissionResult greedy_result = greedy_event_based_solver.GetTransmissionSchedule(
        facility_visibility, satellite_visibility, satellite_types, {});
    std::cout << "Theoretical maximum: " << max_result.total_data / 1000 << "." 
        << ToStringWithLength(max_result.total_data % 1000, 3) << " MiB\n";
    std::cout << "Achieved maximum: " << greedy_result.total_data / 1000 << "." 
        << ToStringWithLength(greedy_result.total_data % 1000, 3) << " MiB\n";
    std::cerr << "Solution execution time: " << since(solution_start_time).count() << "ms" << std::endl;


    // TODO: Add verifier

    // Optional part that allows to iteratively improve previously achieved results.
    // Note that improvement is very minor but could significantly increase the calculation time.
    // Use with caution. 
    
    /*
    int iterations = (int) greedy_result.actions.size();
    const int BATCHES = 300;
    int batch_size = iterations / BATCHES;
    for (int i = 0; i < BATCHES; i++) {
        TransmissionResult optimized_result = greedy_event_based_solver.GetTransmissionSchedule(facility_visibility, 
            satellite_visibility, satellite_types, greedy_result.actions, i * batch_size + rand() % batch_size);
        if (optimized_result.total_data > greedy_result.total_data) {
            greedy_result = optimized_result;
        }
        std::cerr << "Currently achieved maximum (BATCH #" << i + 1 << "/" << BATCHES << "): " << greedy_result.total_data << "\n"; 
    }
    */

    // Writes the calculated schedule to the output file.
    Writer::WriteSchedule(config["schedule_path"], greedy_result.transmission_segments, 
        greedy_result.shooting_segments, facility_names, satellite_names, satellite_types);

    std::cerr << "Total execution time: " << since(start_time).count() << "ms" << std::endl;

    return 0;
}