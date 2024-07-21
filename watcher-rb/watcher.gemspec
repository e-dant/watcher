# frozen_string_literal: true

Gem::Specification.new do |spec|
  spec.name = 'watcher'
  spec.version = File.read('../.version').strip
  spec.authors = ['Will']
  spec.email = ['willhl@protonmail.com']
  spec.summary = 'Filesystem watcher. Simple, efficient and friendly.'
  spec.description = 'Watch events on (almost) any filesystem.'
  spec.homepage = 'https://github.com/e-dant/watcher'
  spec.license = 'MIT'
  spec.required_ruby_version = Gem::Requirement.new('>= 2.7.0')
  spec.metadata['homepage_uri'] = spec.homepage
  spec.metadata['changelog_uri'] = "#{spec.homepage}/changelog.md"
  spec.metadata['allowed_push_host'] = 'TODO'
  spec.files = `ls -1 lib`.split("\n").map { |f| "lib/#{f}" }
  spec.bindir = 'bin'
  spec.executables = 'watcher'
  spec.require_paths = ['lib']
  spec.add_dependency 'ffi'
  spec.add_development_dependency 'bundler', '~> 2.0'
  spec.add_development_dependency 'rake', '~> 13.0'
  spec.add_development_dependency 'rspec', '~> 3.0'
end
