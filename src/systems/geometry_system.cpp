#include "systems/geometry_system.hpp"

namespace ENGINE_NAMESPACE {

#define GEOMETRY_SYS_LOG "GeometrySystem :: "

// Constructor & Destructor
GeometrySystem::GeometrySystem(
    Renderer* const renderer, MaterialSystem* const material_system
)
    : _renderer(renderer), _material_system(material_system) {
    Logger::trace(GEOMETRY_SYS_LOG, "Creating geometry system.");

    if (_max_geometry_count == 0)
        Logger::fatal(
            GEOMETRY_SYS_LOG,
            "Const _max_geometry_count must be greater than 0."
        );
    create_default_geometries();

    Logger::trace(GEOMETRY_SYS_LOG, "Geometry system created.");
}
GeometrySystem::~GeometrySystem() {
    // for (auto geometry : _registered_geometries) {
    //     _renderer->destroy_geometry(geometry.second.handle);
    //     delete geometry.second.handle;
    // }
    // _registered_geometries.clear();
    // _renderer->destroy_geometry(_default_geometry);
    // delete _default_geometry;
    // NOTE: Not sure about this part

    Logger::trace(GEOMETRY_SYS_LOG, "Geometry system destroyed.");
}

// ////////////////////////////// //
// GEOMETRY SYSTEM PUBLIC METHODS //
// ////////////////////////////// //

Geometry* GeometrySystem::acquire(const uint32 id) {
    Logger::trace(GEOMETRY_SYS_LOG, "Geometry with id ", id, " requested.");

    auto ref = _registered_geometries.find(id);
    if (ref == _registered_geometries.end()) {
        Logger::error(
            GEOMETRY_SYS_LOG,
            "Invalid geometry requested. Default returned instead."
        );
        return _default_geometry;
    }
    ref->second.reference_count++;

    Logger::trace(GEOMETRY_SYS_LOG, "Geometry with id ", id, " acquired");
    return ref->second.handle;
}

inline uint32 generate_id() { // TODO: TEMP
    static uint32 id = 0;
    return id++;
}
Geometry* GeometrySystem::acquire(const GeometryConfig& config) {
    Logger::trace(
        GEOMETRY_SYS_LOG, "Geometry \"", config.name, "\" requested."
    );

    // Crete new geometry
    const auto geometry = new (MemoryTag::Resource) Geometry(config.name);

    // Create on GPU
    auto config2d = dynamic_cast<const GeometryConfig2D*>(&config);
    auto config3d = dynamic_cast<const GeometryConfig3D*>(&config);

    if (config2d)
        _renderer->create_geometry(
            geometry, config2d->vertices, config.indices
        );
    else if (config3d)
        _renderer->create_geometry(
            geometry, config3d->vertices, config.indices
        );
    else Logger::fatal(GEOMETRY_SYS_LOG, "Geometry config couldn't be parsed.");

    // Acquire material
    if (config.material_name.length() != 0) {
        geometry->material = _material_system->acquire(config.material_name);
        if (!geometry->material)
            geometry->material = _material_system->default_material();
    } else geometry->material = _material_system->default_material();

    // Generate unique id
    auto id      = generate_id();
    geometry->id = id;

    // Register new slot
    auto& ref           = _registered_geometries[id];
    ref.handle          = geometry;
    ref.auto_release    = config.auto_release;
    ref.reference_count = 1;

    Logger::trace(GEOMETRY_SYS_LOG, "Geometry \"", config.name, "\" acquired.");
    return geometry;
}

void GeometrySystem::release(Geometry* geometry) {
    if (!geometry || !geometry->id.has_value()) {
        Logger::warning(
            GEOMETRY_SYS_LOG,
            "Cannot release invalid geometry. Nothing was done."
        );
        return;
    }

    auto& ref = _registered_geometries[geometry->id.value()];
    if (ref.handle->id != geometry->id.value()) {
        Logger::fatal(
            GEOMETRY_SYS_LOG, "Geometry id mismatch. Check registration logic."
        );
        return;
    }

    if (ref.reference_count > 0) ref.reference_count--;

    // Is the geometry still need, if not release it
    auto id = geometry->id.value();
    if (ref.auto_release && ref.reference_count < 1) {
        _material_system->release(ref.handle->material()->name);
        _renderer->destroy_geometry(ref.handle);
        del(ref.handle);
        _registered_geometries.erase(geometry->id.value());
    }

    Logger::trace(GEOMETRY_SYS_LOG, "Geometry with id ", id, " released.");
}

Geometry* GeometrySystem::generate_cube(
    const String name, const String material_name, const bool auto_release
) {
    float32 l = 0.5f;

    // Initialize vertices and indices for a cube
    Vector<Vertex> vertices {
        // Front
        { { -l, l, -l }, { 0.0f, 1.0f, 0.0f }, {}, {}, { 1.0f, 1.0f } },
        { { l, l, l }, { 0.0f, 1.0f, 0.0f }, {}, {}, { 0.0f, 0.0f } },
        { { -l, l, l }, { 0.0f, 1.0f, 0.0f }, {}, {}, { 1.0f, 0.0f } },
        { { l, l, -l }, { 0.0f, 1.0f, 0.0f }, {}, {}, { 0.0f, 1.0f } },
        // Back
        { { l, -l, -l }, { 0.0f, -1.0f, 0.0f }, {}, {}, { 1.0f, 1.0f } },
        { { -l, -l, l }, { 0.0f, -1.0f, 0.0f }, {}, {}, { 0.0f, 0.0f } },
        { { l, -l, l }, { 0.0f, -1.0f, 0.0f }, {}, {}, { 1.0f, 0.0f } },
        { { -l, -l, -l }, { 0.0f, -1.0f, 0.0f }, {}, {}, { 0.0f, 1.0f } },
        // Left
        { { -l, -l, -l }, { -1.0f, 0.0f, 0.0f }, {}, {}, { 1.0f, 1.0f } },
        { { -l, l, l }, { -1.0f, 0.0f, 0.0f }, {}, {}, { 0.0f, 0.0f } },
        { { -l, -l, l }, { -1.0f, 0.0f, 0.0f }, {}, {}, { 1.0f, 0.0f } },
        { { -l, l, -l }, { -1.0f, 0.0f, 0.0f }, {}, {}, { 0.0f, 1.0f } },
        // Right
        { { l, l, -l }, { 1.0f, 0.0f, 0.0f }, {}, {}, { 1.0f, 1.0f } },
        { { l, -l, l }, { 1.0f, 0.0f, 0.0f }, {}, {}, { 0.0f, 0.0f } },
        { { l, l, l }, { 1.0f, 0.0f, 0.0f }, {}, {}, { 1.0f, 0.0f } },
        { { l, -l, -l }, { 1.0f, 0.0f, 0.0f }, {}, {}, { 0.0f, 1.0f } },
        // Bottom
        { { l, l, -l }, { 0.0f, 0.0f, -1.0f }, {}, {}, { 1.0f, 1.0f } },
        { { -l, -l, -l }, { 0.0f, 0.0f, -1.0f }, {}, {}, { 0.0f, 0.0f } },
        { { l, -l, -l }, { 0.0f, 0.0f, -1.0f }, {}, {}, { 1.0f, 0.0f } },
        { { -l, l, -l }, { 0.0f, 0.0f, -1.0f }, {}, {}, { 0.0f, 1.0f } },
        // Top
        { { -l, l, l }, { 0.0f, 0.0f, 1.0f }, {}, {}, { 1.0f, 1.0f } },
        { { l, -l, l }, { 0.0f, 0.0f, 1.0f }, {}, {}, { 0.0f, 0.0f } },
        { { -l, -l, l }, { 0.0f, 0.0f, 1.0f }, {}, {}, { 1.0f, 0.0f } },
        { { l, l, l }, { 0.0f, 0.0f, 1.0f }, {}, {}, { 0.0f, 1.0f } }
    };
    Vector<uint32> indices(36);
    for (uint32 i = 0; i < 6; ++i) {
        uint32 v_offset = i * 4;
        uint32 i_offset = i * 6;

        indices[i_offset + 0] = v_offset + 0;
        indices[i_offset + 1] = v_offset + 2;
        indices[i_offset + 2] = v_offset + 1;
        indices[i_offset + 3] = v_offset + 0;
        indices[i_offset + 4] = v_offset + 1;
        indices[i_offset + 5] = v_offset + 3;
    }

    // Generate tangents
    generate_tangents(vertices, indices);

    // Crete & return geometry
    GeometryConfig3D config {
        name,           vertices,           indices,
        glm::vec3(0.0), glm::vec3(l, l, l), glm::vec3(-l, -l, -l),
        material_name,  auto_release
    };
    return acquire(config);
}

// Utility methods
void GeometrySystem::generate_normals(
    Vector<Vertex3D>& vertices, const Vector<uint32>& indices
) {
    // Zero out normals (jic)
    for (auto& vertex : vertices)
        vertex.normal = glm::vec3(0.0f);

    // Loops trough all triangles, compute cumulative normal vector
    for (uint32 i = 0; i < indices.size(); i += 3) {
        // Triangle vertices
        Vertex& v0 = vertices[indices[i + 0]];
        Vertex& v1 = vertices[indices[i + 1]];
        Vertex& v2 = vertices[indices[i + 2]];

        // Triangle edges
        glm::vec3 edge1 = v1.position - v0.position;
        glm::vec3 edge2 = v2.position - v0.position;

        // Compute normal
        glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));

        // Face normal
        v0.normal += normal;
        v1.normal += normal;
        v2.normal += normal;
    }

    // Normalize normal vector
    for (auto& vertex : vertices)
        vertex.normal = glm::normalize(vertex.normal);
}
void GeometrySystem::generate_tangents(
    Vector<Vertex3D>& vertices, const Vector<uint32>& indices
) {
    // Loops trough all triangles
    for (uint32 i = 0; i < indices.size(); i += 3) {
        // Triangle vertices
        Vertex& v0 = vertices[indices[i + 0]];
        Vertex& v1 = vertices[indices[i + 1]];
        Vertex& v2 = vertices[indices[i + 2]];

        // Triangle edges
        glm::vec3 edge1 = v1.position - v0.position;
        glm::vec3 edge2 = v2.position - v0.position;

        // UV delta
        float32 delta_U1 = v1.texture_coord.x - v0.texture_coord.x;
        float32 delta_V1 = v1.texture_coord.y - v0.texture_coord.y;
        float32 delta_U2 = v2.texture_coord.x - v0.texture_coord.x;
        float32 delta_V2 = v2.texture_coord.y - v0.texture_coord.y;

        float32 dividend = delta_U1 * delta_V2 - delta_U2 * delta_V1;
        float32 fc       = 1.0f / dividend;

        // Vertex tangent for this triangle
        glm::vec3 tangent = glm::normalize(glm::vec3 {
            fc * (delta_V2 * edge1.x - delta_V1 * edge2.x),
            fc * (delta_V2 * edge1.y - delta_V1 * edge2.y),
            fc * (delta_V2 * edge1.z - delta_V1 * edge2.z) });

        // Used for checking whether the UV coordinates are flipped. This info
        // is stored in tangents w component. (Useful if our model reuses its
        // texture UVs)
        float32 handedness = (dividend > 0.0f) ? -1.0f : 1.0f;
        tangent *= handedness;

        v0.tangent = tangent;
        v1.tangent = tangent;
        v2.tangent = tangent;
    }
}

// /////////////////////////////// //
// GEOMETRY SYSTEM PRIVATE METHODS //
// /////////////////////////////// //

void GeometrySystem::create_default_geometries() {
    float f = 10.0f;

    // === Default for 3D ===
    Vector<Vertex> vertices { { glm::vec3(-0.5f * f, -0.5f * f, 0.0f),
                                glm::vec3(0.0f, 0.0f, 0.0f),
                                glm::vec4(0.0f),
                                glm::vec4(0.0f),
                                glm::vec2(0.0f, 0.0f) },
                              { glm::vec3(0.5f * f, 0.5f * f, 0.0f),
                                glm::vec3(0.0f, 0.0f, 0.0f),
                                glm::vec4(0.0f),
                                glm::vec4(0.0f),
                                glm::vec2(1.0f, 1.0f) },
                              { glm::vec3(-0.5f * f, 0.5f * f, 0.0f),
                                glm::vec3(0.0f, 0.0f, 0.0f),
                                glm::vec4(0.0f),
                                glm::vec4(0.0f),
                                glm::vec2(0.0f, 1.0f) },
                              { glm::vec3(0.5f * f, -0.5f * f, 0.0f),
                                glm::vec3(0.0f, 0.0f, 0.0f),
                                glm::vec4(0.0f),
                                glm::vec4(0.0f),
                                glm::vec2(1.0f, 0.0f) } };
    Vector<uint32> indices { 0, 1, 2, 0, 3, 1 };

    // Crete geometry
    _default_geometry =
        new (MemoryTag::Resource) Geometry(_default_geometry_name);
    _renderer->create_geometry(_default_geometry, vertices, indices);
    _default_geometry->material = _material_system->default_material();

    // === Default for 2D ===
    Vector<Vertex2D> vertices2d {
        { glm::vec2(-0.5f * f, -0.5f * f), glm::vec2(0.0f, 0.0f) },
        { glm::vec2(0.5f * f, 0.5f * f), glm::vec2(1.0f, 1.0f) },
        { glm::vec2(-0.5f * f, 0.5f * f), glm::vec2(0.0f, 1.0f) },
        { glm::vec2(0.5f * f, -0.5f * f), glm::vec2(1.0f, 0.0f) }
    };
    // Note: counter clock-wise
    Vector<uint32> indices2d { 2, 1, 0, 3, 0, 1 };

    // Create 2D geometry
    _default_2d_geometry =
        new (MemoryTag::Resource) Geometry(_default_geometry_name + "2d");
    _renderer->create_geometry(_default_2d_geometry, vertices2d, indices2d);
    _default_2d_geometry->material = _material_system->default_material();
}

} // namespace ENGINE_NAMESPACE