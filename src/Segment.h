// Helper struct to represent visibility intervals. 
// `l` is inclusive and `r` is exclusive.
struct Segment {
    long long l;
    long long r;

    Segment(): l(0), r(0) {}
    Segment(long long _l, long long _r): l(_l), r(_r) {}
    Segment Intersect(const Segment& other) const {
        return Segment(std::max(l, other.l), std::min(r, other.r));
    }
    bool Intersects(const Segment& other) const {
        return std::max(l, other.l) < std::min(r, other.r);
    }
    long long Length() const {
        return std::max(0ll, r - l);
    }

    bool operator<(const Segment& other) const {
        if (l != other.l) {
            return l < other.l;
        }
        return r < other.r;
    }
};
