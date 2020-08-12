module PlatformDetector
  module Windows
    module VS
      class Base < Platform
        def windows?
          true
        end

        def vs?
          true
        end

        def library_prefix
          ''
        end

        def library_extension
          '.lib'
        end

        def system_dependencies
          [GLFW.new(self), GLAD.new(self), PhysFS.new(self)]
        end

        def system_libraries
          %w[opengl32 user32 gdi32 Advapi32 shell32 kernel32]
        end
      end
    end
  end
end
