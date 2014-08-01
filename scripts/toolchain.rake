require 'open-uri'
require 'openssl'
require 'uri'
require 'yaml'
require 'rake/clean'

TC_META  = YAML::load_file( "#{File.dirname( __FILE__ )}/toolchain-meta.yaml" )
TC_FLAGS = YAML::load_file( "#{File.dirname( __FILE__ )}/toolchain-flags.yaml" )

TC = YAML::load_file( "toolchain.yaml" )

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

  case RUBY_PLATFORM
    when /darwin/
      so_suffix = '.dylib'
    when /linux/
      so_suffix = '.so'
    when /[Ww]indows/
      so_suffix = '.dll'
    else
      fail "Unsupported host platform '#{RUBY_PLATFORM}'!"
  end

  # auto generate tasks
  TC_META[ :archives ].each do |target, meta|
    # setup source downloads
    source_arch = "toolchain/arc/#{File.basename( meta[ :uri ] )}"
    source_path = "toolchain/src/#{File.basename( meta[ :uri ], meta[ :uri ].gsub( /^.*(\.tar.\.*)/, '\1' ))}"
    build_path  = "toolchain/bld/#{File.basename( source_path )}"

    file source_arch do
      makedirs "toolchain/arc"
      begin
        length = 0
        URI.parse( meta[ :uri ] ).open( {
          :content_length_proc => lambda { |size| length = size },
          :progress_proc       => lambda { |size| print ".. fetching #{source_arch} [#{size / 1024}/#{length / 1024}kb]...    \r" },
          :read_timeout        => 60,
          :redirect            => true } ) do |io|

          File.open( source_arch, 'wb' ) { |f| f.write( io.read ); puts }
        end

        unless length == meta[ :size ]
          fail "Size mismatch for #{source_arch} (expected #{meta[ :size ]} got #{length})!"
        end

        if meta.has_key? :md5
          hash = OpenSSL::Digest::MD5.new
          hash.update( File.read( source_arch ) )
          fail "MD5 mismatch for #{source_arch}!" if hash.hexdigest != meta[ :md5 ]
        end

        if meta.has_key? :sha
          hash = OpenSSL::Digest::SHA256.new
          hash.update( File.read( source_arch ) )
          fail "SHA256 mismatch for #{source_arch}!" if hash.hexdigest != meta[ :sha ]
        end

      rescue SystemCallError => err
          fail "Error while loading #{meta[ :uri ]}: #{err.to_s}"
      end
    end

    # setup archive extraction
    file source_path => source_arch do
      makedirs "toolchain/src"
      tar_flags = case File.extname( meta[ :uri ] )
        when '.gz'   then '-xzf'
        when '.tgz'  then '-xzf'
        when '.bz2'  then '-xjf'
        when '.tbz2' then '-xjf'
        else '-xf'
      end

      sh "tar #{tar_flags} #{source_arch} -C toolchain/src/"
    end

    # setup the build target
    deps = [ source_path ] + ( meta[ :deps ] ||= [] )

    file "toolchain/.build-#{target}" => deps do
      unless File.exists? "toolchain/.build-#{target}"
        ( conf_flags = [ "--prefix=@PREFIX@" ] + ( meta[ :config_flags ] ||= [] ) ).map! do |flag|
          flag.gsub( /@PREFIX@/, "#{Dir.pwd}/toolchain/" )
        end

        makedirs build_path
        Dir.chdir( build_path ) do
          sh "../../src/#{File.basename( source_path )}/configure #{conf_flags.join( ' ' )}"
          sh "make && make install"
        end
      end
      touch "toolchain/.build-#{target}"
    end

    #desc "Build #{target}"
    task target => "toolchain/.build-#{target}"

    task :build => target
  end
end

# Rake helper functions

def cc_compile( source, target, **options )
    use32 = options.delete( :use32 ) || false

    include_paths = [ "-I #{File.basename( source )}" ]
    compile_flags = include_paths + TC_FLAGS[ ( use32 ? :CFLAGS_32 : :CFLAGS_64 ) ].flatten

    puts "CC #{use32 ? '[32]' : '    '} #{target}"
    sh "#{TC[ :cc ]} #{compile_flags.join( ' ' )} -c -o #{use32 ? target.ext( '.o32' ) : target} #{source}", :verbose => false

    if use32
      sh "#{TC[ :oc ]} -I elf32-i386 -O elf64-x86-64 #{target.ext( '.o32' )} #{target}", :verbose => false
      rm target.ext( '.o32' ), :verbose => false
    end
end

def cc_link( sources, target, **options )
    puts "LD      #{target}"
    sh "#{TC[ :ld ]} #{TC_FLAGS[ :LDFLAGS ].flatten.join( ' ' )} -o #{target} #{sources.join( ' ' )}", :verbose => false
end

def cc_64to32( source, target, **options )
  cleanup = options.delete( :cleanup ) || false

  puts "OC [32] #{target}"
  sh "#{TC[ :oc ]} -I elf64-x86-64 -O elf32-i386 #{source} #{target}", :verbose => false
  if cleanup
    rm source, :verbose => false
  end
end

def cc_rakedeps( sources, target, **options )
  File.open( target, 'w' ) do |depfile|

    unless sources.empty?
      include_paths =  sources.map { |src| "-I #{File.dirname( src )}" }
      include_paths << TC_FLAGS[ :INCLUDE ].flatten
      include_paths = include_paths.join( ' ' )

      `#{TC[ :cc ]} #{include_paths} -MM #{sources.join( ' ' )}`.each_line do |line|

        obj, src = line.strip.split( ':' )
        srcs   ||= ''
        srcs.strip!

        depfile.write( "file '#{obj}' => [ '#{srcs.gsub( /\s+/, "' , '" )}' ] \n" )
      end
    end
  end
end
