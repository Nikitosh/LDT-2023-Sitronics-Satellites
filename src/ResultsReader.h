// Helper struct used to parse the input data.
struct ResultsReader {
    // Parses the given file in the following format:
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
  static std::pair<std::string, std::map<std::string, std::vector<Segment>>> ReadFile(
        const std::string& filename, bool is_facility) {
        std::ifstream file(filename);
        std::string line;
        bool parse = false;
        std::map<std::string, std::vector<Segment>> result;
        std::getline(file, line);
        std::string satellite_name = line;
        while (std::getline(file, line)) {
            if (line.find("Start Time (UTCG)") != std::string::npos) {
                parse = true;
            }
            if (std::find_if(line.begin(), line.end(), ::isdigit) == line.end()) {
                continue;
            }
            if (parse) {
                std::stringstream stream(line);
                int id;
                stream >> id;
                long long l = Time::Parse(stream).ToTimestamp();
                long long r = Time::Parse(stream).ToTimestamp();
                int seconds = 0;
                int millis = 0;
                char dot;
                stream >> seconds >> dot >> millis;
                assert(r - l == 1000 * seconds + millis);
                if (is_facility) {
                    std::string facility;
                    stream >> facility;
                    result[facility].push_back(Segment(l, r));
                } else {
                    result[satellite_name].push_back(Segment(l, r));
                }
            }
        }
        return std::make_pair(satellite_name, result);
    }

    // Reads all transmission-related data files.
    static std::map<std::string, std::map<std::string, std::vector<Segment>>> ReadDropFiles(
        const std::string& directory) {
        std::map<std::string, std::map<std::string, std::vector<Segment>>> result;
        for (const auto& file : fs::directory_iterator(directory)) {
            if (!StartsWith(file.path().stem(), "Drop")) {
                continue;
            }
            auto [satellite, facilities] = ReadFile(file.path(), /*is_facility=*/true);
            for (const auto& facility : facilities) {
                result[facility.first][satellite] = facility.second;
            }
        }
        return result;
    }

    // Reads all photoshooting-related data files.
    static std::map<std::string, std::vector<Segment>> ReadCameraFiles(const std::string& directory) {
        std::map<std::string, std::vector<Segment>> result;
        for (const auto& file : fs::directory_iterator(directory)) {
            if (!StartsWith(file.path().stem(), "Camera")) {
                continue;
            }
            auto [satellite, segments] = ReadFile(file.path(), /*is_facility=*/false);
            result.merge(segments);
        }
        return result;
    }
};
