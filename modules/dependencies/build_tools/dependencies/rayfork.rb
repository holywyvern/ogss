class Rayfork < Dependency
  def cmake_configure_flags
    '-DRAYFORK_ENABLE_AUDIO=TRUE'
  end

  def libraries
    ['rayfork']
  end
end
