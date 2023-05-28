// Virtual class representing solving algorithm.
// Could be incremental by using actions taken at the previous step.
class Solver {
public:
    // `precalculated_actions` could be empty if this is an initial step of the algorithm
    // or represent actions taken at the previous step of the algorithm.
    // If latter, `selected_iteration` is used 
    // to specify at which iteration mutations are expected.
    virtual TransmissionResult GetTransmissionSchedule(
        const vector<vector<vector<Segment>>>& facility_visibility,
        const vector<vector<Segment>>& satellite_visibility,
        const vector<SatelliteType>& satellite_types,
        const vector<vector<int>>& precalculated_actions,
        int selected_iteration = -1) = 0;
};

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
        const vector<vector<vector<Segment>>>& facility_visibility,
        const vector<vector<Segment>>& satellite_visibility,
        const vector<SatelliteType>& satellite_types,
        const vector<vector<int>>& precalculated_actions,
        int selected_iteration = -1) override {
        vector<long long> satellite_data;
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
            satellite_data.push_back(total_time * type.filling_speed / 1000);
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
                        total_station_time += double(event.x - last_l);
                    }
                }
            }
        }

        // Calculates assign satellites to transmit the data.
        int facilities = (int) facility_visibility.size();
        int satellites = (int) satellite_types.size();
        vector<int> perm(satellites);
        iota(perm.begin(), perm.end(), 0);
        sort(perm.begin(), perm.end(), [&](int i, int j) { 
            return satellite_types[i].freeing_speed > satellite_types[j].freeing_speed; });
        TransmissionResult result(facilities, satellites);
        for (int i = 0; i < satellites; i++) {
            int ind = perm[i];
            double tranmission_time = double(satellite_data[ind])
                * 1000. / double(satellite_types[ind].freeing_speed);
            if (tranmission_time <= total_station_time) {
                total_station_time -= tranmission_time;
                result.total_data += satellite_data[ind];
            } else {
                result.total_data += (long long)(total_station_time 
                    * double(satellite_types[ind].freeing_speed) / 1000);
                break;
            }
        }

        return result;
    }
};

// The main solver combining different ideas:
// 1. Quantization of time segments and solving each segment 
//    without changing any assingments inside it.
// 2. Weighted Kuhn's algorithm to calculate perfect matching between stations and satellites.
// 3. Greedily assigning remaining satellites to do photoshooting.
// 4. Some other heuristics used across the code.
class GreedySolver : public Solver {
public:
    TransmissionResult GetTransmissionSchedule(
        const vector<vector<vector<Segment>>>& facility_visibility,
        const vector<vector<Segment>>& satellite_visibility,
        const vector<SatelliteType>& satellite_types,
        const vector<vector<int>>& precalculated_actions,
        int selected_iteration = -1) override {
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
        int facilities = (int) facility_visibility.size();
        int satellites = (int) facility_visibility[0].size();
        vector<vector<int>> facility_iterators(facilities, vector<int>(satellites));
        vector<int> satellite_iterators(satellites);

        // Returns the intersection of `segment` with 
        // current visibility interval for the given satellite. 
        auto get_satellite_intersection = [&satellite_visibility, &satellite_iterators]
            (int i, const Segment& segment) {
            if (satellite_iterators[i] == (int) satellite_visibility[i].size()) {
                return Segment(0, 0);
            }
            return satellite_visibility[i][satellite_iterators[i]].Intersect(segment);
        };

        // Returns the intersection of `segment` with current visibility interval 
        // for the given facility and satellite. 
        auto get_facility_intersection = [&facility_visibility, &facility_iterators]
            (int i, int j, const Segment& segment) {
            if (facility_iterators[i][j] == (int) facility_visibility[i][j].size()) {
                return Segment(0, 0);
            }
            return facility_visibility[i][j][facility_iterators[i][j]].Intersect(segment);
        };

        // Inserts segments to vector and potentially merges it with the previous one.
        auto insert_segment = [](vector<Segment>& segments, const Segment& segment) {
            if (!segments.empty() && segments.back().r == segment.l) {
                segments.back().r = segment.r;
            } else {
                segments.push_back(segment);
            }
        };

        // Currently used disk space per satellite.
        vector<long long> space_used(satellites);
        // All facilities that are available for given satellite 
        // for data transmission during given iteration.
        vector<vector<int>> graph(satellites);
        // Transmitted data per satellite.
        vector<long long> satellite_data(satellites);
        // Indicates if actions from `precalculated_actions` should be used 
        // or if the new greedy assignment should be recalculated.
        bool recalculate = precalculated_actions.empty();
        // Quantize time by segments of `FRAGMENT_LENGTH` millis. 
        // In each segment we never change any assignments between facilities and satellites.
        const long long FRAGMENT_LENGTH = 1000;
        TransmissionResult result(facilities, satellites);
        const double SPACE_USED_RATIO = 0.997;
        for (long long iteration = 0, t = min_timestamp; 
            t < max_timestamp; iteration++, t += FRAGMENT_LENGTH) {
            if (t % 10000000 == 0) {
                cerr << "Progress: " << round(double(t - min_timestamp) * 100. 
                    / double(max_timestamp - min_timestamp)) << "%\n";
            }

            for (int i = 0; i < satellites; i++) {
                graph[i].clear();
            }

            // Advance iterators for satellite and facility visibility segments.
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
                        // Never try to transmit any data from satellite without any data used.
                        if (space_used[j] == 0) {
                            continue;
                        }
                        // Never try to transmit any data from satellite 
                        // if the transmission segment length is too small.
                        if (get_facility_intersection(i, j, current).Length() 
                            < (long long)(0.5 * FRAGMENT_LENGTH)) {
                            continue;
                        }
                        // Create an edge between satellite and facility only 
                        // if satellite is unable to do photoshooting 
                        // or if it's getting out of space.
                        // `SPACE_USED_RATIO` is assigned to the best value 
                        // determined during tests.
                        if (get_satellite_intersection(j, current).Length() == 0 
                            || double(space_used[j]) * 1. / double(satellite_types[j].space) >= SPACE_USED_RATIO) {
                            graph[j].push_back(satellites + i);
                        }
                    }
                }
            }

            // Weighted Kuhn's algorithm implementation.
            // All satellites are ordered by their cost in the descending order.
            // The less time is needed to fully occupy satellite's disk space the bigger is cost.
            // This allows to free up the most critical satellites.
            vector<double> cost(satellites);
            // How far in future we look to estimate potential data coming from photoshooting.
            const int FUTURE_SEGMENTS = 600;
            for (int i = 0; i < satellites; i++) {
                cost[i] = GetCost(space_used[i], 
                    get_satellite_intersection(i, 
                    Segment(t, t + FRAGMENT_LENGTH * FUTURE_SEGMENTS)).Length(), 
                    satellite_types[i]);
            }
            vector<int> perm(satellites);
            iota(perm.begin(), perm.end(), 0);
            sort(perm.begin(), perm.end(), [&](int i, int j) { return cost[i] > cost[j]; });
            
            vector<int> paired;
            if (recalculate) {
                paired = RunKuhn(facilities, satellites, graph, perm);
            } else {
                // Reuse action from the previous step.
                paired = precalculated_actions[iteration];
                if (iteration == selected_iteration) {
                    bool changed = false;
                    // Try to unassign from transmission all satellites 
                    // that could do a photoshooting instead.
                    for (int i = 0; i < satellites; i++) {
                        if (paired[i] != -1 && space_used[i] < satellite_types[i].space
                            && get_satellite_intersection(i, current).Length() 
                                > (long long)(FRAGMENT_LENGTH * 0.5)) {
                            paired[i] = -1;
                            changed = true;
                            break;
                        }
                    }
                    // If the mutation didn't change anything, there is no need to continue.
                    if (!changed) {
                        return result;
                    }
                    // Use greedy algorithm going forward.
                    recalculate = true;
                }
            }
            // All multiplications and divisions by 1000 are coming from the fact 
            // that we use millis for timestamps and seconds for transmission speeds.
            for (int i = 0; i < satellites; i++) {
                if (paired[i] != -1) {
                    // Emulate transmitting data to the station.
                    int f = paired[i] - satellites;
                    Segment intersection = get_facility_intersection(f, i, current);
                    long long freed_space = min(space_used[i], satellite_types[i].freeing_speed * 
                        intersection.Length() / 1000);
                    long long freed_time = freed_space * 1000 / satellite_types[i].freeing_speed;
                    insert_segment(result.transmission_segments[f][i], 
                        Segment(intersection.l, intersection.l + freed_time));
                    space_used[i] -= freed_space;
                    satellite_data[i] += freed_space;
                    result.total_data += freed_space;
                } else {
                    // Emulate doing photoshooting.
                    Segment intersection = get_satellite_intersection(i, current);
                    long long filled_space = min(satellite_types[i].space - space_used[i], 
                        satellite_types[i].filling_speed * intersection.Length() / 1000);
                    long long filled_time = filled_space * 1000 / satellite_types[i].filling_speed;
                    if (filled_space > 0) {
                        space_used[i] += filled_space;
                        insert_segment(result.shooting_segments[i], 
                            Segment(intersection.l, intersection.l + filled_time));
                    }
                }
            }
            result.actions.push_back(paired);
        }
        return result;
    }

private:
    // Runs Kuhn's algorithm on the given `graph` using `perm` order.
    vector<int> RunKuhn(int facilities, int satellites, 
        const vector<vector<int>>& graph, const vector<int>& perm) {
        vector<int> paired(satellites + facilities, -1);
        vector<int> used(satellites + facilities);
        for (bool run = true; run;) { 
            run = false;
            fill(used.begin(), used.end(), 0);
            for (int i = 0; i < satellites; i++) {
                int v = perm[i];
                if (!used[v] && paired[v] == -1 && RunDfs(v, graph, used, paired)) {
                    run = true;
                }
            }
        }
        paired.resize(satellites);
        return paired;
    }

    // Runs one iteration of helper DFS needed for Kuhn's algorithm.
    // Returns true if chain could be extended.
    bool RunDfs(int v, const vector<vector<int>>& graph, vector<int>& used, vector<int>& paired) {
        if (used[v]) {
            return false;
        }
        used[v] = 1;
        for (int to : graph[v]) {
            if (paired[to] == -1 || RunDfs(paired[to], graph, used, paired)) {
                paired[to] = v, paired[v] = to;
                return true;
            }
        }
        return false;
    }

    // Returns cost for the given satellite.
    // Determines the order of satellites in weighted Kuhn's algorithm.
    double GetCost(long long space_used, long long potential_filling, const SatelliteType& satellite) {
        space_used += potential_filling * satellite.filling_speed / 1000;
        return - double(satellite.space - space_used) / double(satellite.filling_speed);
    }
};
