module PlatformDetector
  module Windows
    module VS
      class AMD64 < Base
        def name
          'Windows with Visual Studio (AMD64)'
        end

        def compiler_definitions
          %w[
            OGSS_PLATFORM_DESKTOP
            OGSS_PLATFORM_AMD64
            OGSS_PLATFORM_WINDOWS
            OGSS_PLATFORM_VS
            OGSS_PLATFORM_WINDOWS_VS_AMD64
            OGSS_PLATFORM_GLFW
            OGSS_PLATFORM_OPENGL
            OGSS_PLATFORM_PHYSFS
          ]
        end
      end
    end
  end
end
