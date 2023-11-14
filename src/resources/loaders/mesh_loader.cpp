#include "resources/loaders/mesh_loader.hpp"

#include "systems/resource_system.hpp"
#include "systems/file_system.hpp"
#include "systems/geometry_system.hpp"
#include "renderer/renderer_types.hpp"
#include "serialization/binary_serializer.hpp"

namespace ENGINE_NAMESPACE {

// Helper functions
Result<void, RuntimeError> save_mesh(
    const String& name, const String& path, GeometryConfigArray* const configs
);
Result<GeometryConfigArray*, RuntimeError> load_mesh(
    const String& name, const String& path
);
Result<GeometryConfigArray*, RuntimeError> load_obj(
    const String& name, const String& path
);

// Supported extensions
const std::vector<MeshLoader::MeshFileType>
    MeshLoader::_supported_mesh_file_types { { ".mesh", true, load_mesh },
                                             { ".obj", false, load_obj } };

// Constructor & Destructor
MeshLoader::MeshLoader() {
    _type      = ResourceType::Mesh;
    _type_path = "models";
}
MeshLoader::~MeshLoader() {}

// ////////////////////////// //
// MESH LOADER PUBLIC METHODS //
// ////////////////////////// //

Result<Resource*, RuntimeError> MeshLoader::load(const String name) {
    // Construct file path
    const String file_name = name.lower_c();
    const String file_path =
        ResourceSystem::base_path + "/" + _type_path + "/" + file_name;

    // Check files existence for all supported formats
    for (const auto& supported_mesh_file_type : _supported_mesh_file_types) {
        const auto full_path = file_path + supported_mesh_file_type.extension;
        if (FileSystem::exists(full_path)) {
            // Format found. Load it or fail.
            const auto result = supported_mesh_file_type.load(name, full_path);

            Resource* const configs = check(result);
            configs->full_path      = full_path;
            configs->loader_type    = ResourceType::Mesh;

            return configs;
        }
    }

    // This mesh file doesn't exist
    return Failure(RuntimeError("Mesh file \"" + name + "\" not found."));
}

void MeshLoader::unload(Resource* resource) {
    can_unload(ResourceType::Mesh, resource);

    auto res = (GeometryConfigArray*) resource;
    delete res;
}

// //////////////////////////// //
// MESH LOADER HELPER FUNCTIONS //
// //////////////////////////// //

// -----------------------------------------------------------------------------
// Proprietary
// -----------------------------------------------------------------------------

Result<void, RuntimeError> save_mesh(
    const String&              name,
    const String&              path,
    GeometryConfigArray* const config_array
) {
    // Since this is binary format data needs to be serialized to bytes
    BinarySerializer serializer {};
    String           buffer {};

    // Push header to buffer
    uint64 version = 0x1u;
    buffer += serializer.serialize(
        version, name, (uint32) config_array->configs.size()
    );

    // Push all geometries
    for (const auto config : config_array->configs)
        buffer += config->serialize(&serializer);

    // Create new mesh file
    auto result = FileSystem::create_or_open(path, FileSystem::binary);
    if (result.has_error()) return Failure(result.error().what());
    auto& file = result.value();

    // Write buffer
    file->write(buffer);
    file->close();
    return {};
}

Result<GeometryConfigArray*, RuntimeError> load_mesh(
    const String& name, const String& path
) {
    // Load mesh file
    const auto   bytes_r = FileSystem::read_bytes(path);
    const auto   bytes   = check(bytes_r);
    const String buffer { bytes.data(), bytes.size() };
    uint32       buffer_pos = 0;

    // File is binary, so we will utilize the help of Binary serializer
    BinarySerializer serializer {};

    // Read header info
    uint64 version;
    String mesh_name;
    uint32 geom_count;

    Result<uint32, RuntimeError> read = serializer.deserialize(
        buffer, buffer_pos, version, mesh_name, geom_count
    );
    if (read.has_error()) return Failure(read.error());
    buffer_pos += read.value();

    // Output geometry configuration array
    auto config_array =
        new (MemoryTag::Resource) GeometryConfigArray(mesh_name);

    // Read geometries
    config_array->configs.reserve(geom_count);
    for (uint32 i = 0; i < geom_count; i++) {
        // Read geometry dimension cout
        uint8 dim_count;
        read = serializer.deserialize(buffer, buffer_pos, dim_count);
        if (read.has_error()) {
            delete config_array;
            return Failure(read.error());
        };

        // Read geometry
        switch (dim_count) {
        case 3: {
            auto config = new (MemoryTag::Geometry) Geometry::Config3D();
            read        = config->deserialize(&serializer, buffer, buffer_pos);
            config_array->configs.push_back(config);
        } break;
        default:
            Logger::fatal(
                RESOURCE_LOG,
                "Proprietary mesh loader for a ",
                dim_count,
                "D geometry not supported."
            );
        }

        // Advance buffer
        if (read.has_error()) {
            delete config_array;
            return Failure(read.error());
        }
        buffer_pos += read.value();
    }

    // return results
    return config_array;
}

} // namespace ENGINE_NAMESPACE

// -----------------------------------------------------------------------------
// OBJ
// -----------------------------------------------------------------------------

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include "map.hpp"

namespace ENGINE_NAMESPACE {

// Local helper
String               create_mat_file(const MaterialConfig& config);
Result<uint32, bool> fuzzy_get_index(
    Map<float32, std::pair<Vertex3D, uint32>>& vertex_map, Vertex3D& vertex
);

Result<GeometryConfigArray*, RuntimeError> load_obj(
    const String& name, const String& path
) {
    // Load mesh data
    tinyobj::ObjReader reader;
    if (!reader.ParseFromFile(path))
        return Failure(RuntimeError("TinyObjReader :: " + reader.Error()));

    if (!reader.Warning().empty())
        Logger::warning(RESOURCE_LOG, "TinyObjReader :: ", reader.Warning());

    // Parse mesh
    auto& attributes = reader.GetAttrib();
    auto& shapes     = reader.GetShapes();
    auto& materials  = reader.GetMaterials();

    // Load materials and save them as .mat files
    Vector<String> material_configs;
    for (const auto& material : materials) {
        const auto material_name = create_mat_file(
            { material.name,
              Shader::BuiltIn::MaterialShader, // Note: OBJ doesn't hold
                                               // this info
              material.diffuse_texname,
              material.specular_texname,
              material.bump_texname,
              glm::vec4(
                  material.diffuse[0],
                  material.diffuse[1],
                  material.diffuse[2],
                  1
              ),
              std::max(material.shininess, 8.0f),
              true }
        );
        // Geometries will reference these materials by name
        material_configs.push_back(material_name);
    }

    // Geometry configuration obj
    auto config_array = new (MemoryTag::Resource) GeometryConfigArray(name);
    config_array->configs.reserve(shapes.size());

    // Loop over shapes
    Map<float32, std::pair<Vertex3D, uint32>> unique_vertices {};
    for (const auto& shape : shapes) {
        Vector<Vertex3D> vertices { { MemoryTag::Geometry } };
        Vector<uint32>   indices { { MemoryTag::Geometry } };
        glm::vec3        extent_min { Infinity32, Infinity32, Infinity32 };
        glm::vec3        extent_max { -Infinity32, -Infinity32, -Infinity32 };

        // Load vertices, indices and extent
        unique_vertices.clear();
        for (const auto& index : shape.mesh.indices) {
            Vertex3D vertex {};

            // Load position
            const auto x    = attributes.vertices[3 * index.vertex_index + 0];
            const auto y    = attributes.vertices[3 * index.vertex_index + 1];
            const auto z    = attributes.vertices[3 * index.vertex_index + 2];
            vertex.position = { x, y, z };

            // Compute extent
            if (x < extent_min.x) extent_min.x = x;
            if (y < extent_min.y) extent_min.y = y;
            if (z < extent_min.z) extent_min.z = z;
            if (x > extent_max.x) extent_max.x = x;
            if (y > extent_max.y) extent_max.y = y;
            if (z > extent_max.z) extent_max.z = z;

            // Load normal
            vertex.normal = { attributes.normals[3 * index.normal_index + 0],
                              attributes.normals[3 * index.normal_index + 1],
                              attributes.normals[3 * index.normal_index + 2] };

            // Load texture coordinate
            vertex.texture_coord = {
                attributes.texcoords[2 * index.texcoord_index + 0],
                1.0f - attributes.texcoords[2 * index.texcoord_index + 1]
            };

            // Load colors
            vertex.color = { attributes.colors[3 * index.vertex_index + 0],
                             attributes.colors[3 * index.vertex_index + 1],
                             attributes.colors[3 * index.vertex_index + 2],
                             1 };

            // Was vertex with this index already present?
            const auto new_index_res = fuzzy_get_index(unique_vertices, vertex);
            const auto new_index     = new_index_res.value_or(vertices.size());
            if (new_index_res.has_value() == false) {
                // Push to vertex list
                unique_vertices[vertex.position.x] =
                    std::pair(vertex, new_index);
                vertices.push_back(vertex);
            }

            // Push this index to the list anyways
            indices.push_back(new_index);
        }

        // Compute tangents
        GeometrySystem::generate_tangents(vertices, indices);

        // Compute materials
        // TODO: Support multiple materials per geometry
        // Does that even ever happen?
        const auto val_of_in = shape.mesh.material_ids.size();
        if (val_of_in > 1)
            Logger::trace(
                "[!!!] :: Shape with multiple materials : ", val_of_in
            );

        const auto mat_id = shape.mesh.material_ids[0];
        const auto material_name =
            (mat_id >= 0) ? material_configs[mat_id] : "";

        // Save as new geometry 3D of this object
        Geometry::Config3D* config =
            new (MemoryTag::Geometry) Geometry::Config3D(
                name + "_" + shape.name,
                vertices,
                indices,
                { extent_min, extent_max },
                material_name, // TODO:
                true
            );
        config_array->configs.push_back(config);
    }

    // Save as proprietary format for future faster loading
    // Changes the path extension from .obj to .mesh
    const String mesh_path = path.substr(0, path.size() - 3) + "mesh";
    if (save_mesh(name, mesh_path, config_array).has_error())
        Logger::error(
            RESOURCE_LOG, "Failed to save loaded OBJ file as \".mesh\""
        );

    return config_array;
}

Result<uint32, bool> fuzzy_get_index(
    Map<float32, std::pair<Vertex3D, uint32>>& vertex_map, Vertex3D& vertex
) {
    const auto key  = vertex.position.x;
    const auto from = vertex_map.lower_bound(key - Epsilon32);
    const auto to   = vertex_map.upper_bound(key + Epsilon32);

    auto current = from;
    for (; current != to; current++)
        if (current->second.first == vertex) return current->second.second;

    return Failure(0);
}

// TODO: REMOVE WHEN POSSIBLE
#define MAT_PATH "materials"

#define write_setting(setting) file->write_ln(#setting, "=", config.setting);

String create_mat_file(const MaterialConfig& config) {
    // Material path
    auto full_path = String::build(
        ResourceSystem::base_path, "/", MAT_PATH, "/", config.name, ".mat"
    );

    // Create new material file
    auto result = FileSystem::create_or_open(full_path);
    if (result.has_error()) {
        Logger::error(
            RESOURCE_LOG,
            "Creation of material at \"",
            full_path,
            "\" failed for some reason."
        );
        return "";
    }
    auto& file = result.value();

    // Write data
    file->write_ln("# ", config.name(), " material file");
    file->write_ln("");
    file->write_ln("version=0.1");
    write_setting(name);
    file->write_ln(
        "diffuse_color=",
        config.diffuse_color[0],
        " ",
        config.diffuse_color[1],
        " ",
        config.diffuse_color[2],
        " ",
        config.diffuse_color[3]
    );
    write_setting(shininess);
    write_setting(diffuse_map_name);
    write_setting(specular_map_name);
    write_setting(normal_map_name);
    write_setting(shader);

    // Close
    file->close();

    return config.name;
}

} // namespace ENGINE_NAMESPACE