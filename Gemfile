source "https://rubygems.org"
gem "github-pages", group: :jekyll_plugins
gem "jekyll-remote-theme"

# https://stackoverflow.com/questions/8420414/how-to-add-mac-specific-gems-to-bundle-on-mac-but-not-on-linux
def os_is(re)
    RbConfig::CONFIG['host_os'] =~ re
end

gem "cocoapods", platforms: :ruby, install_if: os_is(/darwin/)

# Windows does not include zoneinfo files, so bundle the tzinfo-data gem
gem "tzinfo-data", platforms: [:mingw, :mswin, :x64_mingw, :jruby]

# Performance-booster for watching directories on Windows
gem "wdm", "~> 0.1.0" if Gem.win_platform?