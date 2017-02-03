{
  "targets": [
    {
      "target_name": "ghostscript-js",
      "sources": [ "src/ghostscript.cpp" ],
      "link_settings": {
        # "libraries": [ "-L/home/meja/Dev/Github/ghostscript-js/src/ghostpdl/sobin/libgs.so" ]
        "libraries": [ "<(module_root_dir)/src/ghostpdl/sobin/libgs.so" ]
      },
      "include_dirs" : [
        "<!(node -e \"require('nan')\")"
      ]
    }
  ]
}
