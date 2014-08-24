require 'tmpdir'
require 'rake/clean'

namespace :package do
  desc "Generate a bootable ISO image"
  task :iso => :kernel do
    Dir.mktmpdir( 'Hotarubi_iso_' ) do |cd_root|
      iso_sources = FileList.new

      iso_sources.include( "#{File.dirname( __FILE__ )}/CD_root/*" )
      iso_sources.include( 'hotarubi.elf' )
      
      if Dir.exists? 'iso'
        iso_sources.include( 'iso/*' )
      end

      puts "PREPISO"
      cp_r( iso_sources, cd_root, :verbose => false )

      puts "GENISO   hotarubi.iso"
      sh %{#{TC[:xorriso]} -as mkisofs -o hotarubi.iso -quiet     \
                                       -b "isolinux/isolinux.bin" \
                                       -c "isolinux/isolinux.cat" \
                                       -no-emul-boot              \
                                       -boot-load-size 4          \
                                       -boot-info-table           \
                                       -rock                      \
                                       "#{cd_root}"}, :verbose => false
    end
  end
end

CLEAN.include( 'hotarubi.iso' )
