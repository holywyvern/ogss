require 'open-uri'
require 'zip'

DEBUG = ENV.fetch('DEBUG', 'false')

MRUBY_CONFIG =
  File.expand_path(ENV['MRUBY_CONFIG'] || '.travis_build_config.rb')
MRUBY_VERSION = ENV['MRUBY_VERSION'] || '2.1.1'
RAYFORK_VERSION = ENV['RAYFORK_VERSION'] || 'master'
GLFW_VERSION = ENV['GLFW_VERSION'] || '3.3.2'
GLAD_VERSION = ENV['GLAD_VERSION'] || 'master'
MRUBY_BUILD_HOSTS = ['host'] + (ENV['MRUBY_CROSS_BUILDS']&.split(',') || [])
PHYSFS_URL = ENV['PHYSFS_URL'] || 'https://hg.icculus.org/icculus/physfs/archive/default.zip'

file :mruby do
  sh 'git clone --depth=1 git://github.com/mruby/mruby.git'
  if MRUBY_VERSION != 'master'
    Dir.chdir 'mruby' do
      sh 'git fetch --tags'
      rev = `git rev-parse #{MRUBY_VERSION}`
      sh "git checkout #{rev}"
    end
  end
end

file rayfork: :mruby do
  MRUBY_BUILD_HOSTS.each do |host|
    dependency_dir = File.join('mruby', 'build', host, 'dependencies')
    rayfork_dir = File.join(dependency_dir, 'rayfork')
    next if File.exist?(rayfork_dir)

    FileUtils.mkdir_p(dependency_dir)
    Dir.chdir dependency_dir do
      sh 'git clone --recursive https://github.com/SasLuca/rayfork.git'
    end
    next if RAYFORK_VERSION == 'master'

    Dir.chdir rayfork_dir do
      sh 'git fetch --tags'
      rev = `git rev-parse #{RAYFORK_VERSION}`
      sh "git checkout #{rev}"
    end
  end
end

file glfw: :mruby do
  MRUBY_BUILD_HOSTS.each do |host|
    dependency_dir = File.join('mruby', 'build', host, 'dependencies')
    glfw_dir = File.join(dependency_dir, 'glfw')
    next if File.exist?(glfw_dir)

    FileUtils.mkdir_p(dependency_dir)
    Dir.chdir dependency_dir do
      sh 'git clone --recursive https://github.com/glfw/glfw.git'
    end
    next if GLFW_VERSION == 'master'

    Dir.chdir glfw_dir do
      sh 'git fetch --tags'
      rev = `git rev-parse #{GLFW_VERSION}`
      sh "git checkout #{rev}"
    end
  end
end

file glad: :mruby do
  MRUBY_BUILD_HOSTS.each do |host|
    dependency_dir = File.join('mruby', 'build', host, 'dependencies')
    glad_dir = File.join(dependency_dir, 'glad')
    next if File.exist?(glad_dir)

    FileUtils.mkdir_p(dependency_dir)
    Dir.chdir dependency_dir do
      sh 'git clone --recursive https://github.com/Dav1dde/glad.git'
    end
    next if GLAD_VERSION == 'master'

    Dir.chdir glad_dir do
      sh 'git fetch --tags'
      rev = `git rev-parse #{GLAD_VERSION}`
      sh "git checkout #{rev}"
    end
  end
end

file physfs: :mruby do
  MRUBY_BUILD_HOSTS.each do |host|
    dependency_dir = File.join('mruby', 'build', host, 'dependencies')
    physfs_dir = File.join(dependency_dir, 'physfs')
    next if File.exist?(physfs_dir)

    FileUtils.mkdir_p(dependency_dir)
    URI.open(PHYSFS_URL) do |physfs|
      Zip::File.open_buffer(physfs) do |zip|
        zip.each do |f|
          entry = File.join(dependency_dir, f.name)
          FileUtils.mkdir_p(File.dirname(entry))
          zip.extract(f, entry) unless File.exist?(entry)
        end
      end
    end
    Dir.chdir dependency_dir do
      FileUtils.move('physfs-default', 'physfs', force: true)
    end
  end
end

file dgs: :mruby do
  MRUBY_BUILD_HOSTS.each do |host|
    dependency_dir = File.join('mruby', 'build', host, 'dependencies')
    dgs_dir = File.join(dependency_dir, 'dgs')
    next if File.exist?(dgs_dir)

    FileUtils.mkdir_p(dependency_dir)
    Dir.chdir dependency_dir do
      sh 'git clone --recursive https://github.com/holywyvern/Snippets.git dgs'
    end
  end
end

desc 'compile binary'
task compile: %i[mruby rayfork glfw glad physfs dgs] do
  sh "cd mruby && rake all MRUBY_CONFIG=#{MRUBY_CONFIG} DEBUG=#{DEBUG}"
end

desc 'test'
task test: :compile do
  sh "cd mruby && rake all test MRUBY_CONFIG=#{MRUBY_CONFIG} ENABLE_TESTS=true"
end

desc 'cleanup'
task :clean do
  exit 0 unless File.directory?('mruby')
  sh 'cd mruby && rake deep_clean'
end

task default: :compile
