MRuby::Gem::Specification.new('orgf-graphics') do |spec|
  spec.license = 'Apache-2.0'
  spec.author  = 'Ramiro Rojo'

  spec.add_dependency 'orgf-dependencies'
  spec.add_dependency 'orgf-filesystem'
  spec.add_dependency 'orgf-core'
  spec.add_dependency 'orgf-math'

  spec.add_dependency 'mruby-time'
end
