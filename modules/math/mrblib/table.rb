class Table
  include Enumerable

  def each(&block)
    to_a.each(&block)
  end
end
