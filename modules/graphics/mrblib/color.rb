class Color
  def self.random(alpha = 255)
    new(rand(255), rand(255), rand(255), alpha)
  end

  def self._load(data)
    new(*data.unpack('C4'))
  end

  def _dump(_depth = 0)
    [red, green, blue, alpha].pack('C4')
  end

  def invert
    dup.invert!
  end

  def invert!
    self.red = 255 - red
    self.green = 255 - green
    self.blue = 255 - blue
    self
  end

  def grayscale
    dup.grayscale!
  end

  def grayscale!
    gray = (0.3 * red) + (0.59 * green) + (0.11 * blue)
    self.red = gray
    self.green = gray
    self.blue = gray
    self
  end

  def inspect
    "Color(#{red}, #{green}, #{blue}, #{alpha})"
  end
end
