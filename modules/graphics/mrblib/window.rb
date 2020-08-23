class Window
  delegate :top, :bottom, :left, :right,
           :top=, :bottom=, :left=, :right=, to: :padding
  delegate :x, :y, :width, :height, :x=, :y=, :width=, :height=, to: :rect
  delegate :x, :y, :x=, :y=, to: :offset, prefix: true

  class Padding
    def inspect
      "Padding(#{top}, #{bottom}, #{left}, #{right})"
    end
  end

  def ox
    offset.x
  end

  def ox=(value)
    offset.x = value
  end

  def oy
    offset.y
  end

  def oy=(value)
    offset.y = value
  end

  def move(xpos, ypos, width = rect.width, height = rect.height)
    rect.set(xpos, ypos, width, height)
  end

  def resize(width, height)
    rect.set(x, y, width, height)
  end

  def open?
    openess >= 255
  end

  def close?
    openess.zero?
  end

  def back_opacity
    color.alpha
  end

  def back_opacity=(value)
    color.alpha = value
  end

  def tone=(value)
    if value.is_a?(Array)
      tone.set(*value)
    else
      tone.set(value)
    end
  end

  def rect=(value)
    if value.is_a?(Array)
      rect.set(*value)
    else
      rect.set(value)
    end
  end

  def padding=(value)
    if value.is_a?(Array)
      padding.set(*value)
    elsif value.is_a?(Numeric)
      padding.top = padding.bottom = padding.left = padding.right = value
    else
      padding.set(value)
    end
  end

  def activate
    self.active = true
  end

  def deactivate
    self.active = false
  end

  def show
    self.visible = true
  end

  def hide
    self.visible = false
  end
end
