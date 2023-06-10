// Helper struct used to output the final schedule.
struct Writer {
    // Writes the given schedule into the output file in the following format.
    //
    // Ground_Anadyr1.txt:
    // Anadyr1
    // -------
    // Access *       Start Time (UTCG) *        Stop Time (UTCG) * Duration (sec) * Satellite name * Data (Mbytes)
    //      1   1 Jun 2027 11:24:03.000   1 Jun 2027 11:24:14.005           11.005   KinoSat_110101         1408.64
    // ......
    // 
    // Drop_KinoSat_110101.txt:
    // KinoSat_110101
    // --------------
    // Access *       Start Time (UTCG) *        Stop Time (UTCG) * Duration (sec) * Station name * Data (Mbytes)
    //      1   1 Jun 2027 11:24:03.000   1 Jun 2027 11:24:14.005           11.005        Anadyr1         1408.64
    // ......
    // 
    // Camera_KinoSat_110101.txt:
    // KinoSat_110101
    // --------------
    // Access *       Start Time (UTCG) *        Stop Time (UTCG) * Duration (sec) * Data (Mbytes)
    //      1   1 Jun 2027 11:24:03.000   1 Jun 2027 11:24:14.005           11.005         5634.56
    // ......
    static void WriteSchedule(const std::string& directory, 
        const std::vector<std::vector<std::vector<Segment>>>& transmission_segments, 
        const std::vector<std::vector<Segment>>& shooting_segments, 
        const std::vector<std::string>& facility_names, 
        const std::vector<std::string>& satellite_names, 
        const std::vector<SatelliteType> satellite_types) {
        for (const auto& dir : {directory, directory + "Ground/", directory + "Drop/", directory + "Camera/"}) {
            fs::create_directory(dir);
        }
        long long total_data = 0;
        int total_count = 0;
        for (int i = 0; i < (int) transmission_segments.size(); i++) {
            for (int j = 0; j < (int) transmission_segments[i].size(); j++) {
                total_count += (int) transmission_segments[i][j].size();
            }
        }
        for (int i = 0; i < (int) transmission_segments.size(); i++) {
            std::ofstream file(directory + "Ground/Ground_" + facility_names[i] + ".txt");
            file << facility_names[i] << "\n";
            file << std::string(facility_names[i].size(), '-') << "\n";
            file << " Access *        Start Time (UTCG) *         Stop Time (UTCG) * Duration (sec) * Satellite name * Data (Mbytes)\n";    
            int id = 0;
            for (int j = 0; j < (int) transmission_segments[i].size(); j++) {
                for (int g = 0; g < (int) transmission_segments[i][j].size(); g++) {
                    const auto& segment = transmission_segments[i][j][g];
                    long long duration = segment.r - segment.l;
                    long long data = duration * satellite_types[j].freeing_speed;
                    file << ToStringWithLength(std::to_string(++id), 7) << "   " 
                        << ToStringWithLength(Time::FromTimestamp(segment.l).ToString(), 24) << "   " 
                        << ToStringWithLength(Time::FromTimestamp(segment.r).ToString(), 24) << "   " 
                        << ToStringWithLength(std::to_string(duration / 1000) +  "." 
                        + ToStringWithLength(duration % 1000, 3), 14) << "   " 
                        << ToStringWithLength(satellite_names[j], 14) << "   "
                        << ToStringWithLength(std::to_string(data / 1000) + "."
                        + ToStringWithLength(data % 1000, 3), 13) << "\n";
                    total_data += data;
                }
            }
        }
        for (int j = 0; j < (int) transmission_segments[0].size(); j++) {
            std::ofstream file(directory + "Drop/Drop_" + satellite_names[j] + ".txt");
            file << satellite_names[j] << "\n";
            file << std::string(satellite_names[j].size(), '-') << "\n";
            file << " Access *        Start Time (UTCG) *         Stop Time (UTCG) * Duration (sec) * Station name * Data (Mbytes)\n";    
            int id = 0;
            for (int i = 0; i < (int) transmission_segments.size(); i++) {
                for (int g = 0; g < (int) transmission_segments[i][j].size(); g++) {
                    const auto& segment = transmission_segments[i][j][g];
                    long long duration = segment.r - segment.l;
                    long long data = duration * satellite_types[j].freeing_speed;
                    file << ToStringWithLength(std::to_string(++id), 7) << "   " 
                        << ToStringWithLength(Time::FromTimestamp(segment.l).ToString(), 24) << "   " 
                        << ToStringWithLength(Time::FromTimestamp(segment.r).ToString(), 24) << "   " 
                        << ToStringWithLength(std::to_string(duration / 1000) +  "." 
                        + ToStringWithLength(duration % 1000, 3), 14) << "   " 
                        << ToStringWithLength(facility_names[i], 12) << "   "
                        << ToStringWithLength(std::to_string(data / 1000) + "." 
                        + ToStringWithLength(data % 1000, 3), 13) << "\n";
                }
            }
        }

        for (int i = 0; i < (int) shooting_segments.size(); i++) {
            std::ofstream file(directory + "Camera/Camera_" + satellite_names[i] + ".txt");
            file << satellite_names[i] << "\n";
            file << std::string(satellite_names[i].size(), '-') << "\n";
            file << " Access *        Start Time (UTCG) *         Stop Time (UTCG) * Duration (sec) * Data (Mbytes)\n";
            for (int j = 0; j < (int) shooting_segments[i].size(); j++) {
                const auto& segment = shooting_segments[i][j];
                long long duration = segment.r - segment.l;
                long long data = duration * satellite_types[i].filling_speed;
                file << ToStringWithLength(std::to_string(j + 1), 7) << "   " 
                    << ToStringWithLength(Time::FromTimestamp(segment.l).ToString(), 24) << "   " 
                    << ToStringWithLength(Time::FromTimestamp(segment.r).ToString(), 24) << "   " 
                    << ToStringWithLength(std::to_string(duration / 1000) +  "." 
                    + ToStringWithLength(duration % 1000, 3), 14)  << "   "
                    << ToStringWithLength(std::to_string(data / 1000) + "." 
                    + ToStringWithLength(data % 1000, 3), 13) << "\n";
            }
        }

        std::cout << "Total written data: " << total_data / 1000 << "." 
            << ToStringWithLength(total_data % 1000, 3) << "\n";
    }
};
