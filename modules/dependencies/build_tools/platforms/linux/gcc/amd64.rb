module PlatformDetector
  module Linux
    module GCC
      class AMD64 < Base
        def name
          'Linux with GCC (AMD64)'
        end        
      end
    end
  end
end
