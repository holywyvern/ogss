class Sprite
  delegate :x, :x=, :y, :y=, to: :position
  delegate :x, :x=, :y, :y=, to: :anchor, prefix: true
  delegate :x, :x=, :y, :y=, to: :scale, prefix: true
  delegate :x, :x=, :y, :y=, to: :zoom, prefix: true

  def width
    src_rect.width * scale.x.abs
  end

  def height
    src_rect.height * scale.y.abs
  end

  def ox
    anchor.x * width
  end

  def ox=(value)
    achor.x = value.to_f / width.to_f
  end

  def oy
    anchor.y * height
  end

  def oy=(value)
    achor.y = value.to_f / height.to_f
  end

  def position=(value)
    if value.is_a?(Array)
      position.set(*value)
    else
      position.set(value)
    end
  end

  def anchor=(value)
    if value.is_a?(Array)
      anchor.set(*value)
    else
      anchor.set(value)
    end
  end

  def scale=(value)
    scale.set(value)
  end

  def src_rect=(value)
    if value.is_a?(Array)
      src_rect.set(*value)
    else
      src_rect.set(value)
    end
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

  def mirror_x
    scale.x.negative?
  end

  def mirror_y
    scale.y.negative?
  end

  def mirror_x=(value)
    scale.x = scale.x.abs * (value ? -1 : 1)
  end

  def mirror_y=(value)
    scale.y = scale.y.abs * (value ? -1 : 1)
  end

  def opacity
    color.alpha
  end

  def opacity=(value)
    color.alpha = value
  end

  def show
    self.visible = true
  end

  def hide
    self.visible = false
  end

  alias mirror mirror_x
  alias mirror= mirror_x=
  alias zoom= scale=
end
