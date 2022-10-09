#include "systems/geometry_system.hpp"

#define GEOMETRY_SYS_LOG "GeometrySystem :: "

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
    create_default_geometry();

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
    auto ref = _registered_geometries.find(id);
    if (ref == _registered_geometries.end()) {
        Logger::error(
            GEOMETRY_SYS_LOG, "Invalid geometry requested. Return nullptr."
        );
        return nullptr;
    }
    ref->second.reference_count++;
    return ref->second.handle;
}

uint32    generate_id();
Geometry* GeometrySystem::acquire(
    const std::vector<Vertex> vertices,
    const std::vector<uint32> indices,
    const String              name,
    const String              material_name,
    bool                      auto_release
) {
    auto id = generate_id();

    auto& ref           = _registered_geometries[id];
    ref.auto_release    = auto_release;
    ref.reference_count = 1;

    // Crete geometry
    ref.handle     = new Geometry(name);
    ref.handle->id = id;
    _renderer->create_geometry(ref.handle, vertices, indices);

    // Acquire material
    if (material_name != "") {
        ref.handle->material = _material_system->acquire(material_name);
        if (!ref.handle->material)
            ref.handle->material = _material_system->default_material();
    }

    return ref.handle;
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

    if (ref.auto_release && ref.reference_count < 1) {
        _material_system->release(ref.handle->material()->name);
        _renderer->destroy_geometry(ref.handle);
        delete ref.handle;
        _registered_geometries.erase(geometry->id.value());
    }

    Logger::trace(GEOMETRY_SYS_LOG, "Geometry released.");
}

// /////////////////////////////// //
// GEOMETRY SYSTEM PRIVATE METHODS //
// /////////////////////////////// //

void GeometrySystem::create_default_geometry() {}

// //////////////////////////////// //
// GEOMETRY SYSTEM HELPER FUNCTIONS //
// //////////////////////////////// //

uint32 generate_id() {
    static uint32 id = 0;
    return id++;
}