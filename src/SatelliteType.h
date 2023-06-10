// Represents a type of satellite.
struct SatelliteType {
    int type;
    std::string name;
    // Used to determine the type in the input data. 
    std::string name_regex;
    // The speed of filling the disk space (when photoshooting).
    long long filling_speed = 0;
    // The speed of freeing the disk space (when sending data back to stations).
    long long freeing_speed = 0;
    // The total disk space available.
    long long space = 0;

    SatelliteType(int _type, const std::string& _name, const std::string& _name_regex, 
        long long _filling_speed, long long _freeing_speed, long long _space): 
        type(_type), name(_name), name_regex(_name_regex), 
        filling_speed(_filling_speed), freeing_speed(_freeing_speed), space(_space) {}
};
