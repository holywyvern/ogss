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
            'ORGF_PLATFORM_DESKTOP',
            'ORGF_PLATFORM_AMD64',
            'ORGF_PLATFORM_LINUX',
            'ORGF_PLATFORM_GCC',
            'ORGF_PLATFORM_LINUX_GCC_AMD64'
          ]
        end        
      end
    end
  end
end
