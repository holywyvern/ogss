module DependencyConsole
  class << self
    def log(msg)
      puts '======================================================='
      puts msg
      puts '======================================================='
    end
  end
end
