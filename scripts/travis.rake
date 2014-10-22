require 'rubygems'

if ENV['TRAVIS'] and File.exists?( '.toolchain_build' )
  puts <<BANNER

#--- This run required building and caching of a new toolchain               ---
#--- Any other tests will probably cause it to timeout, so I'm going to      ---
#--- exit now. If you think that's an error please run the command:          ---
#---  $ rm .toolchain_build                                                  ---
#--- And try againg!                                                         ---

BANNER
  exit( 0 )
end

namespace :travis do
  desc "Prepare Travis-CI build"
  task :prepare => :fetch_toolchain

  desc "Run Travis-CI build + tests"
  task :run_build => [ 'test:offline', 'default' ]

  begin
    require 'net/ssh'
    require 'net/sftp'

    store_options = {
      :auth_methods => [ 'publickey' ],
      :config       => true,
      :keys         => 'scripts/travis-store.key',
      :paranoid     => false
    }

    release =`git log -1 --pretty=format:%h scripts/toolchain/toolchain-meta.yml scripts/patches`
    toolchain_archive = "toolchain-#{RUBY_PLATFORM}-#{release}#{ENV['TRAVIS'] ? '_travis' : ''}.tar.bz2"

    task :fetch_toolchain do
      puts "Checking for a cached toolchain archive.."
      Net::SFTP.start( ENV['CACHE_HOST'], ENV['CACHE_USER'], store_options ) do |sftp|
        sftp.dir.foreach( '.' ) do |entry|
          if entry.name == toolchain_archive
            begin
              last_progress = 0
              sftp.download!( toolchain_archive, toolchain_archive ) do |event, loader, *args|
                case event
                  when :open
                    print "Downloading #{args[0].local}.."
                  when :get
                    if entry.attributes.size and args[1]
                      progress = args[1] * 100 / entry.attributes.size
                      print "#{progress}..." if progress % 10 == 0 and progress > last_progress
                      last_progress = progress
                    end
                  when :close
                    puts 'done'
                end
              end
            rescue
              puts "Download for #{toolchain_archive} failed."
            end
          end
        end
      end

      if File.exists? toolchain_archive
        mkdir_p 'toolchain'
        sh "tar -xjf #{toolchain_archive} -C toolchain/"
      else
        puts "No cached toolchain available - building it.."
        Rake::Task[ 'travis:cache_toolchain' ].invoke
        Rake::Task[ 'travis:cache_toolchain' ].reenable

        puts "-- toolchain build and cached --"
        touch '.toolchain_build'
      end
    end

    task :cache_toolchain => 'toolchain:archive' do
      puts "Caching toolchain archive #{toolchain_archive}.."
      Net::SFTP.start( ENV['CACHE_HOST'], ENV['CACHE_USER'], store_options ) do |sftp|
        begin
          sftp.upload!( toolchain_archive, toolchain_archive )
        rescue
          fail "Upload of #{toolchain_archive} failed."
        end
      end
    end

  rescue LoadError
    task :fetch_toolchain do
      fail 'Sorry but Net::SFTP is not available!'
    end

    task :cache_toolchain do
      fail 'Sorry but Net::SFTP is not available!'
    end
  end
end