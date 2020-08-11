class Font
  class << self
    attr_accessor :default_name, :default_size, :default_antialias
  end
end

Font.default_name = 'OpenSans.ttf'
Font.default_size = 16
Font.default_antialias = false
