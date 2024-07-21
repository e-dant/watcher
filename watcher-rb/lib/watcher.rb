# rubocop:disable Metrics/MethodLength
# rubocop:disable Style/Documentation
# frozen_string_literal: true

require 'date'
require 'fiddle'
require 'fiddle/closure'
require 'fiddle/cparser'
require 'fiddle/import'
require 'fiddle/struct'

module Watcher
  C_EVENT_DATA = [
    'char* path_name',
    'int8_t effect_type',
    'int8_t path_type',
    'int64_t effect_time',
    'char* associated_path_name'
  ].freeze

  C_EVENT = Fiddle::Importer.struct(C_EVENT_DATA)

  C_EVENT_C_TYPE = Fiddle::Importer.parse_struct_signature(C_EVENT_DATA)

  # Wraps: void (* wtr_watcher_callback)(struct wtr_watcher_event event, void* context)

  C_CALLBACK_BRIDGE_CLOSURE = Class.new(Fiddle::Closure) {
    def call(c_event, rb_cb)
      rb_cb.call(Watcher.c_event_to_event(c_event))
    end
  }.new(Fiddle::TYPE_VOID, [Fiddle::TYPE_VOIDP, Fiddle::TYPE_VOIDP])

  def self.to_utf8
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
      native_solib_file_ending =
        case RbConfig::CONFIG['host_os']
        when /darwin/
          'so'
        when /mswin|mingw|cygwin/
          'dll'
        else
          'so'
        end
      version = '0.11.0' # hook: tool/release
      lib_name = "libwatcher-c-#{version}.#{native_solib_file_ending}"
      lib_path = File.join(File.dirname(__FILE__), lib_name)
      puts("Using library: '#{lib_path}'")
      @_path = path.encode('UTF-8')
      @_callback = callback
      @_lib = Fiddle.dlopen(lib_path)
      @_wtr_watcher_open = Fiddle::Function.new(
        @_lib['wtr_watcher_open'],
        [Fiddle::TYPE_VOIDP, Fiddle::TYPE_VOIDP, Fiddle::TYPE_VOIDP],
        Fiddle::TYPE_VOIDP
      )
      @_wtr_watcher_close = Fiddle::Function.new(
        @_lib['wtr_watcher_close'],
        [Fiddle::TYPE_VOIDP],
        Fiddle::TYPE_VOIDP
      )
      @_c_callback_bridge = Fiddle::Function.new(
        C_CALLBACK_BRIDGE_CLOSURE,
        [Fiddle::TYPE_VOIDP, Fiddle::TYPE_VOIDP],
        Fiddle::TYPE_VOID
      )
      @_watcher = @_wtr_watcher_open.call(@_path, @_c_callback_bridge, @_callback)
      raise 'Failed to open a watcher' unless @_watcher
    end

    def close
      return unless _watcher

      _wtr_watcher_close(_watcher)
      _watcher = nil
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

# rubocop:enable Style/Documentation
# rubocop:enable Metrics/MethodLength
