require 'open-uri'
require 'openssl'
require 'uri'
require 'yaml'
require 'rake/clean'

# -- patch env to include the default toolchain location
ENV['PATH']              = "#{Dir.pwd}/toolchain/bin:#{ENV['PATH']}"
ENV['LD_LIBRARY_PATH']   = "#{Dir.pwd}/toolchain/lib:#{ENV['LD_LIBRARY_PATH']}"
ENV['DYLD_LIBRARY_PATH'] = "#{Dir.pwd}/toolchain/lib:#{ENV['DYLD_LIBRARY_PATH']}"
# --

TC_META  = YAML::load_file( "#{File.dirname( __FILE__ )}/toolchain-meta.yml" )
TC_FLAGS = YAML::load_file( "#{File.dirname( __FILE__ )}/toolchain-flags.yml" )
TC_LANG  = "#{File.dirname( __FILE__ )}/toolchain.build-language"
TC_RELEASE =`git log -1 --pretty=format:%h scripts/toolchain/toolchain-meta.yml scripts/patches`

TC = {
  :cxx => 'toolchain/bin/x86_64-elf-g++',
  :cc  => 'toolchain/bin/x86_64-elf-gcc',
  :as  => 'toolchain/bin/x86_64-elf-as',
  :ld  => 'toolchain/bin/x86_64-elf-ld',
  :oc  => 'toolchain/bin/x86_64-elf-objcopy',
  :od  => 'toolchain/bin/x86_64-elf-objdump',
  :nm  => 'toolchain/bin/x86_64-elf-nm',
  # used in packaging
  :xorriso => 'toolchain/bin/xorriso',
  :grubimg => 'toolchain/bin/grub-mkimage',
}
TC.merge!( YAML::load_file( 'toolchain.yml' ) ) if File.exists?( 'toolchain.yml' )

$silent  = ""

def _show_progress
  if ENV['TRAVIS'] or ENV['QUIET']
    dots = Thread.new { while true do print '.'; sleep 30; end }
    dots
  end
  nil
end

def _stop_progress( dots )
  dots.kill unless dots.nil?
  puts
end

if ENV['TRAVIS'] or ENV['QUIET']
  # TRAVIS or QUIET was set, no build output
  $silent  = ">/dev/null 2>/dev/null"
end

namespace :toolchain do
  desc "Build the host toolchain and required dependencies (defined in toolchain-meta.yaml)."
  task :build do
    puts "done building."
  end

  desc "Remove source archives, sources and build paths."
  task :clean do
    _CLEAN = FileList.new  << "toolchain/arc" << "toolchain/src" << "toolchain/bld"
    rm_r( _CLEAN, :force => true )
  end

  desc "Remove source archives, sources, build paths and the toolchain binaries."
  task :clobber do
    _CLOBBER = FileList.new << "toolchain"
    rm_r( _CLOBBER, :force => true )
  end

  desc "Create a tar archive of the toolchain binaries"
  task :archive => ['toolchain:build', 'toolchain:clean'] do
    Dir.chdir( 'toolchain' ) do
      sh "tar -cjf ../toolchain-#{RUBY_PLATFORM}-#{TC_RELEASE}#{ENV['TRAVIS'] ? '_travis' : ''}.tar.bz2 ."
    end
  end

  desc "Install Sublime Text support / build language"
  task :sublime do
    sublime_dir = nil
    case RUBY_PLATFORM
      when /darwin/
        sublime_dir = 'Library/Application Support/Sublime Text 3'
      when /linux/
        sublime_dir = '.config/sublime-text-3'
      else
        fail "Unsupported host platform '#{RUBY_PLATFORM}'!"
    end

    makedirs "#{ENV['HOME']}/#{sublime_dir}/Packages/User/"
    rm_f "#{ENV['HOME']}/#{sublime_dir}/Packages/User/Hotarubi.build-language"
    ln_s TC_LANG, "#{ENV['HOME']}/#{sublime_dir}/Packages/User/Hotarubi.build-language"
  end

  case RUBY_PLATFORM
    when /darwin/
      so_suffix = '.dylib'
      deps_host  = :deps_darwin
    when /linux/
      so_suffix = '.so'
      deps_host  = :deps_linux
    else
      fail "Unsupported host platform '#{RUBY_PLATFORM}'!"
  end

  # auto generate tasks
  TC_META[ :archives ].each do |target, meta|
    # setup source downloads
    source_arch   = "toolchain/arc/#{meta[ :name ]}" if meta.has_key? :name 
    source_arch ||= "toolchain/arc/#{File.basename( meta[ :uri ] )}"
    source_path   = "toolchain/src/#{File.basename( source_arch, source_arch.gsub( /^.*(\.tar.\.*)/, '\1' ))}"
    build_path    = "toolchain/bld/#{File.basename( source_path )}"
    prefix_path   = "#{Dir.pwd}/toolchain/"

    file source_arch do
      makedirs "toolchain/arc"
      tries = 3
      while tries > 0 do
        error = nil
        begin
          length = 0
          progress_proc = lambda do |size|
            length ||= meta[:size] # some servers don't send a content length..
            return if size.nil? or length.nil?

            print ".. fetching #{source_arch} [#{size / 1024}/#{length / 1024}kb]...    \r"
          end
          
          unless $silent.empty?
            # no progress on Travis-CI builds
            progress_proc = lambda { |size| true }
            puts ".. fetching #{source_arch}..."
          end

          URI.parse( meta[ :uri ] ).open( {
            :content_length_proc => lambda { |size| length = size },
            :progress_proc       => progress_proc,
            :read_timeout        => 60,
            :redirect            => true } ) do |io|

            File.open( source_arch, 'wb' ) { |f| f.write( io.read ); puts }
          end

          unless length == meta[ :size ]
            error = "Size mismatch for #{source_arch} (expected #{meta[ :size ]} got #{length})!"
          end

          if meta.has_key? :md5
            hash = OpenSSL::Digest::MD5.new
            hash.update( File.read( source_arch ) )
            error = "MD5 mismatch for #{source_arch}!" if hash.hexdigest != meta[ :md5 ]
          end

          if meta.has_key? :sha
            hash = OpenSSL::Digest::SHA256.new
            hash.update( File.read( source_arch ) )
            error = "SHA256 mismatch for #{source_arch}!" if hash.hexdigest != meta[ :sha ]
          end

        rescue SystemCallError => err
            error = "Error while loading #{meta[ :uri ]}: #{err.to_s}"
        end
        tries -= 1
        
        break if error.nil?
        unless error.nil?
          if tries == 0
            fail error
          else
            puts error
          end
          rm_f source_arch
        end
      end
    end

    # setup archive extraction
    file source_path => source_arch do
      unless $silent.empty?
        Rake::FileUtilsExt.verbose( false )
      end

      makedirs "toolchain/src"
      rm_rf "#{source_path}", :verbose => false
      tar_flags = case File.extname( meta[ :uri ] )
        when '.gz'   then '-xzf'
        when '.tgz'  then '-xzf'
        when '.bz2'  then '-xjf'
        when '.tbz2' then '-xjf'
        else '-xf'
      end

      sh "tar #{tar_flags} #{source_arch} -C toolchain/src/"
      # apply patches as needed
      patches = meta[ :patches ] ||= []
      prepare = meta[ :prepare ] ||= []

      Dir.chdir( source_path ) do
        patches.each do |patch|
          begin
            sh "patch -p1 < ../../../scripts/patches/#{patch} #{$silent}"
          rescue
            puts "Failed to apply #{patch}!"
            raise
          end
        end

        prepare.each do |cmd|
          begin
            cmd = cmd.gsub( /@PREFIX@/, prefix_path )\
                     .gsub( /@PWD@/, Dir.pwd )\
                     .gsub( /@TOP@/, File.dirname( prefix_path ) )

            sh "#{cmd} #{ '> prepare.log 2>&1' unless $silent.empty?}"
            rm_f 'prepare.log'
          rescue
            puts "Failed to run preparation command: #{cmd}"
            if File.exists? 'perpare.log'
              puts "Command output:"
              open( 'prepare.log', 'r' ) do |io|
                puts io.read
              end
            end

            raise
          end
        end

        File.chmod( 0755, 'configure' )
      end

      unless $silent.empty?
        Rake::FileUtilsExt.verbose( true )
      end
    end

    task :fetch  => source_arch
    task :unpack => source_path

    # setup the build target
    deps = [ source_path ] + ( meta[ :deps ] ||= [] ) + ( meta[ deps_host ] ||= [] )

    file "toolchain/.build-#{target}" => deps do
      progress = nil
      unless $silent.empty?
        print "building #{target}"
        progress = _show_progress
        Rake::FileUtilsExt.verbose( false )
      end

      unless File.exists? "toolchain/.build-#{target}"
        ( conf_flags = [ "--prefix=@PREFIX@" ] + ( meta[ :config_flags ] ||= [] ) ).map! do |flag|
          flag.gsub( /@PREFIX@/, prefix_path)\
              .gsub( /@PWD@/, Dir.pwd )\
              .gsub( /@TOP@/, File.dirname( prefix_path ) )
        end

        makedirs build_path
        Dir.chdir( build_path ) do
          begin
            sh "../../src/#{File.basename( source_path )}/configure #{conf_flags.join( ' ' )} #{$silent}"

            unless meta.has_key? :targets
              sh "make #{ '> build.log 2>&1' unless $silent.empty?}"
              sh "make install-strip #{$silent} || make install #{$silent}"
            else
              # build only specific targets
              meta[ :targets ].each do |target|
                sh "make #{target} #{$silent}"
              end
            end
          rescue
            # dump conifg.log files if possible
            unless $silent.empty?
              puts "building failed - trying to dump config.log(s).."
              Dir.glob( '**/config.log' ).each do |logpath|
                puts "--- #{logpath}:\n#{File.read( logpath )}\n\n"
              end

              puts "\n\nbuilding failed - trying to dump build.log.."
              Dir.glob( '**/build.log' ).each do |logpath|
                puts "--- #{logpath}:\n#{File.read( logpath )}\n\n"
              end
            end

            raise
          end
        end
      end
      touch "toolchain/.build-#{target}"

      unless $silent.empty?
        _stop_progress( progress )
        Rake::FileUtilsExt.verbose( true )
      end
    end

    #desc "Build #{target}"
    task target => "toolchain/.build-#{target}"

    unless meta.has_key? :optional
      task :build => target
    end
  end
end

# Rake helper tasks

[ '.c', '.cc', '.S' ].each do |source_ext|
  rule '.d' => [ source_ext ] do |t|
    include_path = "-I #{File.dirname( t.source )} #{TC_FLAGS[ :INCLUDE ].flatten.join( ' ' )}"

    File.open( t.name, 'w') do |depfile|
      depfile.write( `#{TC[ :cxx ]} #{include_path} -std=gnu++11 -MM #{t.source}`.gsub( /^(.*\.o:)/, t.source.ext( '.o:' ) ) )
    end
  end
end

# Rake helper functions

def cc_compile( source, target, **options )
  use32 = options.delete( :use32 ) || false

  include_paths = [ "-I #{File.dirname( source )}" ]
  if File.extname( source ) =~ /\.(cpp|cc|hpp)/
    compiler      = TC[ :cxx ]
    compile_flags = include_paths + TC_FLAGS[ ( use32 ? :CXXFLAGS_32 : :CXXFLAGS_64 ) ].flatten
  else
    compiler      = TC[ :cc ]
    compile_flags = include_paths + TC_FLAGS[ ( use32 ? :CFLAGS_32 : :CFLAGS_64 ) ].flatten
  end

  puts "CC  #{use32 ? '[32]' : '    '} #{target}"
  sh "#{compiler} #{compile_flags.join( ' ' )} -c -o #{use32 ? target.ext( '.o32' ) : target} #{source}", :verbose => false

  if use32
    sh "#{TC[ :oc ]} -I elf32-i386 -O elf64-x86-64 #{target.ext( '.o32' )} #{target}", :verbose => false
    rm target.ext( '.o32' ), :verbose => false
  end
end

def cc_link( sources, target, **options )
  # provide some defaults
  crtsuffix = options.has_key?( :kernel ) ? '-kernel' : ''
  link_flags = TC_FLAGS[ :LDFLAGS ].flatten.join( ' ' )
  defaults = {
    :crtbegin => '',
    :crtend   => '',
    :sources  => ''
  }

  # check for crti / crte and create the required linker context for them
  Array.new( sources ).each do |source|
    if File.basename( source ) == 'crti.o'
      defaults[ :crtbegin ] = "#{source} #{`#{TC[ :cc ]} #{link_flags} -print-file-name=crtbegin#{crtsuffix}.o`.chomp}"
      sources.delete( source )
    elsif File.basename( source ) == 'crtn.o'
      defaults[ :crtend ] = "-lgcc #{`#{TC[ :cc ]} #{link_flags} -print-file-name=crtend#{crtsuffix}.o`.chomp} #{source}"
      sources.delete( source )
    end
  end
  defaults[ :sources ] = sources.join( ' ' )

  puts "LD       #{target}"
  if defaults[ :crtbegin ].empty? and defaults[ :crtend ].empty?
    # normal link, no crt
    sh "#{TC[ :cc ]} -o #{target} #{link_flags} #{sources}", :verbose => false
  else
    # link against crt / libgcc, this requires a special order
    sh "#{TC[ :cc ]} -o #{target} #{link_flags} #{defaults[ :crtbegin ]} #{defaults[ :sources ]} #{defaults[ :crtend ]}", :verbose => false
  end
end

def cc_64to32( source, target, **options )
  cleanup = options.delete( :cleanup ) || false

  puts "OC  [32] #{target}"
  sh "#{TC[ :oc ]} -I elf64-x86-64 -O elf32-i386 #{source} #{target}", :verbose => false
  if cleanup
    rm source, :verbose => false
  end
end
