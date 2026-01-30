MRuby::Gem::Specification.new('picoruby-tft') do |spec|
  spec.license = 'MIT'
  spec.author  = 'hamachang'
  spec.summary = 'TFT library binding for PicoRuby'

  spec.cc.include_paths << "#{dir}/include"
  spec.cc.include_paths << "#{dir}/ports/esp32"

  if Dir.exist?("#{dir}/src/mrubyc")
    spec.objs << "#{dir}/src/mrubyc/tft.cpp"
  end
end
