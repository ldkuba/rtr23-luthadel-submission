#include "pipeline.hpp"

#include <fstream>

std::vector<byte> Pipeline::read_file(const std::string& filepath) {
    std::ifstream file{ filepath, std::ios::ate | std::ios::binary };

    if (!file.is_open()) throw std::runtime_error("Failed to open file: " + filepath);

    size_t file_size = static_cast<size_t>(file.tellg());
    std::vector<byte> buffer(file_size);

    file.seekg(0);
    file.read(buffer.data(), file_size);

    file.close();

    return buffer;
}

#include <iostream>

void Pipeline::create_graphics_pipeline(const std::string& vertex_shader_path, const std::string& fragment_shader_path) {
    try {
        auto vertex_code = read_file(vertex_shader_path);
        auto fragment_code = read_file(fragment_shader_path);
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
    }

}

Pipeline::Pipeline(const std::string& vertex_shader_path, const std::string& fragment_shader_path) {
    create_graphics_pipeline(vertex_shader_path, fragment_shader_path);
}

Pipeline::~Pipeline() {}