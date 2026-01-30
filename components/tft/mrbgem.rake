MRuby::Gem::Specification.new('tft') do |spec|
  spec.license = 'MIT'
  spec.author  = 'hamachang'
  spec.summary = 'TFT_eSPI library binding for PicoRuby'

  # Add TFT_eSPI include paths
  spec.cc.include_paths << "#{dir}/include"
  spec.cc.include_paths << "#{dir}/ports/esp32"

  # Only compile mrubyc version
  if Dir.exist?("#{dir}/src/mrubyc")
    spec.objs << "#{dir}/src/mrubyc/tft.cpp"
  end
end
