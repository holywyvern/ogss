class GLFW < Dependency
  def libraries
    []
  end

  def build_command
    "cmake #{cmake_configure_flags} .."
  end
end
