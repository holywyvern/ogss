module Kernel
  module_function

  def p(*args)
    puts(*args.map(&:inspect))
  end

  def msgbox_p(*args)
    msgbox(*args.map(&:inspect))
  end
end
