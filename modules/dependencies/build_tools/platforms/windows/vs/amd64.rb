module PlatformDetector
  module Windows
    module VS
      class AMD64 < Base
        def name
          'Windows with Visual Studio (AMD64)'
        end

        def compiler_definitions
          %w[
            ORGF_PLATFORM_DESKTOP
            ORGF_PLATFORM_AMD64
            ORGF_PLATFORM_WINDOWS
            ORGF_PLATFORM_VS
            ORGF_PLATFORM_WINDOWS_VS_AMD64
            ORGF_PLATFORM_GLFW
            ORGF_PLATFORM_OPENGL
            ORGF_PLATFORM_PHYSFS
          ]
        end
      end
    end
  end
end
