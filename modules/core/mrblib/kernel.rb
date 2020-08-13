module Kernel
  module_function

  def load_file(filename)
    obj = nil
    # rubocop:disable Security/MarshalLoad
    File.open(filename) { |f| obj = Marshal.load(f) }
    # rubocop:enable Security/MarshalLoad
    obj
  end

  def load_data(filename)
    load_file("@saves/#{filename}")
  end

  def save_data(obj, filename)
    File.open(filename, 'w') { |f| Marshal.dump(obj, f) }
    nil
  end
end
