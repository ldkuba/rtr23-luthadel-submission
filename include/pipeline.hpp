#pragma once

#include "defines.hpp"

#include <string>
#include <vector>

class Pipeline {
private:
    static std::vector<byte> read_file(const std::string& filepath);

public:
    Pipeline(const std::string& vertex_shader_path, const std::string& fragment_shader_path);
    ~Pipeline();

    void create_graphics_pipeline(const std::string& vertex_shader_path, const std::string& fragment_shader_path);
};