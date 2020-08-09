class PhysFS < Dependency
  def libraries
    [platform.vs? ? 'physfs-static' : 'physfs']
  end
end
