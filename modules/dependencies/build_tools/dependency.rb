class Dependency
  attr_reader :platform

  def initialize(platform)
    @platform = platform
  end

  def spec
    @spec ||= platform.spec
  end

  def sh(command)
    @spec.send(:sh, command)
  end

  def configure
    return if already_compiled?

    DependencyConsole.log "Configuring #{name}"
    FileUtils.mkdir_p(build_dir)
    Dir.chdir(build_dir) do
      sh config_command
    end
  end

  def compile
    return if already_compiled?

    DependencyConsole.log "Compiling #{name}"
    Dir.chdir(build_dir) do
      sh build_command
    end
  end

  def build_command
    cmd = "cmake --build #{cmake_build_flags} ."
    vs = platform.windows? && platform.vs?
    vs ? "#{cmd} --config Release" : cmd
  end

  def config_command
    "cmake #{cmake_configure_flags} .."
  end

  def cmake_configure_flags
    ''
  end

  def cmake_build_flags
    ''
  end

  def include_paths
    [
      dir, File.join(dir, 'include'), File.join(build_dir, 'include'),
      File.join(dir, 'src'), File.join(build_dir, 'src')
    ].filter { |f| File.exist?(f) }
  end

  def library_paths
    [
      build_dir, File.join(build_dir, 'Release'),
      File.join(build_dir, 'src'), File.join(build_dir, 'src', 'Release')
    ].filter { |f| File.exist?(f) }
  end

  def libraries
    []
  end

  def name
    @name ||= self.class.name.downcase
  end

  def dir
    @dir ||= File.join(spec.build.build_dir, 'dependencies', name)
  end

  def build_dir
    @build_dir = File.join(dir, 'build')
  end

  def already_compiled?
    libraries.all? { |lib| library_exist?(lib) }
  end

  private

  def library_exist?(lib)
    library_paths.any? do |path|
      file = File.join(path, "#{library_prefix}#{lib}#{library_extension}")
      File.exist?(file)
    end
  end

  def library_prefix
    platform.library_prefix
  end

  def library_extension
    platform.library_extension
  end
end
