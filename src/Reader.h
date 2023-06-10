// Helper struct used to parse the input data.
struct Reader {
    // Parses the given file in the following format:
    // Anadyr1-To-KinoSat_110101
    // -------------------------
    //              Access        Start Time (UTCG)           Stop Time (UTCG)        Duration (sec)
    //              ------    ------------------------    ------------------------    --------------
    //                   1     1 Jun 2027 00:00:01.000     1 Jun 2027 00:04:21.296           260.296
    //              ......
    static std::pair<std::string, std::map<std::string, std::vector<Segment>>> ReadFile(
        const std::string& filename) {
        std::ifstream file(filename);
        std::string line;
        bool parse = false;
        std::string facility;
        std::string satellite_name;
        std::map<std::string, std::vector<Segment>> result;
        while (getline(file, line)) {
            if (size_t ind = line.find("-To-"); ind != std::string::npos) {
                facility = line.substr(0, ind);
                satellite_name = line.substr(ind + 4, line.size());
            }
            if (line.find("Start Time (UTCG)") != std::string::npos) {
                parse = true;
            }
            if (StartsWith(line, "Min Duration")) {
                parse = false;
            }
            if (find_if(line.begin(), line.end(), ::isdigit) == line.end()) {
                continue;
            }
            if (parse) {
                std::stringstream stream(line);
                int id;
                stream >> id;
                long long l = Time::Parse(stream).ToTimestamp();
                long long r = Time::Parse(stream).ToTimestamp();
                result[satellite_name].push_back(Segment(l, r));
            }
        }
        return make_pair(facility, result);
    }

    // Reads all facility-satellite visibility files.
    static std::map<std::string, std::map<std::string, std::vector<Segment>>> ReadFacilityVisibility(
        const std::string& directory) {
        std::map<std::string, std::map<std::string, std::vector<Segment>>> result;
        for (const auto& file : fs::directory_iterator(directory)) {
            if (!StartsWith(file.path().stem(), "Facility")) {
                continue;
            }
            auto [facility, satellites] = ReadFile(file.path());
            result[facility] = satellites;
        }
        return result;
    }

    // Reads all satellite visibility (for photoshooting) files.
    static std::map<std::string, std::vector<Segment>> ReadSatelliteVisibility(const std::string& directory) {
        std::map<std::string, std::vector<Segment>> result;
        for (const auto& file : fs::directory_iterator(directory)) {
            if (!StartsWith(file.path().stem(), "Russia")) {
                continue;
            }
            auto [facility, satellites] = ReadFile(file.path());
            assert(facility == "Russia");
            result.merge(satellites);
        }
        return result;
    }

    // Reads config file.
    static json ReadConfig(const std::string& filename) {
        std::ifstream file(filename);
        return json::parse(file);
    }
};
