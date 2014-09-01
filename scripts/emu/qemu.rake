require 'yaml'
require 'rake/task_arguments'

QEMU = YAML::load_file( "#{File.dirname( __FILE__ )}/qemu-flags.yaml" )
namespace :run do
  namespace :qemu do
    
    desc "Run qemu x86-64 without any disk image"
    task :direct => :kernel do
      # prevent toolchain libraries from interfering
      ENV['LD_LIBRARY_PATH'] = ENV['LD_LIBRARY_PATH'].gsub( "#{Dir.pwd}/toolchain/lib:", '' )
      ENV['DYLD_LIBRARY_PATH'] = ENV['DYLD_LIBRARY_PATH'].gsub( "#{Dir.pwd}/toolchain/lib:", '' )

      sh "#{QEMU[:bin]} #{QEMU[:base_flags].join(' ')} -kernel hotarubi.elf"
    end

    desc "Run qemu x86-64 using hotarubi.iso (firmware: bios|efi)"
    task :iso, [:firmware] => 'package:iso' do |t, args|
      # select the platform firmware
      args.with_defaults( :firmware => 'bios' )
      fw = args[:firmware]

      # prevent toolchain libraries from interfering
      ENV['LD_LIBRARY_PATH'] = ENV['LD_LIBRARY_PATH'].gsub( "#{Dir.pwd}/toolchain/lib:", '' )
      ENV['DYLD_LIBRARY_PATH'] = ENV['DYLD_LIBRARY_PATH'].gsub( "#{Dir.pwd}/toolchain/lib:", '' )

      sh "#{QEMU[:bin]} #{QEMU[:base_flags].join(' ')} #{'-bios OVMF.fd' if fw == 'efi'} -cdrom hotarubi.iso"
    end
  end
end