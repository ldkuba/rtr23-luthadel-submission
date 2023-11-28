/**
 * @brief This file contains a list of globally used macros. None of this names
 * can be given to any class, method, macro or other.
 */

/*
// === Engine specific ===
//  Supported platforms
    -   PLATFORM        Note: Check current platform (ex. PLATFORM == LINUX)
    -   LINUX
    -   WINDOWS32
    -   WINDOWS64
//  For logs
    -   fatal           Note: Used only in Logger::fatal, not alone
    -   LOG_LOCATION    Node: Current file, line and function for output
    // Would like to get rid of:
    -   ALLOCATOR_LOG
    -   RESOURCE_LOG
    -   RENDERER_VULKAN_LOG
//  Property
    -   GET
    -   SET
//  Result
    -   match_error(result)
    -   match_error_code(result)
    -   Err(error)
    -   Ok()
    -   check(result)
//  Multithreading
    -   for_loop
    -   for_loop_1 & for_loop_2
    -   GET_FOR_LOOP_MACRO
//  General for use
    -   APP_NAME
    -   ENGINE_NAME
    -   ENGINE_NAMESPACE
    -   MEMORY_PADDING      Note: Memory padding used by allocators
    -   StringEnum          Note: `constexpr static const char* const` type
    -   STRING_ENUM(str)    Note: Used for custom "String" enum classes
// Serialization extensions
    - serializable_attributes
//  General, NOT FOR USE
    -   ...

//  === Library specific ===
//  Tiny loader
    -   TINYOBJLOADER_IMPLEMENTATION (TODO: TEMP)
//  GLM
    -   GLM_FORCE_RADIANS
    -   GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
    -   GLM_FORCE_DEPTH_ZERO_TO_ONE
    -   GLM_ENABLE_EXPERIMENTAL
*/
