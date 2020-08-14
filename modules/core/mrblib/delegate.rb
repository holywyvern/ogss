class Module
  # rubocop:disable Metrics/MethodLength, Security/Eval
  def delegate(*methods, to:, prefix: nil, allow_nil: nil)
    method_prefix =
      if prefix
        "#{prefix == true ? prefix : to}_"
      else
        ''
      end
    methods.each do |method|
      eval <<-DEFINE, __exc_file__, __LINE__ + 1
        def #{method_prefix}#{method}
          #{to}#{allow_nil ? '&' : ''}.#{method}
        end
      DEFINE
    end
  end
  # rubocop:enable Metrics/MethodLength, Security/Eval
end
