#pragma once

#include "texture.hpp"
#include "unordered_map.hpp"

#include "outcome.hpp"

namespace ENGINE_NAMESPACE {

class TextureSystem;

/// @brief Supported shader attribute types
enum class ShaderAttributeType : uint8 {
    float32,
    vec2,
    vec3,
    vec4,
    int8,
    int16,
    int32,
    uint8,
    uint16,
    uint32,
    COUNT
};
/// @brief Supported uniform types
enum class ShaderUniformType : uint8 {
    float32,
    vec2,
    vec3,
    vec4,
    int8,
    int16,
    int32,
    uint8,
    uint16,
    uint32,
    matrix4,
    sampler,
    custom
};

/// @brief Shader stages available
enum class ShaderStage : uint8 {
    Vertex   = 0x1,
    Geometry = 0x2,
    Fragment = 0x4,
    Compute  = 0x8
};

enum class ShaderBindingType : uint8 { Uniform, Sampler, Storage };

enum class ShaderScope : uint8 { Global, Instance, Local };

/// @brief Byte range description
struct ByteRange {
    uint64 offset;
    uint64 size;
};

/// @brief Structure containing all attribute relevant data
struct ShaderAttribute {
    String              name;
    uint32              size;
    ShaderAttributeType type;
};
/// @brief Shader uniform configuration
struct ShaderUniformConfig {
    String            name;
    ShaderUniformType type;
    uint32            size;
    uint32            location;
};
/// @brief Shader binding configuration
struct ShaderBindingConfig {
    uint32                      set_index;
    ShaderScope                 scope;
    ShaderBindingType           type;
    size_t                      count;
    uint8                       shader_stages;
    Vector<ShaderUniformConfig> uniforms;
};

/// @brief Structure containing all uniform relevant data
struct ShaderUniform {
    ByteRange         byte_range;
    ShaderScope       scope;
    uint32            binding; // Binding -1 means it's a push constant
    uint16            location;
    uint16            index;
    ShaderUniformType type;
};

/// @brief Structure containing all binding relevant data
struct ShaderBinding {
    uint32            set_index;
    ByteRange         byte_range;
    uint32            stride;
    ShaderScope       scope;
    ShaderBindingType type;
    size_t            count;
    uint8             shader_stages;
    Vector<size_t>    uniforms;
    bool              was_modified;
};

struct ShaderDescriptorSet {
    Vector<ShaderBinding> bindings;
    ByteRange             byte_range;
    uint64                stride;

    bool is_empty() { return bindings.size() == 0; }
};

/**
 * @brief Shader configuration resource. Usually this resource is loaded from a
 * .shadercfg file.
 */
class ShaderConfig : public Resource {
  public:
    TextureSystem*                    texture_system = nullptr;
    const String                      render_pass_name;
    const uint8                       shader_stages;
    const Vector<ShaderAttribute>     attributes;
    const Vector<ShaderBindingConfig> bindings;
    const Vector<ShaderUniformConfig> push_constants;
    const bool                        use_instances;
    const bool                        use_locals;

    ShaderConfig(
        const String&                      name,
        const String&                      render_pass_name,
        const uint8                        shader_stages,
        const Vector<ShaderAttribute>&     attributes,
        const Vector<ShaderBindingConfig>& bindings,
        const Vector<ShaderUniformConfig>& push_constants,
        const bool                         use_instances,
        const bool                         use_locals
    )
        : Resource(name), render_pass_name(render_pass_name),
          shader_stages(shader_stages), attributes(attributes),
          bindings(bindings), push_constants(push_constants),
          use_instances(use_instances), use_locals(use_locals) {}
    ~ShaderConfig() {}
};

/**
 * @brief An instance-level shader state.
 */
struct InstanceState {
    uint64 offset;
    bool   should_update = true;

    Vector<TextureMap*> instance_texture_maps;
};

/**
 * @brief Frontend (API agnostic) representation of a shader.
 */
class Shader {
  public:
    /**
     * @brief Construct a new Shader object
     *
     * @param config Shader configurations
     */
    Shader(const ShaderConfig config);
    virtual ~Shader();

    // TODO: TEMP
    String get_name() { return _name; }

    /**
     * @brief Reload shader to process changes in shader code.
     * TODO: for now this is simplified. Doesn't consider changes in shader's
     * attributes & uniforms configuration.
     */
    virtual void reload();

    /**
     * @brief Use this shader
     */
    virtual void use();

    /**
     * @brief Binds shader to global scope. Must be done before setting global
     * level uniforms.
     */
    virtual void bind_globals();
    /**
     * @brief Binds specific shader instance for use. Must be done before the
     * setting of instance level uniforms.
     * @param id
     */
    virtual void bind_instance(const uint32 id);

    /**
     * @brief Apply set global uniforms
     */
    virtual void apply_global();
    /**
     * @brief Apply set instance uniforms
     */
    virtual void apply_instance();

    /**
     * @brief Acquires resources required for initialization of a shader
     * instance
     * @return uint32 instance id
     */
    virtual uint32 acquire_instance_resources(const Vector<TextureMap*>& maps);
    /**
     * @brief Release previously acquired instance resources
     * @param instance_id Id of instance to be released
     */
    virtual void   release_instance_resources(uint32 instance_id);

    virtual void acquire_texture_map_resources(TextureMap* texture_map);
    virtual void release_texture_map_resources(TextureMap* texture_map);

    /**
     * @brief Get the the index of a requested uniform
     *
     * @param name The name of the uniform to search for
     * @returns Uniform index if found
     * @throws InvalidArgument exception if no uniform is found
     */
    Result<uint16, InvalidArgument> get_uniform_index(const String name);

    /**
     * @brief Set the uniform value by uniform name
     *
     * @tparam T Value type
     * @param name Uniform name
     * @param value Value we wish to set
     * @throws InvalidArgument exception if no uniform is found
     */
    template<typename T>
    Result<void, InvalidArgument> set_uniform(
        const String name, const T* value
    ) {
        // Get id
        auto id = get_uniform_index(name);
        if (id.has_error())
            return Failure(InvalidArgument(String::build(
                "Couldn't set required uniform \"",
                name,
                "\" because no such uniform exists."
            )));
        // Set uniform with id
        return set_uniform<T>(id.value(), value);
    }
    /**
     * @brief Set the uniform value by uniform id
     *
     * @tparam T Value type
     * @param name Uniform id
     * @param value Value we wish to set
     * @throws InvalidArgument exception if no uniform is found
     */
    template<typename T>
    Result<void, InvalidArgument> set_uniform(const uint16 id, const T* value) {
        if (id < 0 || id >= _uniforms.size())
            return Failure(InvalidArgument(String::build(
                "Couldn't set required uniform id=",
                id,
                " because no such uniform exists."
            )));
        if (set_uniform(id, (void*) value).failed())
            Logger::fatal("Shader :: Uniform_set failed for some reason.");
        return {};
    }

    /**
     * @brief Set the sampler texture by sampler name
     *
     * @param name Sampler name
     * @param texture Texture the sampler will use
     * @throws InvalidArgument exception if no sampler is found
     */
    Result<void, InvalidArgument> set_sampler(
        const String name, const TextureMap* const texture_map
    );
    /**
     * @brief Set the sampler texture by sampler id
     *
     * @param name Sampler id
     * @param texture Texture the sampler will use
     * @throws InvalidArgument exception if no sampler is found
     */
    Result<void, InvalidArgument> set_sampler(
        const uint16 id, const TextureMap* const texture_map
    );

    const static uint32 max_name_length    = 256;
    const static uint32 max_instance_count = 1024;

  protected:
    TextureSystem* _texture_system;

    String _name;
    bool   _use_instances;
    bool   _use_locals;
    uint64 _required_ubo_alignment;

    // Currently bound
    ShaderScope _bound_scope;
    uint32      _bound_instance_id;
    uint32      _bound_ubo_offset;

    // Attributes
    Vector<ShaderAttribute> _attributes {};
    uint16                  _attribute_stride = 0;

    // Named uniforms
    // This vector holds the actual uniform structs
    // All other uniform vectors: global, instance and push constants,
    // store indices to this vector
    Vector<ShaderUniform>        _uniforms {};
    UnorderedMap<String, uint16> _uniforms_hash {};

    // Descriptor Sets
    ShaderDescriptorSet _global_descriptor_set {};
    ShaderDescriptorSet _instance_descriptor_set {};

    // Global uniforms
    Vector<TextureMap*> _global_texture_maps {};

    // Instance uniforms
    uint64                 _instance_ubo_size   = 0;
    uint64                 _instance_ubo_stride = 0;
    Vector<InstanceState*> _instance_states;
    uint8                  _instance_texture_count;

    // Push constants
    uint64         _push_constant_size   = 0;
    uint64         _push_constant_stride = 128;
    Vector<size_t> _push_constants {};

    /**
     * @brief Set the uniform object
     *
     * @param id id of the uniform
     * @param value value to set
     * @param size only used if uniform type is custom, otherwise ignored
     * @return Outcome
     */
    virtual Outcome set_uniform(const uint16 id, void* value);

    ShaderBinding* get_binding(ShaderScope scope, uint32 index);

  private:
    void add_binding(const ShaderBindingConfig& config);
};

} // namespace ENGINE_NAMESPACE