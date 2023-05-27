#include <fstream>
#include <iostream>
#include <regex>
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
    long long l;
    long long r;
    Segment(long long _l, long long _r): l(_l), r(_r) {}
    bool Intersects(const Segment& other) const {
        return max(l, other.l) < min(r, other.r);
    }
    long long GetIntersectionLength(const Segment& other) const {
        return max(0ll, min(r, other.r) - max(l, other.l));
    }
};

struct SatelliteType {
    int type;
    string name;
    string name_regex;
    int filling_speed = 0;
    int freeing_speed = 0;
    int space = 0;
    SatelliteType(int _type, const string& _name, const string& _name_regex, 
        int _filling_speed, int _freeing_speed, int _space): 
        type(_type), name(_name), name_regex(_name_regex), 
        filling_speed(_filling_speed), freeing_speed(_freeing_speed), space(_space) {}
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
                long long l = Date::Parse(stream).ToTimestamp();
                long long r = Date::Parse(stream).ToTimestamp();
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
            if (!StartsWith(file.path().stem(), "Russia")) {
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
    static void WriteSchedule(const string& directory, 
        const vector<vector<vector<Segment>>>& transmission_segments, 
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
                            << ToStringWithLength(Date::FromTimestamp(segment.l).ToString(), 24) << "    " 
                            << ToStringWithLength(Date::FromTimestamp(segment.r).ToString(), 24) << "    " 
                            << ToStringWithLength(to_string(duration / 1000) +  "." 
                            + ToStringWithLength(duration % 1000, 3), 14) << "    " 
                            << ToStringWithLength(satellite_names[j], 14) << "    "
                            << ToStringWithLength(to_string(duration * satellite_types[j].freeing_speed / 1000), 13) << "\n";
                    }
                    file << "\n";
                }
            }
    }
};

struct TransmissionResult {
    vector<vector<vector<Segment>>> segments;
    long long total_data;
};

class Solver {
public:
    virtual TransmissionResult GetTransmissionSchedule(const vector<vector<vector<Segment>>>& facility_visibility,
        const vector<vector<Segment>>& satellite_visibility,
        const vector<SatelliteType>& satellite_types) = 0;
};

class TheoreticalMaxSolver : public Solver {
public:
    TransmissionResult GetTransmissionSchedule(const vector<vector<vector<Segment>>>& facility_visibility,
        const vector<vector<Segment>>& satellite_visibility,
        const vector<SatelliteType>& satellite_types) override {
        vector<long long> satellite_data;
        for (int i = 0; i < (int) satellite_visibility.size(); i++) {
            SatelliteType type = satellite_types[i];
            long long filling_time = (type.space * 1000) / type.filling_speed;
            double ratio = type.freeing_speed * 1. / (type.freeing_speed + type.filling_speed);  
            long long total_time = 0;
            for (const auto& segment: satellite_visibility[i]) {
                long long duration = segment.r - segment.l;
                if (duration <= filling_time) {
                    total_time += duration;
                } else {
                    total_time += filling_time + (long long) ((duration - filling_time) * ratio);
                }
            }
            satellite_data.push_back(total_time * type.filling_speed / 1000);
        }
        double total_station_time = 0;
        for (const auto& facility_satellites : facility_visibility) {
            struct Event {
                long long x;
                int type;
                bool operator<(const Event& other) const {
                    if (x != other.x) {
                        return x < other.x;
                    }
                    return type < other.type;
                }
            };
            vector<Event> events;
            for (const auto& satellite : facility_satellites) {
                for (const auto& segment : satellite) {
                    events.push_back(Event{.x = segment.l, .type = 0});
                    events.push_back(Event{.x = segment.r, .type = 1});
                }
            }
            sort(events.begin(), events.end());
            int balance = 0;
            long long last_l = 0;
            for (const auto& event : events) {
                if (event.type == 0) {
                    if (balance == 0) {
                        last_l = event.x;
                    }
                    balance++;
                } else {
                    balance--;
                    if (balance == 0) {
                        total_station_time += event.x - last_l;
                    }
                }
            }
        }

        int satellites = (int) satellite_types.size();
        vector<int> perm(satellites);
        iota(perm.begin(), perm.end(), 0);
        sort(perm.begin(), perm.end(), [&](int i, int j) { 
            return satellite_types[i].freeing_speed > satellite_types[j].freeing_speed; });
        long long total_data = 0;
        for (int i = 0; i < satellites; i++) {
            int ind = perm[i];
            double tranmission_time = (satellite_data[ind] * 1000. / satellite_types[ind].freeing_speed);
            if (tranmission_time <= total_station_time) {
                total_station_time -= tranmission_time;
                total_data += satellite_data[ind];
            } else {
                total_data += total_station_time * satellite_types[ind].freeing_speed / 1000;
                break;
            }
        }

        return TransmissionResult { .total_data = total_data };
    }
};

class GreedySolver : public Solver {
public:
    TransmissionResult GetTransmissionSchedule(const vector<vector<vector<Segment>>>& facility_visibility,
        const vector<vector<Segment>>& satellite_visibility,
        const vector<SatelliteType>& satellite_types) override {
        long long min_timestamp = numeric_limits<long long>::max();
        long long max_timestamp = 0;
        for (const auto& facility_satellites : facility_visibility) {
            for (const auto& satellite : facility_satellites) {
                for (const auto& segment : satellite) {
                    min_timestamp = min(min_timestamp, segment.l);
                    max_timestamp = max(max_timestamp, segment.r);
                }
            }
        }
        for (const auto& satellite : satellite_visibility) {
            for (const auto& segment : satellite) {
                min_timestamp = min(min_timestamp, segment.l);
                max_timestamp = max(max_timestamp, segment.r);
            }
        }
        const long long FRAGMENT_LENGTH = 1000;
        int facilities = (int) facility_visibility.size();
        int satellites = (int) facility_visibility[0].size();
        vector<vector<int>> facility_iterators(facilities, vector<int>(satellites));
        vector<int> satellite_iterators(satellites);

        auto get_satellite_intersection_length = [&satellite_visibility, &satellite_iterators](int i, const Segment& segment) {
            if (satellite_iterators[i] == (int) satellite_visibility[i].size()) {
                return 0ll;
            }
            return satellite_visibility[i][satellite_iterators[i]].GetIntersectionLength(segment);
        };

        auto get_facility_intersection_length = [&facility_visibility, &facility_iterators](int i, int j, const Segment& segment) {
            if (facility_iterators[i][j] == (int) facility_visibility[i][j].size()) {
                return 0ll;
            }
            return facility_visibility[i][j][facility_iterators[i][j]].GetIntersectionLength(segment);
        };

        vector<long long> space_used(satellites);
        vector<vector<int>> g(satellites);
        long long total_data = 0;
        vector<long long> satellite_data(satellites);
        for (long long t = min_timestamp; t < max_timestamp; t += FRAGMENT_LENGTH) {
            if (t % 10000000 == 0) {
                cerr << "Progress: " << round(100 * (t - min_timestamp) * 1. / (max_timestamp - min_timestamp)) << "%\n";
            }
            vector<double> cost(satellites);
            for (int i = 0; i < satellites; i++) {
                g[i].clear();
                cost[i] = GetCost(space_used[i], satellite_types[i]);
            }
            vector<int> perm(satellites);
            iota(perm.begin(), perm.end(), 0);
            sort(perm.begin(), perm.end(), [&](int i, int j) { return cost[i] > cost[j]; });
            vector<int> paired(satellites + facilities, -1);
            vector<int> used(satellites + facilities);
            Segment current(t, min(max_timestamp, t + FRAGMENT_LENGTH));
            for (int i = 0; i < satellites; i++) {
                const auto& segments = satellite_visibility[i];
                while (satellite_iterators[i] < (int) segments.size() 
                        && segments[satellite_iterators[i]].r <= t) {
                    satellite_iterators[i]++;
                }
            }
            for (int i = 0; i < facilities; i++) {
                for (int j = 0; j < satellites; j++) {
                    const auto& segments = facility_visibility[i][j];
                    while (facility_iterators[i][j] < (int) segments.size() 
                        && segments[facility_iterators[i][j]].r <= t) {
                        facility_iterators[i][j]++;
                    }
                    if (facility_iterators[i][j] < (int) segments.size() 
                        && segments[facility_iterators[i][j]].Intersects(current)) {
                        if (get_satellite_intersection_length(i, current) == 0
                            || 1.26 * space_used[j] >= satellite_types[j].space) {
                            g[j].push_back(satellites + i);
                        }
                    }
                }
            }
            for (bool run = true; run;) { 
                run = false;
                fill(used.begin(), used.end(), 0);
                for (int i = 0; i < satellites; i++) {
                    int v = perm[i];
                    if (!used[v] && paired[v] == -1 && RunDfs(v, g, used, paired)) {
                        run = true;
                    }
                }
            }
            for (int i = 0; i < satellites; i++) {
                if (paired[i] != -1) {
                    int f = paired[i] - satellites;
                    int freed_space = min(space_used[i], satellite_types[i].freeing_speed * 
                        get_facility_intersection_length(f, i, current) / 1000);
                    space_used[i] -= freed_space;
                    satellite_data[i] += freed_space;
                    total_data += freed_space;
                } else {
                    int filled_space = min(satellite_types[i].space - space_used[i], 
                        satellite_types[i].filling_speed * 
                        get_satellite_intersection_length(i, current) / 1000);
                    space_used[i] += filled_space;
                }
            }
        }
        return TransmissionResult { .segments = {}, .total_data = total_data };
    }

private:
    bool RunDfs(int v, const vector<vector<int>>& g, vector<int>& used, vector<int>& paired) {
        if (used[v]) {
            return false;
        }
        used[v] = 1;
        for (int to : g[v]) {
            if (paired[to] == -1 || RunDfs(paired[to], g, used, paired)) {
                paired[to] = v, paired[v] = to;
                return true;
            }
        }
        return false;
    }

    double GetCost(int space_used, const SatelliteType& satellite) {
        // TODO: Add accounting for the case when satellite stopped shooting and we don't care about releasing its space right now.
        return -(satellite.space - space_used) * 1. / satellite.filling_speed;// * (1 << 20) / satellite.freeing_speed;
    }
};

int main() {
    cin.tie(0);
    ios_base::sync_with_stdio(0);

    json config = Reader::ReadConfig("config.json");
    vector<SatelliteType> satellites_config;
    for (auto& satellite : config["satellites"]) {
        satellites_config.push_back(SatelliteType((int) satellites_config.size(), satellite["name"], satellite["name_regex"], satellite["filling_speed"],
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
            if (regex_match(name, regex(satellite_type.name_regex))) {
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

    TheoreticalMaxSolver solver;
    TransmissionResult result = solver.GetTransmissionSchedule(facility_visibility, satellite_visibility, satellite_types);
    cout << "Theoretical maximum: " << result.total_data << "\n";

    GreedySolver greedy_solver;
    result = greedy_solver.GetTransmissionSchedule(facility_visibility, satellite_visibility, satellite_types);
    cout << "Achieved maximum: " << result.total_data << "\n";
    Writer::WriteSchedule(config["schedule_path"], facility_visibility, facility_names, satellite_names, satellite_types);

    // TODO: Add visibiltiy segments to answer
    // TODO: Add dumping photoshooting schedule 

    return 0;
}