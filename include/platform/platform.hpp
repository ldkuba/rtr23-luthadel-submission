#pragma once

#include <vector>
#include <string>

#include <vulkan/vulkan.hpp>

#include "defines.hpp"
#include "event.hpp"
#include "result.hpp"
#include "error_types.hpp"

class Platform {
  private:
  public:
    Platform();
    ~Platform();

    /// @brief Get current internal clock time in seconds
    /// @return Time in seconds
    static float64 get_absolute_time();
    /// @brief Suspends the application process for the specified amount of time
    /// @param ms Time to sleep in miliseconds
    static void    sleep(uint64 ms);

    // TODO: Separate platform code from knowing about renderers
    static const std::vector<const char*> get_required_vulkan_extensions();

    class Console {
      private:
      public:
        Console();
        ~Console();

        static void write(
            std::string message, uint32 kind = 0, bool new_line = true
        );
        static std::string read();
    };

    class Surface {
      private:
      protected:
        Surface() {}

      public:
        virtual ~Surface() {}
        Event<void, uint32, uint32> resize_event;

        static Surface* get_instance(
            uint32 width, uint32 height, std::string name
        );

        // Vulkan functions
        virtual Result<vk::SurfaceKHR, RuntimeError> get_vulkan_surface(
            const vk::Instance&                  vulkan_instance,
            const vk::AllocationCallbacks* const allocator
        ) const {
            return vk::SurfaceKHR();
        }

        virtual uint32 get_width_in_pixels() { return 0; }
        virtual uint32 get_height_in_pixels() { return 0; }

        virtual void process_events() {}
        virtual bool should_close() { return false; }
    };

    // class FileSystem {
    // private:
    // public:
    //     FileSystem() {};
    //     ~FileSystem() {};
    // };
};