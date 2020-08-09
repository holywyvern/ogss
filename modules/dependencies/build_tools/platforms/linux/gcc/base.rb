module PlatformDetector
  module Linux
    module GCC
      class Base < Platform
        def linux?
          true
        end

        def system_libraries
          ['pthread', 'm', 'dl']
        end

        def compiler_definitions
          [
            'OGSS_PLATFORM_DESKTOP',
            'OGSS_PLATFORM_AMD64',
            'OGSS_PLATFORM_LINUX',
            'OGSS_PLATFORM_GCC',
            'OGSS_PLATFORM_LINUX_GCC_AMD64'
          ]
        end        
      end
    end
  end
end
