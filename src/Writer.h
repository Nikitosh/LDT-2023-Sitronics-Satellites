
// Helper struct used to output the final schedule.
struct Writer {
    // Writes the given schedule into the output file in the following format.
    // Anadyr1-To-KinoSat_110101
    // -------------------------
    // Access        Start Time (UTCG)           Stop Time (UTCG)        Duration (sec)    Satellite name    Data (Mbytes)
    // ------    ------------------------    ------------------------    --------------    --------------    -------------
    //      1     1 Jun 2027 11:24:03.000     1 Jun 2027 11:24:14.000            11.000    KinoSat_110101            11264
    // ......
    // 
    // "Russia-To-Satellite.txt" file contains photoshooting segments.
    static void WriteSchedule(const string& directory, 
        const vector<vector<vector<Segment>>>& transmission_segments, 
        const vector<vector<Segment>>& shooting_segments, 
        const vector<string>& facility_names, 
        const vector<string>& satellite_names, 
        const vector<SatelliteType> satellite_types) {
        fs::create_directory(directory);
        for (int i = 0; i < (int) transmission_segments.size(); i++) {
            ofstream file(directory + "Facility-" + facility_names[i] + ".txt");
            for (int j = 0; j < (int) transmission_segments[i].size(); j++) {
                file << facility_names[i] << "-To-" << satellite_names[j] << "\n";
                file << string(facility_names[i].size() + satellite_names[j].size() + 4, '-') << "\n";
                file << "Access        Start Time (UTCG)           Stop Time (UTCG)        Duration (sec)    Satellite name    Data (Mbytes)\n";
                file << "------    ------------------------    ------------------------    --------------    --------------    -------------\n";
                for (int g = 0; g < (int) transmission_segments[i][j].size(); g++) {
                    const auto& segment = transmission_segments[i][j][g];
                    long long duration = segment.r - segment.l;
                    file << ToStringWithLength(to_string(g + 1), 6) << "    " 
                        << ToStringWithLength(Time::FromTimestamp(segment.l).ToString(), 24) << "    " 
                        << ToStringWithLength(Time::FromTimestamp(segment.r).ToString(), 24) << "    " 
                        << ToStringWithLength(to_string(duration / 1000) +  "." 
                        + ToStringWithLength(duration % 1000, 3), 14) << "    " 
                        << ToStringWithLength(satellite_names[j], 14) << "    "
                        << ToStringWithLength(to_string(duration 
                        * satellite_types[j].freeing_speed / 1000), 13) << "\n";
                }
                file << "\n";
            }
        }

        ofstream file(directory + "Russia-To-Satellite.txt");
        for (int i = 0; i < (int) shooting_segments.size(); i++) {
            file << "Russia-To-" << satellite_names[i] << "\n";
            file << string(satellite_names[i].size() + 10, '-') << "\n";
            file << "Access        Start Time (UTCG)           Stop Time (UTCG)        Duration (sec)\n";
            file << "------    ------------------------    ------------------------    --------------\n";
            for (int j = 0; j < (int) shooting_segments[i].size(); j++) {
                const auto& segment = shooting_segments[i][j];
                long long duration = segment.r - segment.l;
                file << ToStringWithLength(to_string(j + 1), 6) << "    " 
                    << ToStringWithLength(Time::FromTimestamp(segment.l).ToString(), 24) << "    " 
                    << ToStringWithLength(Time::FromTimestamp(segment.r).ToString(), 24) << "    " 
                    << ToStringWithLength(to_string(duration / 1000) +  "." 
                    + ToStringWithLength(duration % 1000, 3), 14)<< "\n";
            }
            file << "\n";
        }
    }
};
