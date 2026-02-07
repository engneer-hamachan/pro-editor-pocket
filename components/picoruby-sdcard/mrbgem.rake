MRuby::Gem::Specification.new('picoruby-sdcard') do |spec|
  spec.license = 'MIT'
  spec.author  = 'hamachang'
  spec.summary = 'SD Card library binding for PicoRuby'

  spec.cc.include_paths << "#{dir}/include"
  spec.cc.include_paths << "#{dir}/ports/esp32"
end
