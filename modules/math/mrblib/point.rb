class Point
  def empty
    set(0, 0)
  end

  def inspect
    "Point(#{x}, #{y})"
  end
end

Vector2 = Point
