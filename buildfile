./: tests/ doc{README.md} legal{LICENSE} manifest

./: lib{lyrahgames-buddy-system}: lyrahgames/buddy_system/hxx{**}
{
  cxx.export.poptions = "-I$out_root" "-I$src_root"
}
hxx{**}: install.subdirs = true

tests/: install = false

