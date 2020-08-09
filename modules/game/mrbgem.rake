MRuby::Gem::Specification.new('ogss-game') do |spec|
  spec.license = 'Apache-2.0'
  spec.author  = 'Ramiro Rojo'

  spec.bins = ['game']

  spec.add_dependency 'ogss-dependencies'
  spec.add_dependency 'ogss-core'
  spec.add_dependency 'ogss-audio'
  spec.add_dependency 'ogss-filesystem'
  spec.add_dependency 'ogss-graphics'
  spec.add_dependency 'ogss-input'
  spec.add_dependency 'ogss-math'
end
