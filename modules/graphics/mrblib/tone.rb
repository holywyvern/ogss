class Tone
  def self._load(data)
    new(*data.unpack('s4'))
  end

  def _dump(_depth = 0)
    [red, green, blue, gray].pack('s4')
  end
end
