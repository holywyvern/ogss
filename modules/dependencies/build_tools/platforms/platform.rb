module PlatformDetector
  class BasePlatform
    attr_reader :spec

    def initialize(spec)
      @spec = spec
    end

    def cc
      spec.cc
    end

    def cxx
      spec.cxx
    end

    def linker
      spec.linker
    end

    def library_prefix
      ''
    end

    def library_extension
      '.a'
    end

    def ios?
      false
    end

    def osx?
      false
    end

    def windows?
      false
    end

    def vs?
      false
    end

    def linux?
      false
    end
  end

  class Platform < BasePlatform
    def compile_dependencies
      all_dependencies.each do |dependency|
        dependency.configure
        dependency.compile
      end
    end

    def configure_platform
      configure_compilers
      configure_linker
    end

    protected

    def system_dependencies
      []
    end

    def compiler_flags
      []
    end

    def linker_flags
      []
    end

    def system_libraries
      []
    end

    private

    def all_dependencies
      @all_dependencies ||= core_dependencies + system_dependencies
    end

    def core_dependencies
      [rayfork]
    end

    def rayfork
      @rayfork ||= Rayfork.new(self)
    end

    def configure_compilers
      compilers.each do |c|
        c.defines       += compiler_definitions
        c.flags         += compiler_flags
        c.include_paths += include_paths
      end
    end

    def compilers
      [cc, cxx, spec.build.cc, spec.build.cxx]
    end

    def configure_linker
      linker.library_paths += library_paths
      linker.libraries     += libraries
      linker.flags         += linker_flags
    end

    def include_paths
      all_dependencies.map(&:include_paths).sum([])
    end

    def library_paths
      all_dependencies.map(&:library_paths).sum([])
    end

    def libraries
      all_dependencies.map(&:libraries).sum([]) + system_libraries
    end

    def compiler_definitions
      []
    end

    def name
      raise 'Platform does not have a name'
    end
  end
end
