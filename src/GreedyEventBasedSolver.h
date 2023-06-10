// The main solver combining different ideas:
// 1. Sorting all events (starts and ends of visibility intervals) and solving 
//    each segment without changing any assingments inside it.
// 2. Weighted Kuhn's algorithm to calculate perfect matching between stations and satellites.
// 3. Greedily assigning remaining satellites to do photoshooting.
// 4. Some other heuristics used across the code.
class GreedyEventBasedSolver : public Solver {
public:
    TransmissionResult GetTransmissionSchedule(
        const std::vector<std::vector<std::vector<Segment>>>& facility_visibility,
        const std::vector<std::vector<Segment>>& satellite_visibility,
        const std::vector<SatelliteType>& satellite_types,
        const std::vector<std::vector<int>>& precalculated_actions,
        int selected_iteration = -1) override {
        struct Event {
            // Timestamp.
            long long x = 0;
            // End timestamp.
            long long end_x = 0;
            // 1 stands for the start of interval, 0 stands for the end of interval.
            int type = 0;
            // Index of facility for (satellite, facility) visibility intervals.
            // -1 otherwise. 
            int facility;
            // Index of satellite for both (satellite, facility) and satellite visibility intervals.
            int satellite;

            bool operator<(const Event& other) const {
                if (x != other.x) {
                    return x < other.x;
                }
                return std::make_tuple(type, facility, satellite) 
                    < std::make_tuple(other.type, other.facility, other.satellite);
            }
        };

        // Collect all the events of visibility segment start / end and sort them.
        std::vector<Event> events;
        int facilities = (int) facility_visibility.size();
        int satellites = (int) facility_visibility[0].size();
        for (int i = 0; i < facilities; i++) {
            for (int j = 0; j < satellites; j++) {
                for (const auto& segment : facility_visibility[i][j]) {
                    events.push_back(Event{.x = segment.l, .end_x = segment.r, .type = 1, .facility = i, .satellite = j});
                    events.push_back(Event{.x = segment.r, .type = 0, .facility = i, .satellite = j});
                }                
            }
        }
        for (int i = 0; i < satellites; i++) {
            for (const auto& segment : satellite_visibility[i]) {
                events.push_back(Event{.x = segment.l, .end_x = segment.r, .type = 1, .facility = -1, .satellite = i});
                events.push_back(Event{.x = segment.r, .type = 0, .facility = -1, .satellite = i});
            }
        }
        sort(events.begin(), events.end());

        // Inserts segments to vector and potentially merges it with the previous one.
        auto insert_segment = [](std::vector<Segment>& segments, const Segment& segment) {
            if (!segments.empty() && segments.back().r == segment.l) {
                segments.back().r = segment.r;
            } else {
                segments.push_back(segment);
            }
        };

        // Currently used disk space per satellite.
        // Is stored in 0.001 MiBs to avoid calculations in floating point numbers.
        std::vector<long long> space_used(satellites);
        // All facilities that are available for given satellite 
        // for data transmission during given iteration.
        std::vector<std::vector<int>> graph(satellites);
        // Stores the end of time period when satellite is visible or 0 otherwise.
        std::vector<long long> satellite_visible(satellites);
        // Stores 1 when satellite is visible from given facility or 0 otherwise.
        std::vector<std::vector<int>> facility_satellite_visible(facilities, std::vector<int>(satellites));
        
        TransmissionResult result(facilities, satellites);
        const double SPACE_USED_RATIO = 0.93;
        long long current_time = events[0].x;
        for (int it = 0; it < (int) events.size();) {
            while (it < (int) events.size() && events[it].x == current_time) {
                if (events[it].facility == -1) {
                    satellite_visible[events[it].satellite] = events[it].type ? events[it].end_x : 0;
                } else {
                    facility_satellite_visible[events[it].facility][events[it].satellite] = events[it].type;
                }
                it++;
            }
            if (it == (int) events.size()) {
                break;
            }
            // Currently considered segment.
            Segment current(current_time, events[it].x);
            if (it % 1000 == 0) {
                std::cerr << "Progress: " << round(it * 100. / (int) events.size()) << "%\n";
            }

            for (int i = 0; i < satellites; i++) {
                graph[i].clear();
            }

            for (int i = 0; i < facilities; i++) {
                for (int j = 0; j < satellites; j++) {
                    if (facility_satellite_visible[i][j]) {
                        // Never try to transmit any data from satellite with small amount of data.
                        if (space_used[j] < satellite_types[j].freeing_speed * 5) {
                            continue;
                        }
                        // Create an edge between satellite and facility only 
                        // if satellite is unable to do photoshooting 
                        // or if it's getting out of space.
                        // `SPACE_USED_RATIO` is assigned to the best value 
                        // determined during tests.
                        if (!satellite_visible[j] 
                            || double(space_used[j]) * 0.001 / double(satellite_types[j].space) >= SPACE_USED_RATIO) {
                            graph[j].push_back(satellites + i);
                        }
                    }
                }
            }

            // Weighted Kuhn's algorithm implementation.
            // All satellites are ordered by their cost in the descending order.
            // The less time is needed to fully occupy satellite's disk space the bigger is cost.
            // The faster satellite transmits its data back to Earth, the bigger is cost.
            // This allows to free up the most critical satellites efficiently.
            std::vector<double> cost(satellites);
            for (int i = 0; i < satellites; i++) {
                cost[i] = GetCost(space_used[i], 
                    satellite_visible[i] == 0 ? 0 : satellite_visible[i] - current_time,
                    satellite_types[i]);
            }
            std::vector<int> perm(satellites);
            iota(perm.begin(), perm.end(), 0);
            sort(perm.begin(), perm.end(), [&](int i, int j) { return cost[i] > cost[j]; });
            
            std::vector<int> paired = RunKuhn(facilities, satellites, graph, perm);

            // Not the entire segment has to have the same assignment.
            // Change assignment once any of ongoing events finishes.
            long long min_duration = current.Length();
            for (int i = 0; i < satellites; i++) {
                if (paired[i] != -1) {
                    long long freed_space = std::min(space_used[i], satellite_types[i].freeing_speed * 
                        current.Length());
                    long long freed_time = freed_space / satellite_types[i].freeing_speed;
                    assert(freed_time != 0);
                    min_duration = std::min(min_duration, freed_time);
                } else {
                    long long filled_space = std::min(satellite_types[i].space * 1000 - space_used[i], 
                        satellite_types[i].filling_speed * current.Length());
                    long long filled_time = filled_space / satellite_types[i].filling_speed;
                    if (filled_time > 0) {
                        min_duration = std::min(min_duration, filled_time);
                    }
                }
            }
            const long long MIN_SEGMENT_LENGTH = 1000;
            min_duration = std::max(min_duration, std::min(current.Length(), MIN_SEGMENT_LENGTH));
            current = Segment(current_time, current_time + min_duration);
            for (int i = 0; i < satellites; i++) {
                if (paired[i] != -1) {
                    // Emulate transmitting data to the station.
                    // We can't transmit more than satellite currently has.
                    // Note that space is calculated in 0.001 MiBs.
                    long long freed_space = std::min(space_used[i], satellite_types[i].freeing_speed * 
                        current.Length());
                    long long freed_time = freed_space / satellite_types[i].freeing_speed;
                    long long real_freed_space = freed_time * satellite_types[i].freeing_speed;
                    assert(freed_time != 0);
                    int f = paired[i] - satellites;
                    insert_segment(result.transmission_segments[f][i], 
                        Segment(current.l, current.l + freed_time));
                    space_used[i] -= real_freed_space;
                    result.total_data += real_freed_space;
                } else if (satellite_visible[i]) {
                    // Emulate doing photoshooting.
                    // We can't exceed the satellite's disk space.
                    long long filled_space = std::min(satellite_types[i].space * 1000 - space_used[i], 
                        satellite_types[i].filling_speed * current.Length());
                    long long filled_time = filled_space / satellite_types[i].filling_speed;
                    if (filled_time > 0) {
                        long long real_filled_space = filled_time * satellite_types[i].filling_speed;
                        space_used[i] += real_filled_space;
                        insert_segment(result.shooting_segments[i], 
                            Segment(current.l, current.l + filled_time));
                    }
                }
            }
            current_time += min_duration;
            result.actions.push_back(paired);
        }
        return result;
    }

private:
    // Runs Kuhn's algorithm on the given `graph` using `perm` order.
    std::vector<int> RunKuhn(int facilities, int satellites, 
        const std::vector<std::vector<int>>& graph, const std::vector<int>& perm) {
        std::vector<int> paired(satellites + facilities, -1);
        std::vector<int> used(satellites + facilities);
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
    bool RunDfs(int v, const std::vector<std::vector<int>>& graph, std::vector<int>& used, std::vector<int>& paired) {
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
        space_used += potential_filling * satellite.filling_speed;
        return double(space_used) / double(satellite.filling_speed) * double(satellite.freeing_speed);
    }
};
