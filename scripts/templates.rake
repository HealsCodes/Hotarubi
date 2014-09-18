require 'erb'
require 'rake/clean'
require 'rake/task_arguments'

# A helper class used by the ERB templates further down.
# ConditionalTemplate provides support to add members at runtime
# and will only ever rewrite the target file if it's missing or would change.
class ConditionalTemplate
  include ERB::Util

  def initialize
    @source = '???'
  end

  def define_attrs( attrs )
    attrs.each do |var, value|
      ( class << self; self; end ).class_eval { attr_accessor var }
      instance_variable_set "@#{var}", value
    end
  end

  def render( source )
    @source = File.basename( source )
    ERB.new( File.read( source ), 0, '<>' ).result( binding )
  end

  def generate( target, source )
    data  = render( source )

    if File.exists? target
      if data == File.read( target )
        return false
      end
    end

    File.open( target, 'w' ) { |io| io.write( data ) }
    true
  end
end

namespace :templates do
  # check if any templates require attention
  task :check do
    SOURCES.each do |src|
      unless uptodate? src.ext( '.d' ), [ src ]
        Rake::Task['templates:generate'].execute( Rake::TaskArguments.new( [ :force ], [ true ] ) )
        break
      end
    end
  end

  desc 'Process all template files'
  task :generate, [:force] do |t, args|
    args.with_defaults( :force => false )
    GENERATED.each do |t|
      if args[:force] or ! uptodate?( t, [ "#{t}.erb" ] )
        Rake::Task[t].execute
      end
    end
  end
end

# Rake helper tasks

rule '.h' => '.h.erb' do |t|
  template = ConditionalTemplate.new

  case File.basename( t.name )
    when 'local_data.h'
      template.define_attrs( :includes => [], :defines => [] )

      SOURCES.each do |src|
        File.open( src ).grep( /^\s*LOCAL_DATA_(INC|DEF)\s*\(\s*.+\s*\)\s*;/ ).each do |match|
          case match
            when /LOCAL_DATA_INC\s*\(\s*([\w. \/-]+[\w.-]+)\s*\)/
              template.includes << $1 unless template.includes.include? $1.strip!

            when /LOCAL_DATA_DEF\s*\(\s*([^;]+)\s*\)/
              template.defines << $1.strip
          end
        end
      end

    when 'release.h'
      template.define_attrs(
        :release       => RELEASE_NAME,
        :release_major => RELEASE_MAJOR,
        :release_minor => RELEASE_MINOR,
        :release_micro => RELEASE_MICRO,
        :git_revision  => "(git:#{`git rev-parse --abbrev-ref HEAD`.chomp}:#{`git log -1 --pretty=format:%h`.chomp})",
        :builder_tag   => "#{DateTime.now.to_s} - #{ENV['USER']}@#{`hostname`.chomp}"
      )
  end
  puts "ERB      #{t.name}" if template.generate( t.name, t.source )
end

CLEAN.include( GENERATED )
