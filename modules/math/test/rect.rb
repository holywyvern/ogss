assert('Rect.new') do
  rect = Rect.new
  rect_2 = Rect.new(1, 2, 3, 4)
  rect_3 = Rect.new(rect_2)
  assert_equal(rect.x, 0)
  assert_equal(rect.y, 0)
  assert_equal(rect.width, 0)
  assert_equal(rect.height, 0)
  assert_equal(rect_2.x, 1)
  assert_equal(rect_2.y, 2)
  assert_equal(rect_2.width, 3)
  assert_equal(rect_2.height, 4)
  assert_equal(rect_2.x, rect_3.x)
  assert_equal(rect_2.y, rect_3.y)
  assert_equal(rect_2.width, rect_3.width)
  assert_equal(rect_2.height, rect_3.height)
end

assert('Rect#x') do
  rect = Rect.new(1, 2, 3, 4)
  assert_equal(rect.x, 1)
end

assert('Rect#y') do
  rect = Rect.new(1, 2, 3, 4)
  assert_equal(rect.y, 2)
end

assert('Rect#width') do
  rect = Rect.new(1, 2, 3, 4)
  assert_equal(rect.width, 3)
end

assert('Rect#height') do
  rect = Rect.new(1, 2, 3, 4)
  assert_equal(rect.height, 4)
end

assert('Rect#x=') do
  rect = Rect.new
  rect.x = 8
  assert_equal(rect.x, 8)
end

assert('Rect#y=') do
  rect = Rect.new
  rect.y = 8
  assert_equal(rect.y, 8)
end

assert('Rect#width=') do
  rect = Rect.new
  rect.width = 8
  assert_equal(rect.width, 8)
end

assert('Rect#height=') do
  rect = Rect.new
  rect.height = 8
  assert_equal(rect.height, 8)
end

assert('Rect#set') do
  rect = Rect.new
  rect.set(1, 2, 3, 4)
  assert_equal(rect.x, 1)
  assert_equal(rect.y, 2)
  assert_equal(rect.width, 3)
  assert_equal(rect.height, 4)
  rect.set(Rect.new)
  assert_equal(rect.x, 0)
  assert_equal(rect.y, 0)
  assert_equal(rect.width, 0)
  assert_equal(rect.height, 0)
end

assert('Rect#empty') do
  rect = Rect.new(1, 2, 3, 4)
  rect.empty
  assert_equal(rect.x, 0)
  assert_equal(rect.y, 0)
  assert_equal(rect.width, 0)
  assert_equal(rect.height, 0)
end

assert('Rect#_dump') do
  rect = Rect.new(1, 2, 3, 4)
  assert_equal(rect._dump, [1, 2, 3, 4].pack('F4'))
end

assert('Rect._load') do
  rect = Rect._load([1, 2, 3, 4].pack('F4'))
  assert_equal(rect.x, 1)
  assert_equal(rect.y, 2)
  assert_equal(rect.width, 3)
  assert_equal(rect.height, 4)
end
