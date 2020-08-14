class Module
  # rubocop:disable Metrics/MethodLength
  def delegate(*methods, to:, prefix: false, allow_nil: false)
    prefix = "#{to}_" if prefix == true
    prefix = "#{prefix}_" if prefix == false

    methods.each do |name|
      method_name = "#{prefix}#{name}"
      if /\=$/.match?(name.to_s)
        define_method(method_name) { |value| send(to)&.send(name, value) }
      else
        define_method(method_name) do |*args, **karg, &blk|
          if allow_nil
            send(to)&.send(name, *args, **karg, &blk)
          else
            send(to).send(name, *args, **karg, &blk)
          end
        end
      end
    end
  end
  # rubocop:enable Metrics/MethodLength
end
