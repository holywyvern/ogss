assert('Table.new') do
  table = Table.new(1, 2, 3)
  assert_equal(table.xsize, 1)
  assert_equal(table.ysize, 2)
  assert_equal(table.zsize, 3)
end

assert('Table#xsize') do
  table = Table.new(2, 3, 4)
  assert_equal(table.xsize, 2)
end

assert('Table#ysize') do
  table = Table.new(2, 3, 4)
  assert_equal(table.ysize, 3)
end

assert('Table#zsize') do
  table = Table.new(2, 3, 4)
  assert_equal(table.zsize, 4)
end

assert('Table#resize') do
  table = Table.new(1, 2, 3)
  table.resize(8, 9, 10)
  assert_equal(table.xsize, 8)
  assert_equal(table.ysize, 9)
  assert_equal(table.zsize, 10)
end

assert('Table#[]') do
  table = Table.new(10, 10, 10)
  table[2, 2, 2] = 23
  assert_equal(table[2, 2, 2], 23)
end

assert('Table#to_a') do
  table = Table.new(10, 10)
  assert_equal(100, table.to_a.size)
end

assert('Table._load') do
  pack = [3, 2, 2, 1, 4].pack('L5') + [1, 0, 0, 2].pack('S4')
  table = Table._load(pack)
  assert_equal(2, table.xsize)
  assert_equal(2, table.ysize)
  assert_equal(1, table.zsize)
  assert_equal(1, table[0, 0])
  assert_equal(2, table[1, 1])
end

assert('Table#_dump') do
  table = Table.new(2, 2)
  pack = [3, 2, 2, 1, 4].pack('L5') + [0, 0, 0, 0].pack('S4')
  assert_equal(pack, table._dump)
end
