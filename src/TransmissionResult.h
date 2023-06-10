// Class containing the final schedule produced by an algorithm.
struct TransmissionResult {
    // Total amount of transmitted data.
    // Is stored in 0.001 MiBs.
    long long total_data = 0;
    // Intervals representing when the data was transmitted.
    // `transmission_segments[i][j]` represents all intervals when data was 
    // transmitted from satellite `j` to station `i`.
    std::vector<std::vector<std::vector<Segment>>> transmission_segments;
    // Intervals representing when the photoshotting was active.
    // `shooting_segments[i]` represents all intervals of photoshooting of satellite `i`.
    std::vector<std::vector<Segment>> shooting_segments;
    // Actions taken by different satellites in each iteration.
    // `actions[i][j] == facility` if satellite `j` was transmitting data to station `facility`
    // during iteration `i`. `actions[i][j] == -1` otherwise.
    std::vector<std::vector<int>> actions;

    TransmissionResult(int facilities, int satellites): 
        transmission_segments(facilities, std::vector<std::vector<Segment>>(satellites)), 
        shooting_segments(satellites) {}
};
