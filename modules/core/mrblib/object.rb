class Object
  def blank?
    !self || empty?
  end

  def present?
    !blank?
  end

  def empty?
    false
  end

  def presence
    present? ? self : nil
  end
end
