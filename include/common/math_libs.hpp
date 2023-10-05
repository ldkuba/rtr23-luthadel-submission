#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace std {
string to_string(const glm::vec<2, float>& in);
string to_string(const glm::vec<3, float>& in);
string to_string(const glm::vec<4, float>& in);
string to_string(const glm::mat<2, 2, float>& in);
string to_string(const glm::mat<3, 3, float>& in);
string to_string(const glm::mat<4, 4, float>& in);
} // namespace std