
require 'rake/clean'

TEST_SOURCES      = FileList.new( '**/*.tt' )
TEST_BINARIES     = TEST_SOURCES.map do |source|
  "#{File.dirname( source )}/test-#{File.basename( source ).ext( '' )}"
end

TEST_INCLUDE_BASE = "#{File.dirname( __FILE__ )}"

namespace :test do

  desc "Run offline tests against GTest"
  task :offline do
    TEST_BINARIES.each do |binary|
      puts "DO-TEST  #{File.basename( binary )}"
      sh %{#{binary} --gtest_color=yes}, :verbose => false
    end

    puts "\nOffline test passed :)\n\n"
  end

  desc "Remove test binaries"
  task :clean do
    TEST_BINARIES.each { |binary| rm_f binary if File.exists?( binary ) }
  end

  TEST_SOURCES.each do |source|
    binary = "#{File.dirname( source )}/test-#{File.basename( source ).ext( '' )}"
    file binary => source do |target|
      puts "CC-TEST  #{target}"
      sh %{gcc -I #{TEST_INCLUDE_BASE}           \
               -DRUN_TESTS=1                     \
               -o #{target.name}                 \
               -x c++                            \
               #{target.prerequisites.join(' ')} \
               #{TEST_INCLUDE_BASE}/gtest/*.cc   \
               -lc++}, :verbose => false
    end

    task :offline => binary
  end
end

task :clean => 'test:clean'
