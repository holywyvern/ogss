module PlatformDetector
  class << self
    def detect(spec)
      raise 'No Cross build support for now' if spec.is_a?(MRuby::CrossBuild)

      os = ENV['OS'] || RUBY_PLATFORM
      case os
      when 'Windows_NT'
        windows_detection(spec)
      when 'x86_64-linux'
        linux_amd64_detection(spec)
      else
        raise "No build found for operating system: '#{os}'"
      end
    end

    private

    def windows_detection(spec)
      arch = ENV['PROCESSOR_ARCHITECTURE']
      case arch
      when 'AMD64'
        PlatformDetector::Windows::VS::AMD64.new(spec)
      else
        raise "There is no target for the folowing architecture: '#{arch}'"
      end
    end

    def linux_amd64_detection(spec)
      raise 'Linux builds require GCC' unless gcc?

      PlatformDetector::Linux::GCC::AMD64.new(spec)
    end

    def gcc?
      spec.build.toolchains.include?('gcc')
    end
  end
end
