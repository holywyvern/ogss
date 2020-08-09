class Rayfork < Dependency
  def configure
    super

    File.open(cmake_file, 'a') do |f|
      f.puts ''
      f.puts 'add_compile_definitions(RAYFORK_ENABLE_AUDIO)'
    end
  end

  def cmake_file
    @cmake_file ||= File.join(dir, 'CMakeLists.txt')
  end

  def libraries
    ['rayfork']
  end
end
