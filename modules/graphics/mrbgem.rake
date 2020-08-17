MRuby::Gem::Specification.new('ogss-graphics') do |spec|
  spec.license = 'Apache-2.0'
  spec.author  = 'Ramiro Rojo'

  spec.add_dependency 'ogss-dependencies'
  spec.add_dependency 'ogss-filesystem'
  spec.add_dependency 'ogss-core'
  spec.add_dependency 'ogss-math'

  spec.add_dependency 'mruby-time'
end
