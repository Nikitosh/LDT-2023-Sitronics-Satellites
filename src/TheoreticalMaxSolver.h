// Represent a class used to calculate theoretical maximum.
// Uses the following algorithm:
// 1. Calculates how much data could be shot by extreme greedy algorithm: 
//    a) First, fully occupy all the disk space
//    b) Then, free it up and reuse again to shoot more photos.
// 2. Calculates how much time could stations use to transmit the data altogether.
// 3. Greedily distribute shot data between stations giving a priority to 
//    satellites with higher transmission speed. 
//
// Please note that this is one of upper boundaries, not necessarily precise or achievable.
class TheoreticalMaxSolver : public Solver {
public:
    TransmissionResult GetTransmissionSchedule(
        const std::vector<std::vector<std::vector<Segment>>>& facility_visibility,
        const std::vector<std::vector<Segment>>& satellite_visibility,
        const std::vector<SatelliteType>& satellite_types,
        const std::vector<std::vector<int>>& precalculated_actions,
        int selected_iteration = -1) override {
        std::vector<long long> satellite_data;
        // Calculates greedily the maximum shooting time.
        for (int i = 0; i < (int) satellite_visibility.size(); i++) {
            SatelliteType type = satellite_types[i];
            long long filling_time = (type.space * 1000) / type.filling_speed;
            double ratio = double(type.freeing_speed) / double(type.freeing_speed + type.filling_speed);  
            long long total_time = 0;
            for (const auto& segment: satellite_visibility[i]) {
                long long duration = segment.r - segment.l;
                if (duration <= filling_time) {
                    total_time += duration;
                } else {
                    total_time += filling_time + (long long) (double(duration - filling_time) * ratio);
                }
            }
            satellite_data.push_back(total_time * type.filling_speed);
        }

        // Calculates total time that could be used by stations to receive data.
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
            std::vector<Event> events;
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
                        total_station_time += double(event.x - last_l);
                    }
                }
            }
        }

        // Calculates assign satellites to transmit the data.
        int facilities = (int) facility_visibility.size();
        int satellites = (int) satellite_types.size();
        std::vector<int> perm(satellites);
        iota(perm.begin(), perm.end(), 0);
        sort(perm.begin(), perm.end(), [&](int i, int j) { 
            return satellite_types[i].freeing_speed > satellite_types[j].freeing_speed; });
        TransmissionResult result(facilities, satellites);
        for (int i = 0; i < satellites; i++) {
            int ind = perm[i];
            double tranmission_time = double(satellite_data[ind]) / double(satellite_types[ind].freeing_speed);
            if (tranmission_time <= total_station_time) {
                total_station_time -= tranmission_time;
                result.total_data += satellite_data[ind];
            } else {
                result.total_data += (long long)(total_station_time 
                    * double(satellite_types[ind].freeing_speed));
                break;
            }
        }

        return result;
    }
};
