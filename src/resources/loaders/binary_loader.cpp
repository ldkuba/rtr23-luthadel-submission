#include "resources/loaders/binary_loader.hpp"

#include "systems/resource_system.hpp"
#include "resources/datapack.hpp"
#include "file_system.hpp"

// Constructor & Destructor
BinaryLoader::BinaryLoader() {
    _type      = ResourceType::Binary;
    _type_path = "";
}
BinaryLoader::~BinaryLoader() {}

// //////////////////////////// //
// BINARY LOADER PUBLIC METHODS //
// //////////////////////////// //

Resource* BinaryLoader::load(const String name) {
    // Construct full path
    String file_path =
        ResourceSystem::base_path + "/" + _type_path + "/" + name;

    // Read all data
    std::vector<byte> data;
    try {
        data = FileSystem::read_file_bytes(file_path);
    } catch (const FileSystemException& e) {
        Logger::error(RESOURCE_LOG, e.what());
        throw std::runtime_error(e.what());
    }

    // Return byte data
    auto byte_data         = new ByteArrayData(name, data);
    byte_data->full_path   = file_path;
    byte_data->loader_type = ResourceType::Binary;

    return byte_data;
}

void BinaryLoader::unload(Resource* resource) {
    CAN_UNLOAD(Binary, resource);

    ByteArrayData* data = (ByteArrayData*) (resource);
    delete data;
}
