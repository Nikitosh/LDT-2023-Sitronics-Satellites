#include <bits/stdc++.h>
#include "json.hpp"

namespace fs = std::filesystem;
using json = nlohmann::json;
using namespace std;

namespace {

bool StartsWith(const string& s, const string& prefix) {
    return s.compare(0, prefix.size(), prefix) == 0;
}


static bool IsLeap(int year) {
    if (year % 400 == 0) {
        return true;
    } else if (year % 100 == 0) {
        return false;
    } else if (year % 4 == 0) {
        return true;
    } else {
        return false;
    }
}

vector<int> createPartialYearDays() {
    const int MAX_YEAR = 10000;
    vector<int> result(MAX_YEAR);
    for (int i = 1; i < MAX_YEAR; i++) {
        result[i] = result[i - 1] + 365 + (IsLeap(i) ? 1 : 0);
    }
    return result;
}

}

vector<string> MONTHS = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
vector<int> DAYS = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

struct Date {
    int year;
    int month; // 0-indexing
    int day; // 0-indexing
    int hour;
    int minute;
    int second;
    int millis;

    const static vector<int> PARTIAL_YEAR_DAYS;
    
    static Date Parse(stringstream& stream) {
        Date result;
        string month;
        char c;
        stream >> result.day >> month >> result.year >> result.hour >> c >> result.minute >> c >> result.second >> c >> result.millis;
        result.month = int(find(MONTHS.begin(), MONTHS.end(), month) - MONTHS.begin());
        result.day--;
        return result;
    }

    long long ToTimestamp() {
        long long days = PARTIAL_YEAR_DAYS[year - 1];
        for (int i = 0; i < month; i++) {
            days += DAYS[i];
            if (i == 1 && IsLeap(year)) {
                days++;
            }
        }
        days += day;
        return (((days * 24 + hour) * 60 + minute) * 60 + second) * 1000 + millis;
    }
};

const vector<int> Date::PARTIAL_YEAR_DAYS = createPartialYearDays();

struct Segment {
    Date l;
    Date r;
    Segment(Date _l, Date _r): l(_l), r(_r) {}
};

struct SatelliteType {
    string name;
    int filling_speed;
    int freeing_speed;
    int space;
    SatelliteType(const string& _name, int _filling_speed, int _freeing_speed, int _space): 
        name(_name), filling_speed(_filling_speed), freeing_speed(_freeing_speed), space(_space) {}
};

struct Reader {
    static pair<string, map<string, vector<Segment>>> ReadFile(const string& filename) {
        ifstream file(filename);
        string line;
        bool parse = false;
        string facility;
        string satellite_name;
        map<string, vector<Segment>> result;
        while (getline(file, line)) {
            if (size_t ind = line.find("-To-"); ind != string::npos) {
                facility = line.substr(0, ind);
                satellite_name = line.substr(ind + 4, line.size());
            }
            if (line.find("Start Time (UTCG)") != string::npos) {
                parse = true;
            }
            if (StartsWith(line, "Min Duration")) {
                parse = false;
            }
            if (find_if(line.begin(), line.end(), ::isdigit) == line.end()) {
                continue;
            }
            if (parse) {
                stringstream stream(line);
                int id;
                stream >> id;
                Date l = Date::Parse(stream);
                Date r = Date::Parse(stream);
                result[satellite_name].push_back(Segment(l, r));
            }
        }
        return make_pair(facility, result);
    }

    static map<string, map<string, vector<Segment>>> ReadFacilityVisibility(const string& directory) {
        map<string, map<string, vector<Segment>>> result;
        for (const auto& file : fs::directory_iterator(directory)) {
            if (!StartsWith(file.path().stem(), "Facility")) {
                continue;
            }
            auto [facility, satellites] = ReadFile(file.path());
            result[facility] = satellites;
        }
        return result;
    }

    static map<string, vector<Segment>> ReadSatelliteVisibility(const string& directory) {
        map<string, vector<Segment>> result;
        for (const auto& file : fs::directory_iterator(directory)) {
            if (!StartsWith(file.path().stem(), "AreaTarget")) {
                continue;
            }
            auto [facility, satellites] = ReadFile(file.path());
            assert(facility == "Russia");
            result.merge(satellites);
        }
        return result;
    }

    json ReadConfig(const string& filename) {
        ifstream file(filename);
        return json::parse(file);
    }
};

int main() {
    cin.tie(0);
    ios_base::sync_with_stdio(0);

    Reader reader;
    json config = reader.ReadConfig("config.json");
    vector<SatelliteType> satellites_config;
    for (auto& satellite : config["satellites"]) {
        satellites_config.push_back(SatelliteType(satellite["name"], satellite["filling_speed"],
            satellite["freeing_speed"], satellite["space"]));
    }

    map<string, vector<Segment>> satellite_visibility_map = reader.ReadSatelliteVisibility(config["satellite_path"]);
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
            if (StartsWith(name, satellite_type.name)) {
                satellite_types.push_back(satellite_type);
            }
        }
    }
    
    map<string, map<string, vector<Segment>>> facility_visibility_map = reader.ReadFacilityVisibility(config["facility_path"]);
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

    return 0;
}