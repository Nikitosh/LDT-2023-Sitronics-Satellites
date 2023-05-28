// Class containing the final schedule produced by an algorithm.
struct TransmissionResult {
    // Total amount of transmitted data.
    long long total_data = 0;
    // Intervals representing when the data was transmitted.
    // `transmission_segments[i][j]` represents all intervals when data was 
    // transmitted from satellite `j` to station `i`.
    vector<vector<vector<Segment>>> transmission_segments;
    // Intervals representing when the photoshotting was active.
    // `shooting_segments[i]` represents all intervals of photoshooting of satellite `i`.
    vector<vector<Segment>> shooting_segments;
    // Actions taken by different satellites in each iteration.
    // `actions[i][j] == facility` if satellite `j` was transmitting data to station `facility`
    // during iteration `i`. `actions[i][j] == -1` otherwise.
    vector<vector<int>> actions;

    TransmissionResult(int facilities, int satellites): 
        transmission_segments(facilities, vector<vector<Segment>>(satellites)), 
        shooting_segments(satellites) {}
};
