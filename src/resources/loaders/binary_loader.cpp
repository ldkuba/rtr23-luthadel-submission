#include "resources/loaders/binary_loader.hpp"

#include "systems/resource_system.hpp"
#include "resources/datapack.hpp"
#include "systems/file_system.hpp"

// Constructor & Destructor
BinaryLoader::BinaryLoader() {
    _type      = ResourceType::Binary;
    _type_path = "";
}
BinaryLoader::~BinaryLoader() {}

// //////////////////////////// //
// BINARY LOADER PUBLIC METHODS //
// //////////////////////////// //

Result<Resource*, RuntimeError> BinaryLoader::load(const String name) {
    // Construct full path
    String file_path = ResourceSystem::base_path + "/" + name;

    // Read all data
    auto data = FileSystem::read_file_bytes(file_path);
    if (data.has_error()) {
        Logger::error(RESOURCE_LOG, data.error().what());
        return Failure(data.error().what());
    }

    // Return byte data
    auto byte_data =
        new (MemoryTag::Resource) ByteArrayData(name, data.value());
    byte_data->full_path   = file_path;
    byte_data->loader_type = ResourceType::Binary;

    return byte_data;
}

void BinaryLoader::unload(Resource* resource) {
    can_unload(ResourceType::Binary, resource);

    ByteArrayData* data = (ByteArrayData*) (resource);
    delete data;
}
