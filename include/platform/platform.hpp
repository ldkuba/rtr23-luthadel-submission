#pragma once

#include <string>

#include <vulkan/vulkan.hpp>

#include "vector.hpp"
#include "defines.hpp"
#include "event.hpp"
#include "result.hpp"
#include "error_types.hpp"

/**
 * @brief Static class representing the platform layer of the application.
 * Provides a platform agnostic way to access certain functionalities of the
 * system. Individual supported platforms should provide their own .cpp
 * implementation files.
 */
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
    static const Vector<const char*> get_required_vulkan_extensions();

    /**
     * @brief A platform agnostic Console I/O class. Can only be used if the
     * platform supports a console.
     *
     */
    class Console {
      private:
      public:
        /**
         * @brief Construct a new Console object
         *
         */
        Console();
        ~Console();

        /**
         * @brief Write a message to the console
         *
         * @param message Message contents
         * @param kind Determins message look and importance
         * @param new_line Adds a new line after the message
         */
        static void write(
            std::string message, uint32 kind = 0, bool new_line = true
        );
        /**
         * @brief Read plain text from console
         *
         * @return std::string
         */
        static std::string read();
    };

    /**
     * @brief A platform agnostic render-able surface. Can be a window or the
     * entire screen.
     *
     */
    class Surface {
      public:
        virtual ~Surface() {}
        /// @brief Invoked on surface resize
        Event<void, uint32, uint32> resize_event;

        /**
         * @brief Create an instance of Surface object
         *
         * @param width Surface width
         * @param height Surface height
         * @param name Surface name
         * @return Reference to the created surface
         */
        static Surface* get_instance(
            uint32 width, uint32 height, std::string name
        );

        // Vulkan functions (TODO: Might be moved)
        virtual Result<vk::SurfaceKHR, RuntimeError> get_vulkan_surface(
            const vk::Instance&                  vulkan_instance,
            const vk::AllocationCallbacks* const allocator
        ) const {
            return vk::SurfaceKHR();
        }

        /**
         * @brief Get surface width in pixels
         * @return width in pixels
         */
        virtual uint32 get_width_in_pixels() { return 0; }
        /**
         * @brief Get surface height in pixels
         * @return height in pixels
         */
        virtual uint32 get_height_in_pixels() { return 0; }

        /**
         * @brief Process surface related events
         */
        virtual void process_events() {}
        /**
         * @brief Check if surface should close
         *
         * @return true If surface closure event is detected
         * @return false Otherwise
         */
        virtual bool should_close() { return false; }

      protected:
        Surface() {}
    };
};