// Virtual class representing solving algorithm.
// Could be incremental by using actions taken at the previous step.
class Solver {
public:
    // `precalculated_actions` could be empty if this is an initial step of the algorithm
    // or represent actions taken at the previous step of the algorithm.
    // If latter, `selected_iteration` is used 
    // to specify at which iteration mutations are expected.
    virtual TransmissionResult GetTransmissionSchedule(
        const std::vector<std::vector<std::vector<Segment>>>& facility_visibility,
        const std::vector<std::vector<Segment>>& satellite_visibility,
        const std::vector<SatelliteType>& satellite_types,
        const std::vector<std::vector<int>>& precalculated_actions,
        int selected_iteration = -1) = 0;
};
