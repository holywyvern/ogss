require_relative 'build_tools/build_tools'

MRuby::Gem::Specification.new('ogss-dependencies') do |spec|
  spec.license = 'Apache-2.0'
  spec.author  = 'Ramiro Rojo'

  platform = PlatformDetector.detect(spec)
  platform.compile_dependencies
  platform.configure_platform
end
