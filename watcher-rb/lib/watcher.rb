# frozen_string_literal: true

require 'fiddle'
require 'date'

module Watcher
  class CEvent < Fiddle::Struct
    layout(
      :path_name,
      :pointer,
      :effect_type,
      :int8_t,
      :path_type,
      :int8_t,
      :effect_time,
      :int64_t,
      :associated_path_name,
      :pointer
    )
  end

  LIB = nil

  def native_solib_file_ending
    case RbConfig::CONFIG['host_os']
    when /darwin/
      'so'
    when /mswin|mingw|cygwin/
      'dll'
    else
      'so'
    end
  end

  def libwatcher_c_lib_path
    version = '0.11.0' # hook: tool/release
    lib_name = "libwatcher-c-#{version}.#{native_solib_file_ending}"
    lib_path = File.join(File.dirname(__FILE__), lib_name)
    raise "Library does not exist: '#{lib_path}'" unless File.exist?(lib_path)

    puts("Using library: '#{lib_path}'")
    lib_path
  end

  def self.lazy_static_solib_handle
    return LIB if LIB

    @lib = Fiddle.dlopen(libwatcher_c_lib_path)
    @lib.extern('void* wtr_watcher_open(char*, void*, void*)')
    @lib.extern('bool wtr_watcher_close(void*)')
    @lib
  end

  def to_utf8
    return '' if @value.nil?
    return @value if @value.is_a?(String)
    return @value.to_s.force_encoding('UTF-8') if @value.respond_to?(:to_s)

    raise TypeError
  end

  def self.c_event_to_event(c_event)
    path_name = as_utf8(c_event.path_name.to_s)
    associated_path_name = as_utf8(c_event.associated_path_name.to_s)
    effect_type = EffectType.new(c_event.effect_type)
    path_type = PathType.new(c_event.path_type)
    effect_time = Time.at(c_event.effect_time / 1e9)
    Event.new(path_name, effect_type, path_type, effect_time, associated_path_name)
  end

  class EffectType
    RENAME = 0
    MODIFY = 1
    CREATE = 2
    DESTROY = 3
    OWNER = 4
    OTHER = 5

    def initialize(value)
      @value = value
    end

    def to_s
      case @value
      when RENAME
        'RENAME'
      when MODIFY
        'MODIFY'
      when CREATE
        'CREATE'
      when DESTROY
        'DESTROY'
      when OWNER
        'OWNER'
      when OTHER
        'OTHER'
      else
        'UNKNOWN'
      end
    end
  end

  class PathType
    DIR = 0
    FILE = 1
    HARD_LINK = 2
    SYM_LINK = 3
    WATCHER = 4
    OTHER = 5

    def initialize(value)
      @value = value
    end

    def to_s
      case @value
      when DIR
        'DIR'
      when FILE
        'FILE'
      when HARD_LINK
        'HARD_LINK'
      when SYM_LINK
        'SYM_LINK'
      when WATCHER
        'WATCHER'
      when OTHER
        'OTHER'
      else
        'UNKNOWN'
      end
    end
  end

  class Event
    attr_reader :path_name, :effect_type, :path_type, :effect_time, :associated_path_name

    def initialize(path_name, effect_type, path_type, effect_time, associated_path_name)
      @path_name = path_name
      @effect_type = effect_type
      @path_type = path_type
      @effect_time = effect_time
      @associated_path_name = associated_path_name
    end

    def to_s
      pnm = "path_name: #{@path_name}"
      ety = "effect_type: #{@effect_type}"
      pty = "path_type: #{@path_type}"
      etm = "effect_time: #{@effect_time}"
      apn = "associated_path_name: #{@associated_path_name}"
      "Event(#{pnm}, #{ety}, #{pty}, #{etm}, #{apn})"
    end
  end

  class Watch
    def initialize(path, callback)
      @lib = Watcher.lazy_static_solib_handle
      @path = path.encode('UTF-8')
      @callback = callback
      @c_callback = Fiddle::Closure::BlockCaller.new(0, [CEvent, :void]) do |c_event, _|
        py_event = Watcher.c_event_to_event(c_event)
        @callback.call(py_event)
      end

      @watcher = @lib.wtr_watcher_open(@path, @c_callback, nil)
      raise 'Failed to open a watcher' unless @watcher
    end

    def close
      return unless @watcher

      @lib.wtr_watcher_close(@watcher)
      @watcher = nil
    end

    def finalize
      close
    end

    def self.finalize(id)
      ObjectSpace._id2ref(id).close
    end
  end
end

if __FILE__ == $PROGRAM_NAME
  events_at = ARGV[0] || '.'
  watcher = Watcher::Watch.new(events_at, method(:puts))
  ObjectSpace.define_finalizer(watcher, Watcher::Watch.method(:finalize).to_proc)
  gets
end
