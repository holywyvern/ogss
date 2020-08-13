module Kernel
  module_function

  def p(*args)
    puts(*args.map(&:inspect))
  end
end
