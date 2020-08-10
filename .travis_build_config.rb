MRuby::Build.new do |conf| # rubocop:disable Metrics/BlockLength
  CORE_GEMS = %w[
    mruby-metaprog mruby-pack mruby-sprintf mruby-math mruby-time mruby-struct
    mruby-compar-ext mruby-enum-ext mruby-string-ext mruby-numeric-ext
    mruby-array-ext mruby-hash-ext mruby-range-ext mruby-proc-ext
    mruby-symbol-ext mruby-random mruby-object-ext mruby-objectspace
    mruby-fiber mruby-enumerator mruby-enum-lazy mruby-toplevel-ext
    mruby-rational mruby-complex mruby-bin-mirb mruby-bin-mruby
    mruby-bin-strip mruby-kernel-ext mruby-class-ext mruby-method
    mruby-eval mruby-compiler
  ].freeze
  TEST_REPOS = %w[ksss/mruby-stringio].freeze
  REPOS = %w[iij/mruby-regexp-pcre].freeze
  MODULES = %w[
    dependencies core math filesystem marshal graphics audio input game
  ].freeze

  if ENV['VisualStudioVersion'] || ENV['VSINSTALLDIR']
    toolchain :visualcpp
  else
    toolchain :gcc
  end

  CORE_GEMS.each { |name| conf.gem core: name }
  if ENV['ENABLE_TESTS'] == 'true'
    TEST_REPOS.each { |repo| conf.gem github: repo }
  end
  REPOS.each { |repo| conf.gem github: repo }
  conf.gem __dir__

  conf.enable_test if ENV['ENABLE_TESTS'] == 'true'

  if ENV['DEBUG'] == 'true'
    conf.enable_debug
    conf.cc.defines = %w[MRB_ENABLE_DEBUG_HOOK]
    conf.gem core: 'mruby-bin-debugger'
  end
end
