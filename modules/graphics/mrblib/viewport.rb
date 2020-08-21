class Viewport
  delegate :x, :x=, :y, :y=, :width, :width=, :height, :height=, :set, to: :rect

  def rect=(value)
    if value.is_a?(Array)
      rect.set(*value)
    else
      rect.set(value)
    end
  end

  def opacity
    color.alpha
  end

  def color=(value)
    if value.is_a?(Array)
      color.set(*value)
    else
      color.set(value)
    end
  end

  def tone=(value)
    if value.is_a?(Array)
      tone.set(*value)
    else
      tone.set(value)
    end
  end

  def opacity=(value)
    color.alpha = value
  end

  def offset=(value)
    if value.is_a?(Array)
      offset.set(*value)
    else
      offset.set(value)
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

  def show
    self.visible = true
  end

  def hide
    self.visible = false
  end

  def oy=(value)
    offset.y = value
  end
end
