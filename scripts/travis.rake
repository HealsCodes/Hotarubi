
namespace :travis do
  desc "Prepare Travis-CI build"
  task :prepare => 'toolchain:build'

  desc "Run Travis-CI build + tests"
  task :run_build => [ 'test:offline', 'default' ]
end