MRuby::Gem::Specification.new('ogss-marshal') do |spec|
  spec.license = 'Apache-2.0'
  spec.author  = 'Ramiro Rojo'

  spec.add_dependency 'ogss-dependencies'
  spec.add_dependency 'ogss-filesystem'
  spec.add_dependency 'mruby-pack'
end
