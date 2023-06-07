#include <climits>
#include <fstream>
#include <iostream>
#include <regex>

#include "lib/json.hpp"

namespace fs = std::filesystem;
using json = nlohmann::json;
// Used to make code more compact, should not be used in production.
using namespace std;

#include "Utils.h"
#include "Time.h"
#include "Segment.h"
#include "SatelliteType.h"
#include "Reader.h"
#include "Writer.h"
#include "TransmissionResult.h"
#include "Solver.h"

int main() {
    cin.tie(0);
    ios_base::sync_with_stdio(0);

    // Reads config.
    json config = Reader::ReadConfig("config.json");
    vector<SatelliteType> satellites_config;
    for (auto& satellite : config["satellites"]) {
        satellites_config.push_back(SatelliteType((int) satellites_config.size(), 
            satellite["name"], satellite["name_regex"], satellite["filling_speed"],
            satellite["freeing_speed"], satellite["space"]));
    }

    // Reads and creates all information about satellites.
    // Note that all names are stored separately, 
    // we operate with indexed entities to make all operations faster.
    map<string, vector<Segment>> satellite_visibility_map 
        = Reader::ReadSatelliteVisibility(config["satellite_path"]);
    int satellites = 0;
    vector<string> satellite_names;
    map<string, int> satellite_names_map;
    vector<vector<Segment>> satellite_visibility;
    vector<SatelliteType> satellite_types;
    for (const auto& [name, segments] : satellite_visibility_map) {
        satellite_visibility.push_back(segments);
        satellite_names.push_back(name);
        satellite_names_map[name] = satellites++;
        for (const auto& satellite_type : satellites_config) {
            if (regex_match(name, regex(satellite_type.name_regex))) {
                satellite_types.push_back(satellite_type);
            }
        }
    }
    
    // Reads and creates all information about facilities and facility-satellite visibility segments.
    // Note that all names are stored separately, 
    // we operate with indexed entities to make all operations faster.
    map<string, map<string, vector<Segment>>> facility_visibility_map 
        = Reader::ReadFacilityVisibility(config["facility_path"]);
    int facilities = 0;
    vector<string> facility_names;
    map<string, int> facility_names_map;
    vector<vector<vector<Segment>>> facility_visibility;
    for (const auto& [name, satellites_segments] : facility_visibility_map) {
        vector<vector<Segment>> segments(satellites);
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
    GreedySolver greedy_solver;
    TransmissionResult greedy_result = greedy_solver.GetTransmissionSchedule(facility_visibility, 
        satellite_visibility, satellite_types, {});
    cerr << "Theoretical maximum: " << max_result.total_data << "\n";
    cerr << "Achieved maximum: " << greedy_result.total_data << "\n";
    
    // Optional part that allows to iteratively improve previously achieved results.
    // Note that improvement is very minor but could significantly increase the calculation time.
    // Use with caution. 
    
    /*
    int iterations = (int) greedy_result.actions.size();
    const int BATCHES = 300;
    int batch_size = iterations / BATCHES;
    for (int i = 0; i < BATCHES; i++) {
        TransmissionResult optimized_result = greedy_solver.GetTransmissionSchedule(facility_visibility, 
            satellite_visibility, satellite_types, greedy_result.actions, i * batch_size + rand() % batch_size);
        if (optimized_result.total_data > greedy_result.total_data) {
            greedy_result = optimized_result;
        }
        cerr << "Currently achieved maximum (BATCH #" << i + 1 << "/" << BATCHES << "): " << greedy_result.total_data << "\n"; 
    }
    */
    
    // Writes the calculated schedule to the output file.
    Writer::WriteSchedule(config["schedule_path"], greedy_result.transmission_segments, 
        greedy_result.shooting_segments, facility_names, satellite_names, satellite_types);

    return 0;
}