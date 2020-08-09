module Kernel
  module_function

  def load_data(filename)
    obj = nil
    # rubocop:disable Security/MarshalLoad
    File.open(filename) { |f| obj = Marshal.load(f) }
    # rubocop:enable Security/MarshalLoad
    obj
  end

  def save_data(obj, filename)
    File.open(filename, 'w') { |f| Marshal.dump(obj, f) }
    nil
  end
end
