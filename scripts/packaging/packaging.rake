require 'tmpdir'
require 'rake/clean'

GRUB2_EFI_BOOT="#{File.dirname( __FILE__ )}/CD_root/boot/grub/efi.img"

namespace :package do

  desc "Clear files generated by packaging commands."
  task :clean do
    _CLEAN = FileList.new
    _CLEAN.include( 'hotarubi.iso' )
    _CLEAN.include( GRUB2_EFI_BOOT )

    rm_r( _CLEAN, :force => true )
  end


  desc "Generate a bootable ISO image"
  task :iso => [ :kernel, GRUB2_EFI_BOOT ] do
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
      sh %{#{TC[:xorriso]} -as mkisofs -o hotarubi.iso -quiet          \
                                       -b "boot/isolinux/isolinux.bin" \
                                       -c "boot/isolinux/isolinux.cat" \
                                       -no-emul-boot                   \
                                       -boot-load-size 4               \
                                       -boot-info-table                \
                                       -rock                           \
                                       --efi-boot boot/grub/efi.img    \
                                       "#{cd_root}"}, :verbose => false
    end
  end

  file GRUB2_EFI_BOOT do |t|

    grub_modules = %w[ efi_gop efi_uga fat font gfxterm iso9660 multiboot 
                       multiboot2 normal terminal ]

    # creation of BOOTX64.efi and the empty image is identical on linux / darwin
    # the rest is completely different..
    Rake::FileUtilsExt.verbose( false )
    puts "GENEFI   BOOTX64.efi"
    sh "#{TC[:grubimg]} -O x86_64-efi -o BOOTX64.efi #{grub_modules.join(' ')}"

    puts "GENIMG   boot/grub/efi.img"
    sh "dd if=/dev/zero of=#{t.name} bs=512 count=2048 2>/dev/null"

    case RUBY_PLATFORM
      when /darwin/
        # darwin uses hdiutil and newfs_msdos..
        img_dev=`hdiutil attach -nomount #{t.name}`.chomp!
        
        sh      "newfs_msdos -F 12 -v EFI_BOOT #{File.basename( img_dev )} >/dev/null 2>/dev/null"
        sh      "hdiutil mountvol -quiet #{img_dev}"
        mkdir_p '/Volumes/EFI_BOOT/EFI/BOOT/'
        cp      'BOOTX64.efi', '/Volumes/EFI_BOOT/EFI/BOOT/BOOTX64.efi'
        sh      "hdiutil detach -quiet #{img_dev}"

      when /linux/
        # whereas linux uses mkfs.vfat, mmd and mcopy (e.g. the mtools)
        sh "/usr/bin/mkfs.vfat -F 12 -n EFI_BOOT #{t.name}"
        sh "mmd -i #{t.name} ::/EFI"
        sh "mmd -i #{t.name} ::/EFI/BOOT"
        sh "mcopy -i #{t.name} BOOTX64.efi ::/EFI/BOOT/BOOTX64.efi"
    end

    rm_f 'BOOTX64.efi'
    Rake::FileUtilsExt.verbose( true )
  end
end

task :clean => 'package:clean'

