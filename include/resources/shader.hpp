#pragma once

#include "texture.hpp"
#include "unordered_map.hpp"

#include "outcome.hpp"

namespace ENGINE_NAMESPACE {

class TextureSystem;
class Renderer;

/// @brief Byte range description
struct ByteRange {
    uint64 offset;
    uint64 size;
};

/**
 * @brief Frontend (API agnostic) representation of a shader.
 */
class Shader {
  public:
    /**
     * @brief List of builtin shaders.
     */
    struct BuiltIn {
        StringEnum MaterialShader = "builtin.material_shader";
        StringEnum UIShader       = "builtin.ui_shader";
        StringEnum SkyboxShader   = "builtin.skybox_shader";
    };

    /// @brief Supported shader attribute types
    enum class AttributeType : uint8 {
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
    enum class UniformType : uint8 {
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
    enum class Stage : uint8 {
        Vertex   = 0x1,
        Geometry = 0x2,
        Fragment = 0x4,
        Compute  = 0x8
    };

    /// @brief Shader scope
    enum class Scope : uint8 { Global, Instance, Local };

    /// @brief Determines what face culling mode will be used during rendering
    enum class CullMode { None, Front, Back, Both };

    /// @brief Structure containing all attribute relevant data
    struct Attribute {
        String        name;
        uint32        size;
        AttributeType type;
    };

    /// @brief Structure containing all uniform relevant data
    struct Uniform {
        /// @brief Shader uniform configuration
        struct Config {
            String      name;
            uint32       size;
            uint32      array_index;
            UniformType type;
        };

        ByteRange byte_range;  // Byte range of this uniform in the buffer
        uint16    array_index; // Index array in a binding (TODO: this is not
                            // necessary, only kept beacuse samplers which are
                            // part of the same uniform are named differently
                            // and this is a hack to keep them separated)
        uint32 binding_index; // Index of binding which holds this uniform (-1
                              // for push constants)
        uint32 set_index; // Index of the set this uniform belongs to (-1 for
                          // push constants)
        Scope  scope;
        UniformType type;
    };

    /// @brief Structure containing all binding relevant data
    struct Binding {
        /// @brief Shader binding type
        enum class Type : uint8 { Uniform, Sampler, Storage };

        /// @brief Shader binding configuration
        struct Config {
            uint32                  binding_index;
            Type                    type;
            size_t                  count;
            uint8                   shader_stages;
            Vector<Uniform::Config> uniforms;
        };

        uint32    set_index;     // Index of the set this binding belongs to
        uint32    binding_index; // Index of this binding in the set
        ByteRange byte_range;    // Byte range of this binding in the buffer
        uint32    total_size; // Total size of this binding without allignment
        Type      type;
        size_t    count; // Number of elements in this binding
        uint8     shader_stages;
        Vector<size_t> uniforms; // Vector of indices to uniforms in this
                                 // binding into the main uniform array
        bool           was_modified { true };
    };

    /// @brief Structure containing all descriptor set relevant data
    struct DescriptorSet {
        /**
         * @brief Descriptor set state
         */
        struct State {
            uint64 offset;
            bool   should_update = true;

            // For each sampler binding we need to store a vector of textures
            // TODO: once "named" sampler bindings are implemented properly
            // we could change it to something that relies on the name of the
            // binding instead of the index
            UnorderedMap<uint32, Vector<Texture::Map*>> texture_maps;
        };

        struct BackendData {};

        struct Config {
            uint32                  set_index;
            Scope                   scope;
            Vector<Binding::Config> bindings;
        };

        Vector<Binding> bindings;
        uint64          stride;     // Stride of this descriptor set
        uint64          total_size; // Total size of this descriptor set without
                                    // allignment
        Scope           scope;
        Vector<State*>  states {};
        BackendData*    backend_data = nullptr;
        uint32          texture_map_count;
        uint32          set_index; // Index of this set
    };

    /**
     * @brief Shader configuration resource. Usually this resource is loaded
     * from a .shadercfg file.
     */
    class Config : public Resource {
      public:
        const String                        render_pass_name;
        const uint8                         shader_stages;
        const Vector<Attribute>             attributes;
        const Vector<DescriptorSet::Config> sets;
        const Vector<Uniform::Config>       push_constants;
        const CullMode                      cull_mode;

        Config(
            const String&                        name,
            const String&                        render_pass_name,
            const uint8                          shader_stages,
            const Vector<Attribute>&             attributes,
            const Vector<DescriptorSet::Config>& sets,
            const Vector<Uniform::Config>&       push_constants,
            const CullMode                       cull_mode
        )
            : Resource(name), render_pass_name(render_pass_name),
              shader_stages(shader_stages), attributes(attributes), sets(sets),
              push_constants(push_constants), cull_mode(cull_mode) {}
        ~Config() {}
    };

  public:
    uint64 rendered_frame_number = -1;

  public:
    /**
     * @brief Construct a new Shader object
     *
     * @param config Shader configurations
     */

    /**
     * @brief Construct a new Shader object
     * @param renderer Renderer which owns this shader
     * @param texture_system Texture system reference
     * @param config Shader configuration used
     */
    Shader(
        Renderer* const      renderer,
        TextureSystem* const texture_system,
        const Config&        config
    );
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
     * @brief Apply set instance uniforms. _bound_instance_id will be used
     */
    virtual void apply_instance();

    virtual void acquire_global_resources();
    virtual void release_global_resources();

    /**
     * @brief Acquires resources required for initialization of a shader
     * instance
     * @param maps Maps used by this shader. Need to be pre-initialized.
     * @return uint32 instance id
     */
    virtual uint32 acquire_instance_resources( //
        const Vector<Texture::Map*>& maps
    );
    /**
     * @brief Release previously acquired instance resources
     * @param instance_id Id of instance to be released
     */
    virtual void   release_instance_resources(uint32 instance_id);

    /**
     * @brief Get the the index of a requested uniform
     *
     * @param name The name of the uniform to search for
     * @returns Uniform index if found
     * @throws InvalidArgument exception if no uniform is found
     */
    Result<uint16, InvalidArgument> get_uniform_index(const String& name) const;

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
        const String name, const Texture::Map* const texture_map
    );
    /**
     * @brief Set the sampler texture by sampler id
     *
     * @param name Sampler id
     * @param texture Texture the sampler will use
     * @throws InvalidArgument exception if no sampler is found
     */
    Result<void, InvalidArgument> set_sampler(
        const uint16 id, const Texture::Map* const texture_map
    );

    const static uint32 max_name_length    = 256;
    const static uint32 max_instance_count = 1024;

  protected:
    TextureSystem* _texture_system;
    Renderer*      _renderer;

    String   _name;
    CullMode _cull_mode;
    uint64   _required_ubo_alignment;

    // Uniform counts
    uint32 _uniform_count_global           = 0;
    uint32 _uniform_count_instance         = 0;
    uint32 _uniform_count_local            = 0;
    uint32 _uniform_sampler_count_global   = 0;
    uint32 _uniform_sampler_count_instance = 0;

    // Currently bound
    Scope  _bound_scope;
    uint32 _bound_instance_id;

    // Attributes
    Vector<Attribute> _attributes {};
    uint16            _attribute_stride = 0;

    // Named uniforms
    // This vector holds the actual uniform structs
    // All other uniform vectors: global, instance and push constants,
    // store indices to this vector
    Vector<Uniform>              _uniforms {};
    UnorderedMap<String, uint16> _uniforms_hash {};

    // Descriptor Sets
    Vector<DescriptorSet> _descriptor_sets {};

    // Instance uniforms
    uint64 _instance_ubo_size   = 0;
    uint64 _instance_ubo_stride = 0;

    // Push constants
    uint64         _push_constant_size   = 0;
    uint64         _push_constant_stride = 128;
    Vector<size_t> _push_constants {};

    /**
     * @brief Set the uniform object. If uniform is in instance set,
     * _bound_instance_id will be used
     *
     * @param id id of the uniform
     * @param value value to set
     * @return Outcome
     */
    virtual Outcome set_uniform(const uint16 id, void* value);

    Binding* get_binding(uint32 set_index, uint32 binding_index);

  private:
    void add_binding(const Binding::Config& config, const uint32 set_index);
};

} // namespace ENGINE_NAMESPACE