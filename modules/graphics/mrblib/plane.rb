class Plane
  delegate :x, :x=, :y, :y=, to: :offset, prefix: true
  delegate :x, :x=, :y, :y=, to: :scroll, prefix: true
  delegate :x, :x=, :y, :y=, to: :scale, prefix: true
  delegate :x, :x=, :y, :y=, to: :zoom, prefix: true

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

  def offset=(value)
    if value.is_a?(Array)
      offset.set(*value)
    else
      offset.set(value)
    end
  end

  def scale=(value)
    if value.is_a?(Array)
      scale.set(*value)
    else
      scale.set(value)
    end
  end

  def color=(value)
    if value.is_a?(Array)
      color.set(*value)
    else
      color.set(value)
    end
  end

  def opacity
    color.alpha
  end

  def opacity=(value)
    color.alpha = value
  end

  alias zoom= scale=
end
