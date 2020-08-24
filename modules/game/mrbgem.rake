MRuby::Gem::Specification.new('orgf-game') do |spec|
  spec.license = 'Apache-2.0'
  spec.author  = 'Ramiro Rojo'

  spec.bins = ['game']

  spec.add_dependency 'orgf-dependencies'
  spec.add_dependency 'orgf-core'
  spec.add_dependency 'orgf-audio'
  spec.add_dependency 'orgf-filesystem'
  spec.add_dependency 'orgf-graphics'
  spec.add_dependency 'orgf-input'
  spec.add_dependency 'orgf-math'
end
