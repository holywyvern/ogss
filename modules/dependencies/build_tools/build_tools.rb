require_relative 'console'

require_relative 'platform_detector'

require_relative 'dependency'
require_relative 'dependencies/glad'
require_relative 'dependencies/glfw'
require_relative 'dependencies/rayfork'
require_relative 'dependencies/physfs'
require_relative 'dependencies/dgs'

require_relative 'platforms/platform'
# Windows builds
require_relative 'platforms/windows/vs/base'
require_relative 'platforms/windows/vs/amd64'
# Linux builds
require_relative 'platforms/linux/gcc/base'
require_relative 'platforms/linux/gcc/amd64'
