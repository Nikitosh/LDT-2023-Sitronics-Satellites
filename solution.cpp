#include <bits/stdc++.h>
#include "json.hpp"

namespace fs = std::filesystem;
using json = nlohmann::json;
using namespace std;

namespace {

bool StartsWith(const string& s, const string& prefix) {
    return s.compare(0, prefix.size(), prefix) == 0;
}

string ToStringWithLength(long long n, int len) {
    vector<int> digits;
    while (n > 0) {
        digits.push_back(int(n % 10));
        n /= 10;
    }
    while ((int) digits.size() < len) {
        digits.push_back(0);
    }
    reverse(digits.begin(), digits.end());
    string result;
    for (int digit : digits) {
        result.push_back(char(digit + '0'));
    }
    return result;
}

string ToStringWithLength(const string& s, int len) {
    return string(len - s.size(), ' ') + s;
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
    int year = 0;
    int month = 0; // 0-indexing
    int day = 0; // 0-indexing
    int hour = 0;
    int minute = 0;
    int second = 0;
    int millis = 0;

    const static vector<int> PARTIAL_YEAR_DAYS;
    
    static Date Parse(stringstream& stream) {
        Date result;
        string month;
        char c;
        stream >> result.day >> month >> result.year >> result.hour >> c 
            >> result.minute >> c >> result.second >> c >> result.millis;
        result.month = int(find(MONTHS.begin(), MONTHS.end(), month) - MONTHS.begin());
        result.day--;
        return result;
    }

    static Date FromTimestamp(long long timestamp) {
        Date result;
        int days = int(timestamp / (24 * 3600 * 1000));
        result.year = int(upper_bound(PARTIAL_YEAR_DAYS.begin(), PARTIAL_YEAR_DAYS.end(), days) 
            - PARTIAL_YEAR_DAYS.begin());
        days -= PARTIAL_YEAR_DAYS[result.year - 1];
        for (int i = 0; i < (int) DAYS.size(); i++) {
            int month_days = DAYS[i];
            if (i == 1 && IsLeap(result.year)) {
                month_days++;
            }
            if (days < month_days) {
                break;
            }
            result.month++;
            days -= month_days;
        }
        result.day = days;
        int rest = int(timestamp % (24 * 3600 * 1000));
        result.millis = rest % 1000;
        rest /= 1000;
        result.second = rest % 60;
        rest /= 60;
        result.minute = rest % 60;
        rest /= 60;
        result.hour = rest;
        return result;
    }

    long long ToTimestamp() const {
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

    string ToString() const {
        return to_string(day + 1) + " " + MONTHS[month] + " " + to_string(year) + " " 
            + ToStringWithLength(hour, 2) + ":" + ToStringWithLength(minute, 2) + ":" 
            + ToStringWithLength(second, 2) + "." + ToStringWithLength(millis, 3); 
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
    int filling_speed = 0;
    int freeing_speed = 0;
    int space = 0;
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

    static json ReadConfig(const string& filename) {
        ifstream file(filename);
        return json::parse(file);
    }
};

struct Writer {
    static void WriteSchedule(const vector<vector<vector<Segment>>>& transmission_segments, 
        const vector<string>& facility_names, const vector<string>& satellite_names, const string& directory) {
            fs::create_directory(directory);
            for (int i = 0; i < (int) transmission_segments.size(); i++) {
                ofstream file(directory + "Facility-" + facility_names[i] + ".txt");
                for (int j = 0; j < (int) transmission_segments[i].size(); j++) {
                    file << facility_names[i] << "-To-" << satellite_names[j] << "\n";
                    file << string(facility_names[i].size() + satellite_names[j].size() + 4, '-') << "\n";
                    file << "Access        Start Time (UTCG)           Stop Time (UTCG)        Duration (sec)\n";
                    file << "------    ------------------------    ------------------------    --------------\n";
                    for (int g = 0; g < (int) transmission_segments[i][j].size(); g++) {
                        const auto& segment = transmission_segments[i][j][g];
                        long long duration = segment.r.ToTimestamp() - segment.l.ToTimestamp();
                        file << ToStringWithLength(to_string(g + 1), 6) << "     " 
                            << segment.l.ToString() << "     " << segment.r.ToString()
                            << "    " << ToStringWithLength(to_string(duration / 1000) + 
                            "." + ToStringWithLength(duration % 1000, 3), 14) << "\n";
                    }
                    file << "\n";
                }
            }
    }
};

int main() {
    cin.tie(0);
    ios_base::sync_with_stdio(0);

    json config = Reader::ReadConfig("config.json");
    vector<SatelliteType> satellites_config;
    for (auto& satellite : config["satellites"]) {
        satellites_config.push_back(SatelliteType(satellite["name"], satellite["filling_speed"],
            satellite["freeing_speed"], satellite["space"]));
    }

    map<string, vector<Segment>> satellite_visibility_map = Reader::ReadSatelliteVisibility(config["satellite_path"]);
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
    
    map<string, map<string, vector<Segment>>> facility_visibility_map = Reader::ReadFacilityVisibility(config["facility_path"]);
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

    // TODO: Use real transmission segments instead of `facility_visibility`.
    Writer::WriteSchedule(facility_visibility, facility_names, satellite_names, config["schedule_path"]);

    return 0;
}