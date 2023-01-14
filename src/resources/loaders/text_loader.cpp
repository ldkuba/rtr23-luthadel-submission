#include "resources/loaders/text_loader.hpp"

#include "systems/resource_system.hpp"
#include "resources/datapack.hpp"
#include "systems/file_system.hpp"

// Constructor & Destructor
TextLoader::TextLoader() {
    _type      = ResourceType::Text;
    _type_path = "";
}
TextLoader::~TextLoader() {}

// ////////////////////////// //
// TEXT LOADER PUBLIC METHODS //
// ////////////////////////// //

Result<Resource*, RuntimeError> TextLoader::load(const String name) {
    // Construct full path
    String file_path =
        ResourceSystem::base_path + "/" + _type_path + "/" + name;

    // Read all data
    auto raw_data = FileSystem::read_file_bytes(file_path);
    if (raw_data.has_error()) {
        Logger::error(RESOURCE_LOG, raw_data.error().what());
        return Failure(raw_data.error().what());
    }
    String data = String((const char*) raw_data->data());

    // Return text data
    auto text_data         = new (MemoryTag::Resource) TextData(name, data);
    text_data->full_path   = file_path;
    text_data->loader_type = ResourceType::Text;

    return text_data;
}

void TextLoader::unload(Resource* resource) {
    can_unload(ResourceType::Text, resource);

    TextData* data = (TextData*) (resource);
    delete data;
}
