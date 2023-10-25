#include "math_libs.hpp"

namespace std {
string to_string(const glm::vec<2, float>& in) {
    return "(" + to_string(in.x) + ", " + to_string(in.y) + ")";
}
string to_string(const glm::vec<3, float>& in) {
    return "(" + to_string(in.x) + ", " + to_string(in.y) + ", " +
           to_string(in.z) + ")";
}
string to_string(const glm::vec<4, float>& in) {
    return "(" + to_string(in.x) + ", " + to_string(in.y) + ", " +
           to_string(in.z) + ", " + to_string(in.w) + ")";
}
string to_string(const glm::mat<2, 2, float>& in) {
    return "[" + to_string(in[0]) + ", " + to_string(in[1]) + "]";
}
string to_string(const glm::mat<3, 3, float>& in) {
    return "[" + to_string(in[0]) + ", " + to_string(in[1]) + ", " +
           to_string(in[2]) + "]";
}
string to_string(const glm::mat<4, 4, float>& in) {
    return "[" + to_string(in[0]) + ", " + to_string(in[1]) + ", " +
           to_string(in[2]) + ", " + to_string(in[3]) + "]";
}
} // namespace std