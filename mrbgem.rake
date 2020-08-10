MRuby::Gem::Specification.new('ogss-game-engine') do |spec|
  spec.license = 'Apache-2.0'
  spec.author  = 'Ramiro Rojo'

  DEPENDENCIES = %w[
    dependencies core math filesystem marshal graphics audio input game
  ].freeze

  DEPENDENCIES.each do |dep|
    spec.build.gem File.expand_path(File.join(__dir__, 'modules', dep))
  end
end
