class File
  include Enumerable

  class << self
    def join(*args)
      args.map(&:to_s).filter(&:present?).join('/').gsub(%r{/+}, '/')
    end

    def dirname(file)
      parts = file.split('/')
      parts.pop
      parts.join('/')
    end

    def readable?(name)
      new(name, 'r') { |f| }
      true
    rescue FileError
      false
    end

    def writable?(name)
      new(name, 'w') { |f| }
      true
    rescue FileError
      false
    end

    def open(name, mode = 'r', &block)
      new(name, mode, &block)
    end

    def read(name)
      file = new(name)
      result = nil
      begin
        result = file.read(file.size)
      ensure
        file.close
      end
      result
    end

    def readlines(name, *args)
      file = new(name)
      result = nil
      begin
        result = file.readlines(*args)
      ensure
        file.close
      end
      result
    end
  end

  def readlines(*args)
    [].tap do |lines|
      lines << readline(*args) until eof?
    end
  end

  def each(*args)
    if block_given?
      yield readline(*args) until eof?
    else
      Enumerator.new(self, :each, *args)
    end
  end

  def each_line(*args, &block)
    each(*args, &block)
  end

  def reopen(*args)
    close unless closed?
    args << path unless args.size.positive?

    initialize(*args)
  end
end
