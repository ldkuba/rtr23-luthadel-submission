#include "resources/loaders/text_loader.hpp"

#include "systems/resource_system.hpp"
#include "resources/datapack.hpp"
#include "file_system.hpp"

// Constructor & Destructor
TextLoader::TextLoader() {
    _type      = ResourceType::Text;
    _type_path = "";
}
TextLoader::~TextLoader() {}

// ////////////////////////// //
// TEXT LOADER PUBLIC METHODS //
// ////////////////////////// //

Resource* TextLoader::load(const String name) {
    // Construct full path
    String file_path =
        ResourceSystem::base_path + "/" + _type_path + "/" + name;

    // Read all data
    String data;
    try {
        auto raw_data = FileSystem::read_file_bytes(file_path);
        data          = String((const char*) raw_data.data());
    } catch (const FileSystemException& e) {
        Logger::error(RESOURCE_LOG, e.what());
        throw std::runtime_error(e.what());
    }

    // Return text data
    auto text_data         = new TextData(name, data);
    text_data->full_path   = file_path;
    text_data->loader_type = ResourceType::Text;

    return text_data;
}

void TextLoader::unload(Resource* resource) {
    CAN_UNLOAD(Text, resource);

    TextData* data = (TextData*) (resource);
    delete data;
}
