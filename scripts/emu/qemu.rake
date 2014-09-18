require 'yaml'
require 'rake/task_arguments'

QEMU = YAML::load_file( "#{File.dirname( __FILE__ )}/qemu-flags.yml" )
QEMU.merge!( YAML::load_file( 'qemu-flags.yml' ) ) if File.exists?( 'qemu-flags.yml' )
namespace :run do
  namespace :qemu do
    
    desc "Run qemu x86-64 without any disk image"
    task :direct, [:headless] => :kernel do |t, args|
      args.with_defaults( :headless => false )

      # prevent toolchain libraries from interfering
      ENV['LD_LIBRARY_PATH'] = ENV['LD_LIBRARY_PATH'].gsub( "#{Dir.pwd}/toolchain/lib:", '' )
      ENV['DYLD_LIBRARY_PATH'] = ENV['DYLD_LIBRARY_PATH'].gsub( "#{Dir.pwd}/toolchain/lib:", '' )

      sh "#{QEMU[:bin]} #{QEMU[:base_flags].join(' ')} #{'-nographic' if args[:headless]} -kernel hotarubi.elf"
    end

    desc "Run qemu x86-64 using hotarubi.iso (firmware: bios|efi)"
    task :iso, [:firmware, :headless] => 'package:iso' do |t, args|
      # select the platform firmware
      args.with_defaults( :firmware => 'bios', :headless => false )
      fw = args[:firmware]

      # prevent toolchain libraries from interfering
      ENV['LD_LIBRARY_PATH'] = ENV['LD_LIBRARY_PATH'].gsub( "#{Dir.pwd}/toolchain/lib:", '' )
      ENV['DYLD_LIBRARY_PATH'] = ENV['DYLD_LIBRARY_PATH'].gsub( "#{Dir.pwd}/toolchain/lib:", '' )

      sh "#{QEMU[:bin]} #{QEMU[:base_flags].join(' ')} #{'-bios OVMF.fd' if fw == 'efi'} #{'-nographic' if args[:headless]} -cdrom hotarubi.iso"
    end
  end
end